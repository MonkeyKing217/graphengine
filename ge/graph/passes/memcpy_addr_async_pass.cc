/**
 * Copyright 2020 Huawei Technologies Co., Ltd
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

#include "graph/passes/memcpy_addr_async_pass.h"

#include "common/ge/ge_util.h"
#include "framework/common/debug/log.h"
#include "graph/utils/node_utils.h"
#include "graph/utils/op_desc_utils.h"
#include "graph/utils/tensor_utils.h"

namespace ge {
Status MemcpyAddrAsyncPass::Run(ComputeGraphPtr graph) {
  GE_CHECK_NOTNULL(graph);
  for (auto &node : graph->GetAllNodes()) {
    auto op_desc = node->GetOpDesc();
    GE_IF_BOOL_EXEC(op_desc == nullptr, continue);

    if (op_desc->GetType() == STREAMSWITCHN || op_desc->GetType() == STREAMMERGE) {
      Status ret = AddMemcpyAddrAsyncNode(graph, node);
      if (ret != SUCCESS) {
        GELOGE(ret, "AddMemcpyAddrAsyncNode failed.");
        return ret;
      }
    }
    // handle data->netoutput, const->netoutput in root graph, use mem_addr_async to improve performance
    if (op_desc->GetType() == NETOUTPUT) {
      // check this netoutput is on root graph
      if (node->GetOwnerComputeGraph()->GetParentNode() == nullptr) {
        Status ret = InsertMemAddrAsyncNodeBeforeNetoutput(node->GetOwnerComputeGraph(), node);
        if (ret != SUCCESS) {
          GELOGE(ret, "AddMemcpyAddrAsyncNode failed.");
          return ret;
        }
      }
    }
  }
  return SUCCESS;
}

Status MemcpyAddrAsyncPass::AddMemcpyAddrAsyncNode(const ComputeGraphPtr &graph, const NodePtr &node) {
  GELOGI("Start AddMemcpyAddrAsyncNode for %s.", node->GetName().c_str());
  for (InDataAnchorPtr &in_data_anchor : node->GetAllInDataAnchors()) {
    OutDataAnchorPtr peer_out_anchor = in_data_anchor->GetPeerOutAnchor();
    GE_IF_BOOL_EXEC(peer_out_anchor == nullptr, continue);
    NodePtr in_node = peer_out_anchor->GetOwnerNode();

    if (in_node->GetType() == DATA) {
      ComputeGraphPtr owner_graph = in_node->GetOwnerComputeGraph();
      GE_CHECK_NOTNULL(owner_graph);
      // Data is in parent_graph
      if (owner_graph->GetParentGraph() == nullptr) {
        GELOGI("Need to insert MemcpyAddrAsync directly when data in parent graph.");
        NodePtr memcpy_addr_async_node = CreateMemcpyAddrAsyncNode(graph, peer_out_anchor, node);
        GE_IF_BOOL_EXEC(memcpy_addr_async_node == nullptr, GELOGE(INTERNAL_ERROR, "CreateMemcpyAddrAsyncNode failed.");
                        return INTERNAL_ERROR);

        Status ret = InsertMemcpyAddrAsyncNode(peer_out_anchor, in_data_anchor, memcpy_addr_async_node);
        GE_IF_BOOL_EXEC(ret != SUCCESS, GELOGE(ret, "InsertMemcpyAddrAsyncNode failed."); return ret);
      } else {
        uint32_t parent_index = 0;
        if (!AttrUtils::GetInt(in_node->GetOpDesc(), ATTR_NAME_PARENT_NODE_INDEX, parent_index)) {
          GELOGE(INTERNAL_ERROR, "Failed to get parent index of %s", in_node->GetName().c_str());
          return INTERNAL_ERROR;
        }
        // Data is in sub_graph
        GELOGI("Need to find data in parent graph, then insert MemcpyAddrAsync.");
        NodePtr parent_node = owner_graph->GetParentNode();
        user_data_for_known_ = in_node;
        out_of_user_data_for_known_ = node;
        peer_out_anchor_for_known_ = peer_out_anchor;
        in_anchor_for_known_ = in_data_anchor;
        FindUserData(parent_node, parent_index);
        if (find_user_data_) {
          GELOGI("Insert memcpy_addr_async for non_dynamic.");
          GE_CHECK_NOTNULL(peer_out_anchor_);
          NodePtr memcpy_addr_async_node = CreateMemcpyAddrAsyncNode(graph, peer_out_anchor_, out_of_user_data_);
          GE_IF_BOOL_EXEC(memcpy_addr_async_node == nullptr,
                          GELOGE(INTERNAL_ERROR, "CreateMemcpyAddrAsyncNode failed.");
                          return INTERNAL_ERROR);

          Status ret = InsertMemcpyAddrAsyncNode(peer_out_anchor_, in_anchor_, memcpy_addr_async_node);
          GE_IF_BOOL_EXEC(ret != SUCCESS, GELOGE(ret, "InsertMemcpyAddrAsyncNode failed."); return ret);
        }
        if (find_user_data_for_known_) {
          GELOGI("Insert memcpy_addr_async for known graph.");
          auto sub_graph = user_data_for_known_->GetOwnerComputeGraph();
          NodePtr memcpy_addr_async_node =
              CreateMemcpyAddrAsyncNode(sub_graph, peer_out_anchor_for_known_, out_of_user_data_for_known_);
          GE_IF_BOOL_EXEC(memcpy_addr_async_node == nullptr,
                          GELOGE(INTERNAL_ERROR, "CreateMemcpyAddrAsyncNode for known failed.");
                          return INTERNAL_ERROR);

          Status ret =
              InsertMemcpyAddrAsyncNode(peer_out_anchor_for_known_, in_anchor_for_known_, memcpy_addr_async_node);
          GE_IF_BOOL_EXEC(ret != SUCCESS, GELOGE(ret, "InsertMemcpyAddrAsyncNode for known failed."); return ret);
        }
      }
    }
  }
  return SUCCESS;
}

void MemcpyAddrAsyncPass::FindUserDataForKnown(const NodePtr &parent_node, uint32_t &parent_index) {
  GELOGI("Start FindUserDataForKnown of %s.", parent_node->GetName().c_str());
  if (user_data_for_known_->GetOpDesc() == nullptr) {
    GELOGI("Cannot get op_desc of %s.", user_data_for_known_->GetName().c_str());
    return;
  }
  string src_var_name;
  if (ge::AttrUtils::GetStr(user_data_for_known_->GetOpDesc(), REF_VAR_SRC_VAR_NAME, src_var_name)) {
    GELOGI("The data in known graph is variable, no need to insert memcpy_addr_async.");
    find_user_data_for_known_ = false;
    return;
  } else {
    find_user_data_for_known_ = true;
  }
}

void MemcpyAddrAsyncPass::FindUserDataForNonDynamic(const ge::NodePtr &parent_node, uint32_t &parent_index) {
  GELOGI("Start to FindUserDataForNonDynamic of %s.", parent_node->GetName().c_str());
  InDataAnchorPtr in_data_anchor = parent_node->GetInDataAnchor(parent_index);
  OutDataAnchorPtr out_anchor = in_data_anchor->GetPeerOutAnchor();
  GE_IF_BOOL_EXEC(out_anchor == nullptr,
                  GELOGE(INTERNAL_ERROR, "Cannot find out_anchor of %s.", parent_node->GetName().c_str());
                  return);
  NodePtr in_node = out_anchor->GetOwnerNode();
  GELOGI("in_node of parent_node is %s.", in_node->GetName().c_str());
  if (in_node->GetType() == DATA) {
    if (in_node->GetOwnerComputeGraph()->GetParentGraph() != nullptr) {
      // DATA is in sub graph again, update user_data of known firstly
      user_data_for_known_ = in_node;
      out_of_user_data_for_known_ = parent_node;
      peer_out_anchor_for_known_ = out_anchor;
      in_anchor_for_known_ = in_data_anchor;
      NodePtr pre_in_node = in_node->GetOwnerComputeGraph()->GetParentNode();
      if (!AttrUtils::GetInt(in_node->GetOpDesc(), ATTR_NAME_PARENT_NODE_INDEX, parent_index)) {
        GELOGE(INTERNAL_ERROR, "Failed to refresh parent index of %s", in_node->GetName().c_str());
        return;
      }
      FindUserData(pre_in_node, parent_index);
    } else {
      // DATA is in parent graph and not has input
      user_data_ = in_node;
      out_of_user_data_ = parent_node;
      peer_out_anchor_ = out_anchor;
      in_anchor_ = in_data_anchor;
      find_user_data_ = true;
      GELOGI("%s connect with %s, will insert memcpyaddr.", user_data_->GetName().c_str(),
             out_of_user_data_->GetName().c_str());
    }
  } else if (in_node->GetType() == IF || in_node->GetType() == WHILE || in_node->GetType() == CASE) {
    if (!AttrUtils::GetInt(parent_node->GetOpDesc(), ATTR_NAME_PARENT_NODE_INDEX, parent_index)) {
      GELOGE(INTERNAL_ERROR, "Failed to refresh parent index of %s", in_node->GetName().c_str());
      return;
    }
    FindUserData(in_node, parent_index);
  } else {
    GELOGI("%s connect with %s, which is not user_data.", parent_node->GetName().c_str(), in_node->GetName().c_str());
    find_user_data_ = false;
  }
}

void MemcpyAddrAsyncPass::FindUserData(const NodePtr &parent_node, uint32_t &parent_index) {
  auto parent_op_desc = parent_node->GetOpDesc();
  if (parent_op_desc == nullptr) {
    GELOGI("Cannot get op_desc of %s.", parent_node->GetName().c_str());
    return;
  }
  bool is_unknown_shape = false;
  if (parent_node->GetType() == PARTITIONEDCALL &&
      AttrUtils::GetBool(parent_op_desc, ATTR_NAME_IS_UNKNOWN_SHAPE, is_unknown_shape) && !is_unknown_shape) {
    FindUserDataForKnown(parent_node, parent_index);
  } else {
    FindUserDataForNonDynamic(parent_node, parent_index);
  }
}

NodePtr MemcpyAddrAsyncPass::CreateMemcpyAddrAsyncNode(const ComputeGraphPtr &graph,
                                                       const OutDataAnchorPtr &out_data_anchor,
                                                       const NodePtr &out_of_user_data) {
  GELOGD("Start CreateMemcpyAddrAsyncNode.");
  OpDescPtr pre_op_desc = out_data_anchor->GetOwnerNode()->GetOpDesc();
  GE_CHK_BOOL_EXEC(pre_op_desc != nullptr, return nullptr, "Op_desc of pre node is invalid.");
  std::string node_name = pre_op_desc->GetName() + "_" + MEMCPYADDRASYNC;

  OpDescPtr op_desc = MakeShared<OpDesc>(node_name, MEMCPYADDRASYNC);
  GE_CHECK_NOTNULL_EXEC(op_desc, return nullptr);

  if (op_desc->AddInputDesc(pre_op_desc->GetOutputDesc(out_data_anchor->GetIdx())) != GRAPH_SUCCESS) {
    GELOGE(INTERNAL_ERROR, "Add memcpy_addr_async input desc failed.");
    return nullptr;
  }

  if (op_desc->AddOutputDesc(pre_op_desc->GetOutputDesc(out_data_anchor->GetIdx())) != GRAPH_SUCCESS) {
    GELOGE(INTERNAL_ERROR, "Add memcpy_addr_async output desc failed.");
    return nullptr;
  }

  int64_t stream_id = out_of_user_data->GetOpDesc()->GetStreamId();
  op_desc->SetStreamId(stream_id);
  GELOGI("SetStreamId: Node %s assign stream is %ld.", op_desc->GetName().c_str(), stream_id);
  bool labeled_input = false;
  (void)ge::AttrUtils::GetBool(out_of_user_data->GetOpDesc(), ATTR_NAME_NODE_CONNECT_INPUT, labeled_input);
  if (labeled_input) {
    if (!ge::AttrUtils::SetBool(out_of_user_data->GetOpDesc(), ATTR_NAME_NODE_CONNECT_INPUT, false)) {
      GELOGE(FAILED, "Failed to unset attr %s for node %s.", ATTR_NAME_NODE_CONNECT_INPUT.c_str(),
             out_of_user_data->GetName().c_str());
      return nullptr;
    }
    if (!ge::AttrUtils::SetBool(op_desc, ATTR_NAME_NODE_CONNECT_INPUT, true)) {
      GELOGE(FAILED, "Failed to set attr %s for node %s.", ATTR_NAME_NODE_CONNECT_INPUT.c_str(),
             op_desc->GetName().c_str());
      return nullptr;
    }
  }

  NodePtr memcpy_addr_async_node = graph->AddNodeAfter(op_desc, out_data_anchor->GetOwnerNode());
  GE_CHECK_NOTNULL_EXEC(memcpy_addr_async_node, return nullptr);

  return memcpy_addr_async_node;
}

Status MemcpyAddrAsyncPass::InsertMemcpyAddrAsyncNode(const OutDataAnchorPtr &out_anchor,
                                                      const InDataAnchorPtr &in_anchor, const NodePtr &node) {
  // insert memcpy_addr of each user_data and out_of_user_data
  if (GraphUtils::RemoveEdge(out_anchor, in_anchor) != GRAPH_SUCCESS) {
    GELOGE(INTERNAL_ERROR, "Remove edge of %s and %s failed.", out_anchor->GetOwnerNode()->GetName().c_str(),
           in_anchor->GetOwnerNode()->GetName().c_str());
    return INTERNAL_ERROR;
  }
  if (GraphUtils::AddEdge(out_anchor, node->GetInDataAnchor(0)) != GRAPH_SUCCESS) {
    GELOGE(INTERNAL_ERROR, "Add edge of %s and %s failed.", out_anchor->GetOwnerNode()->GetName().c_str(),
           node->GetName().c_str());
    return INTERNAL_ERROR;
  }
  if (GraphUtils::AddEdge(node->GetOutDataAnchor(0), in_anchor) != GRAPH_SUCCESS) {
    GELOGE(INTERNAL_ERROR, "Add edge of %s and %s failed.", node->GetName().c_str(),
           in_anchor->GetOwnerNode()->GetName().c_str());
    return INTERNAL_ERROR;
  }
  return SUCCESS;
}

Status MemcpyAddrAsyncPass::InsertMemAddrAsyncNodeBeforeNetoutput(const ComputeGraphPtr &graph, const NodePtr &node) {
  GELOGI("Start AddMemcpyAddrAsyncNode for %s.", node->GetName().c_str());
  for (const auto &in_data_anchor : node->GetAllInDataAnchors()) {
    auto in_node = NodeUtils::GetInDataNodeByIndex(*node, in_data_anchor->GetIdx());
    GE_CHECK_NOTNULL(in_node);
    auto peer_out_anchor = in_data_anchor->GetPeerOutAnchor();
    if ((in_node->GetType() != CONSTANT) &&
       (in_node->GetType() != CONSTANTOP) &&
       (in_node->GetType() != DATA)) {
      continue;
    }
    auto desc = in_node->GetOpDesc();
    GE_CHECK_NOTNULL(desc);
    if (IsEmptyTenor(desc->GetOutputDesc(peer_out_anchor->GetIdx()).GetShape())) {
      continue;
    }
    GELOGI("Need to insert MemcpyAddrAsync before netoutput on parent graph.");
    NodePtr memcpy_addr_async_node = CreateMemcpyAddrAsyncNode(graph, peer_out_anchor, in_node);
    GE_IF_BOOL_EXEC(memcpy_addr_async_node == nullptr, GELOGE(INTERNAL_ERROR, "CreateMemcpyAddrAsyncNode failed.");
                    return INTERNAL_ERROR);

    Status ret = InsertMemcpyAddrAsyncNode(peer_out_anchor, in_data_anchor, memcpy_addr_async_node);
    GE_IF_BOOL_EXEC(ret != SUCCESS, GELOGE(ret, "InsertMemcpyAddrAsyncNode failed."); return ret);
    GELOGI("Insert mem_addr_async node %s success between %s and %s.", memcpy_addr_async_node->GetName().c_str(),
           in_node->GetName().c_str(), node->GetName().c_str());
    // if src node is const, need to update attr and offset here because this pass process is after offset set.
    if ((in_node->GetType() == CONSTANT) || (in_node->GetType() == CONSTANTOP)) {
      NodeUtils::UpdateIsInputConst(memcpy_addr_async_node);
      auto output_desc = node->GetOpDesc();
      GE_CHECK_NOTNULL(output_desc);
      auto output_tensor_desc = output_desc->MutableInputDesc(static_cast<uint32_t>(in_data_anchor->GetIdx()));
      int64_t data_offset = 0;
      (void)TensorUtils::GetDataOffset(*output_tensor_desc, data_offset);
      auto input_tensor = memcpy_addr_async_node->GetOpDesc()->MutableInputDesc(0);
      GELOGI("Need update const Offset %ld to op [%s]", data_offset, memcpy_addr_async_node->GetName().c_str());
      TensorUtils::SetDataOffset(*input_tensor, data_offset);
      TensorUtils::SetDataOffset(*output_tensor_desc, 0);
    }
  }
  NodeUtils::UpdateIsInputConst(node);
  return SUCCESS;
}

bool MemcpyAddrAsyncPass::IsEmptyTenor(const GeShape &shape) const {
  for (const auto dim : shape.GetDims()) {
    if (dim == 0) {
      return true;
    }
  }
  return false;
}
}  // namespace ge
