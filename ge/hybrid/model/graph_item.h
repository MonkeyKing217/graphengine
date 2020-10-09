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

#ifndef GE_HYBRID_MODEL_SUBGRAPH_ITEM_H_
#define GE_HYBRID_MODEL_SUBGRAPH_ITEM_H_

#include "external/ge/ge_api_error_codes.h"
#include "hybrid/model/node_item.h"

namespace ge {
namespace hybrid {
class GraphItem {
 public:
  GraphItem() = default;
  ~GraphItem();
  const vector<NodeItem *> &GetAllNodes() const;
  const vector<const NodeItem *> &GetInputNodes() const;
  Status GetOutputDescList(std::vector<ConstGeTensorDescPtr> &output_desc_list) const;

  int TotalInputs() const {
    return total_inputs_;
  }

  int TotalOutputs() const {
    return total_outputs_;
  }

  const std::string& GetName() const {
    return name_;
  }

  void SetName(const string &name) {
    name_ = name;
  }

  const NodeItem *GetOutputNode() const;

  bool IsDynamic() const;
  int GetParentOutputIndex(size_t index) const;
  const vector<int> &GetInputIndexMapping() const;

 private:
  friend class HybridModelBuilder;
  std::string name_;
  std::vector<NodeItem *> node_items_;
  std::vector<const NodeItem *> input_nodes_;
  const NodeItem *output_node_ = nullptr;
  // <src_node, out_index>
  std::vector<std::pair<const NodeItem *, int>> output_edges_;
  int total_inputs_ = 0;
  int total_outputs_ = 0;

  bool is_dynamic_ = true;
  std::vector<int> input_index_mapping_;
  std::vector<int> output_index_mapping_;
};
}  // namespace hybrid
}  // namespace ge
#endif // GE_HYBRID_MODEL_SUBGRAPH_ITEM_H_
