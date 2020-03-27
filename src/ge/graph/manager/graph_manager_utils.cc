/**
 * Copyright 2019-2020 Huawei Technologies Co., Ltd
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "graph/manager/graph_manager_utils.h"

#include <set>
#include <utility>

#include "framework/common/debug/ge_log.h"
#include "common/ge/ge_util.h"
#include "graph/debug/ge_attr_define.h"
#include "common/string_util.h"
#include "graph/compute_graph.h"
#include "graph/op_desc.h"
#include "graph/optimize/common/params.h"
#include "omg/omg_inner_types.h"
#include "runtime/mem.h"

namespace ge {
GraphNode::GraphNode(GraphId graph_id)
    : graph_id_(graph_id),
      run_flag_(false),
      subgraph_ptr_list_(),
      graph_(nullptr),
      compute_graph_(nullptr),
      build_flag_(false),
      load_flag_(false),
      ge_model_(nullptr),
      sem_(1) {
  graph_run_async_listener_ = MakeShared<RunAsyncListener>();
  if (graph_run_async_listener_ == nullptr) {
    GELOGE(MEMALLOC_FAILED, "Make shared failed");
  }
}

GraphNode::~GraphNode() = default;

void GraphNode::Lock() {
  sem_.Push(0);
}

void GraphNode::Unlock() {
  uint8_t unused;
  sem_.Pop(unused);
}

SubGraphInfo::SubGraphInfo() : subgraph_ptr_(nullptr), ge_model_ptr_(nullptr), malloc_flag_(false) {}

SubGraphInfo::~SubGraphInfo() {
  if (malloc_flag_) {
    for (auto &buffer_addr : buffer_addr_) {
      if (buffer_addr == nullptr) {
        continue;
      }
      rtError_t rt_ret;
      rt_ret = rtFreeHost(buffer_addr);
      buffer_addr = nullptr;
      if (rt_ret != RT_ERROR_NONE) {
        GELOGE(rt_ret, "[GraphManager] subgraph free buffer failed, modelId = %u", model_id_info_.model_id);
      }
    }
  }
}

Status SubGraphInfo::FreeInOutBuffer() {
  if (malloc_flag_) {
    for (auto iter = buffer_addr_.begin(); iter != buffer_addr_.end(); ++iter) {
      rtError_t rt_ret;
      rt_ret = rtFreeHost(*iter);
      if (rt_ret != RT_ERROR_NONE) {
        GELOGE(rt_ret, "[GraphManager] subgraph free buffer failed, modelId = %u", model_id_info_.model_id);
        buffer_addr_.erase(buffer_addr_.begin(), iter);
        return GE_GRAPH_FREE_FAILED;
      }
    }
    buffer_addr_.clear();

    malloc_flag_ = false;
    return SUCCESS;
  } else {
    GELOGI("[GraphManager] not malloc buffer, modelId = %u", model_id_info_.model_id);
    return SUCCESS;
  }
}

GraphModelListener::GraphModelListener() : result_code_(0), is_finished_(false), mutex_(nullptr), condition_(nullptr) {}

Status GraphModelListener::SetCondition(std::mutex *mutex, std::condition_variable *cond) {
  if (mutex == nullptr || cond == nullptr) {
    GELOGE(GE_GRAPH_PARAM_NULLPTR, "[GraphManager] param is NULL.");
    return GE_GRAPH_PARAM_NULLPTR;
  }

  mutex_ = mutex;
  condition_ = cond;
  return SUCCESS;
}

Status GraphModelListener::OnComputeDone(uint32_t model_id, uint32_t task_id, uint32_t result) {
  GELOGI(
      "[GraphManager] graph compute call back, model_id:%u, task_id:%u, "
      "resultCode:%u.",
      model_id, task_id, result);
  GE_IF_BOOL_EXEC(condition_ == nullptr, GELOGE(FAILED, "[GraphModelListener] condition is null."); return FAILED);
  std::lock_guard<std::mutex> lock(*mutex_);
  result_code_ = result;
  is_finished_ = true;
  condition_->notify_all();

  return SUCCESS;
}

uint32_t GraphModelListener::GetResultCode() const {
  if (!is_finished_) {
    GELOGE(INTERNAL_ERROR, "[GraphManager] model not run finish.");
    return INTERNAL_ERROR;
  }
  return result_code_;
}

Status GraphModelListener::ResetResult() {
  if (mutex_ == nullptr) {
    GELOGE(GE_GRAPH_PARAM_NULLPTR, "[GraphManager] param is NULL.");
    return GE_GRAPH_PARAM_NULLPTR;
  }

  std::lock_guard<std::mutex> lock(*mutex_);
  result_code_ = 0;
  is_finished_ = false;

  return SUCCESS;
}

void RunAsyncListener::SetCallback(const std::function<void(Status)> &callback) {
  sem_.Push(0);
  callback_ = callback;
}

Status RunAsyncListener::OnComputeDone(uint32_t model_id, uint32_t task_id, uint32_t result) {
  GELOGI("[GraphManager] run graph async call back, modelId:%u, taskId:%u, resultCode:%u.",
         model_id, task_id, result);
  GE_CHECK_NOTNULL(callback_);
  callback_(result);
  uint8_t unused;
  sem_.Pop(unused);
  return SUCCESS;
}

bool HasCalcOp(const ComputeGraphPtr &graph) {
  if (graph == nullptr) {
    return false;
  }

  static const std::set<std::string> calc_op_type = {CONVOLUTION, DECONVOLUTION, FULL_CONNECTION};

  for (const auto &node : graph->GetAllNodes()) {
    OpDescPtr op_desc = node->GetOpDesc();
    GE_IF_BOOL_EXEC(op_desc == nullptr, GELOGE(FAILED, "Node GetOpDesc is nullptr"); return false);
    if (calc_op_type.find(op_desc->GetType()) != calc_op_type.end()) {
      return true;
    }
  }

  return false;
}

Status CheckTinyCalc(const char *cal_conf, const ComputeGraphPtr &graph) {
  if ((Params::Instance() != nullptr) && (Params::Instance()->GetTarget() != TARGET_TYPE_TINY)) {
    return SUCCESS;
  }

  if (cal_conf != nullptr && *cal_conf != '\0') {
    return SUCCESS;
  }

  if (HasCalcOp(graph)) {
    return GE_GRAPH_PARAM_NULLPTR;
  }

  return SUCCESS;
}

Status ParseOutNodes(const string &out_nodes) {
  try {
    if (!out_nodes.empty()) {
      domi::GetContext().out_nodes_map.clear();
      domi::GetContext().user_out_nodes.clear();

      vector<string> nodes_v = StringUtils::Split(out_nodes, ';');
      for (const string &node : nodes_v) {
        vector<string> key_value_v = StringUtils::Split(node, ':');
        if (key_value_v.size() != 2) {  // must contain 2 items
          GELOGE(GE_GRAPH_PARAM_NULLPTR, "Invalid outNodes: %s", node.c_str());
          return GE_GRAPH_PARAM_NULLPTR;
        }
        auto iter = domi::GetContext().out_nodes_map.find(key_value_v[0]);
        int32_t index = std::stoi(StringUtils::Trim(key_value_v[1]));
        if (iter != domi::GetContext().out_nodes_map.end()) {
          iter->second.emplace_back(index);
        } else {
          std::vector<int32_t> index_v;
          index_v.emplace_back(index);
          domi::GetContext().out_nodes_map.emplace(key_value_v[0], index_v);
        }
        domi::GetContext().user_out_nodes.emplace_back(key_value_v[0], index);
      }
    }
  } catch (std::invalid_argument &) {
    GELOGE(PARAM_INVALID, "out nodes: %s, key value[1] is invalid argument", out_nodes.c_str());
    return PARAM_INVALID;
  } catch (std::out_of_range &) {
    GELOGE(PARAM_INVALID, "out nodes: %s, key value[1] is out of range", out_nodes.c_str());
    return PARAM_INVALID;
  } catch (...) {
    GELOGE(GE_GRAPH_PARAM_NULLPTR, "Invalid outNodes: %s", out_nodes.c_str());
    return GE_GRAPH_PARAM_NULLPTR;
  }
  return SUCCESS;
}
}  // namespace ge
