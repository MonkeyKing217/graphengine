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

#ifndef GE_GRAPH_PASSES_MULTI_BATCH_CLONE_PASS_H_
#define GE_GRAPH_PASSES_MULTI_BATCH_CLONE_PASS_H_

#include <map>
#include <string>
#include <vector>

#include "inc/graph_pass.h"

namespace ge {
class MultiBatchClonePass : public GraphPass {
 public:
  Status Run(ComputeGraphPtr graph);

 private:
  ///
  /// @ingroup ge
  /// @brief Collect input output node from original graph.
  /// @param [in] const ComputeGraphPtr &graph: original graph.
  /// @return 0: SUCCESS / others: FAILED
  ///
  Status CollectIoNodes(const ComputeGraphPtr &graph);

  ///
  /// @ingroup ge
  /// @brief Create nodes for root graph.
  /// @param [in] const ComputeGraphPtr &graph: Root/Case graph.
  /// @return 0: SUCCESS / others: FAILED
  ///
  Status CreateRootGraph(const ComputeGraphPtr &graph);

  ///
  /// @ingroup ge
  /// @brief Create index data node for root graph.
  /// @param [in] const ComputeGraphPtr &graph: Root/Case graph.
  /// @param [in] NodePtr node: index data node.
  /// @return 0: SUCCESS / others: FAILED
  ///
  Status CreateIndexDataNode(const ComputeGraphPtr &graph, NodePtr &node);

  ///
  /// @ingroup ge
  /// @brief Create index const node for root graph.
  /// @param [in] const ComputeGraphPtr &graph: Root/Case graph.
  /// @param [in] NodePtr node: index const node.
  /// @return 0: SUCCESS / others: FAILED
  ///
  Status CreateIndexConstNode(const ComputeGraphPtr &graph, NodePtr &node);

  ///
  /// @ingroup ge
  /// @brief Create index node for root graph.
  /// @param [in] const ComputeGraphPtr &graph: Root/Case graph.
  /// @return 0: SUCCESS / others: FAILED
  ///
  Status CreateIndexNode(const ComputeGraphPtr &graph);

  ///
  /// @ingroup ge
  /// @brief Create input node for root graph.
  /// @param [in] const ComputeGraphPtr &graph: Root/Case graph.
  /// @return 0: SUCCESS / others: FAILED
  ///
  Status CreateInputNode(const ComputeGraphPtr &graph);

  ///
  /// @ingroup ge
  /// @brief Create Const node for root graph.
  /// @param [in] const ComputeGraphPtr &graph: Root/Case graph.
  /// @return 0: SUCCESS / others: FAILED
  ///
  Status CreateConstNode(const ComputeGraphPtr &graph);

  ///
  /// @ingroup ge
  /// @brief Create output node for root graph.
  /// @param [in] const ComputeGraphPtr &graph: Root/Case graph.
  /// @return 0: SUCCESS / others: FAILED
  ///
  Status CreateOutputNode(const ComputeGraphPtr &graph);

  ///
  /// @ingroup ge
  /// @brief Set max shape to Data node in root graph.
  /// @param [in] const NodePtr &data: data in Root/Case graph.
  /// @return 0: SUCCESS / others: FAILED
  ///
  Status SetMaxShapeToData(const NodePtr &data);

  ///
  /// @ingroup ge
  /// @brief Set shape to Data node in branch.
  /// @param [in] const NodePtr &data: data in branch.
  /// @param [in] const std::vector<int64_t> &shapes: dims of shape.
  /// @return 0: SUCCESS / others: FAILED
  ///
  Status UpdataShapeToData(const NodePtr &data, const std::vector<int64_t> &shapes);

  ///
  /// @ingroup ge
  /// @brief Set max shape to Data node in root graph.
  /// @param [in] const std::vector<int64_t> &shapes: dims of shape.
  /// @param [in] const NodePtr &data: data in Root/Case graph.
  /// @param [in] GeShape &data_shape: dims of data node.
  /// @return 0: SUCCESS / others: FAILED
  ///
  Status SetShapeToData(const std::vector<int64_t> &shapes, const NodePtr &data, GeShape &data_shape);

  ///
  /// @ingroup ge
  /// @brief Create nodes for root graph.
  /// @param [in] const ComputeGraphPtr &graph: Root/Case graph.
  /// @param [in] const ComputeGraphPtr &branch: original graph.
  /// @return 0: SUCCESS / others: FAILED
  ///
  Status CreateSubgraphs(const ComputeGraphPtr &graph, const ComputeGraphPtr &branch);

  ///
  /// @ingroup ge
  /// @brief Assign parent index for branches.
  /// @param [in] const ComputeGraphPtr &graph: Root/Case graph.
  /// @return 0: SUCCESS / others: FAILED
  ///
  Status PostProcSubgraph(const ComputeGraphPtr &graph);

  ///
  /// @ingroup ge
  /// @brief Remove subgraph supend output anchor.
  /// @param [in] ComputeGraphPtr &graph: Parent compute graph.
  /// @return 0: SUCCESS / others: FAILED
  ///
  Status PruneDirectOutput(const ComputeGraphPtr &graph);

  ///
  /// @ingroup ge
  /// @brief Update subgraph suspend output tensor.
  /// @param [in] parent_index: parent index for check.
  /// @param [in] unused_num: total unused tensor.
  /// @return 0: SUCCESS / others: FAILED
  ///
  Status UpdateOutputTensor(uint32_t parent_index, uint32_t unused_num);

  std::string session_graph_id_;
  std::vector<std::vector<int64_t>> batch_shapes_;

  std::vector<NodePtr> all_data_nodes_;
  std::vector<NodePtr> all_const_nodes_;
  std::vector<NodePtr> all_output_nodes_;

  std::map<uint32_t, std::string> direct_output_;
  std::map<ComputeGraphPtr, NodePtr> all_branch_output_;

  NodePtr case_node_;
};
}  // namespace ge
#endif  // GE_GRAPH_PASSES_MULTI_BATCH_CLONE_PASS_H_
