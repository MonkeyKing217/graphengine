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

#ifndef GE_GRAPH_LOAD_NEW_MODEL_MANAGER_TASK_INFO_TASK_INFO_H_
#define GE_GRAPH_LOAD_NEW_MODEL_MANAGER_TASK_INFO_TASK_INFO_H_

#include <vector>

#include "cce/customize.h"
#include "cce/taskdown_common.hpp"
#include "framework/common/ge_inner_error_codes.h"
#include "graph/load/new_model_manager/task_info/task_info_factory.h"
#include "proto/task.pb.h"
namespace ge {
struct RuntimeParam {
  uint64_t mem_size = 0;
  uint64_t logic_mem_base = 0;
  uint8_t *mem_base = nullptr;
  uint64_t weight_size = 0;
  uint64_t logic_weight_base = 0;
  uint8_t *weight_base = nullptr;
  uint64_t var_size = 0;
  uint64_t logic_var_base = 0;
  uint8_t *var_base = nullptr;
  uint32_t batch_num = 0;
  uint32_t stream_num = 0;
  uint32_t event_num = 0;
  uint64_t session_id = 0;
  uint32_t graph_id = 0;
};

class DavinciModel;

class TaskInfo {
 public:
  TaskInfo() : stream_(nullptr) {}

  virtual ~TaskInfo() { stream_ = nullptr; }

  virtual Status Init(const domi::TaskDef &task_def, DavinciModel *davinci_model) = 0;

  virtual Status Distribute() = 0;

  virtual Status Release() { return SUCCESS; }

  virtual cce::ccOpContext *GetCtx() { return nullptr; }

  virtual uint32_t GetTaskID() { return 0xFFFFFFFF; }

  virtual uintptr_t GetDumpArgs() { return 0; }

 protected:
  Status SetStream(uint32_t stream_id, const std::vector<rtStream_t> &stream_list);

  void *stream_;
};
}  // namespace ge
#endif  // GE_GRAPH_LOAD_NEW_MODEL_MANAGER_TASK_INFO_TASK_INFO_H_
