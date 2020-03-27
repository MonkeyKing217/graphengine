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

#ifndef GE_GRAPH_BUILD_GRAPH_BUILD_H_
#define GE_GRAPH_BUILD_GRAPH_BUILD_H_

#include <fstream>
#include <iostream>
#include <map>
#include <memory>
#include <string>
#include <vector>

#include "common/debug/log.h"
#include "common/debug/memory_dumper.h"
#include "common/properties_manager.h"
#include "common/string_util.h"
#include "common/types.h"
#include "common/util.h"
#include "graph/build/model_builder.h"
#include "graph/build/task_generator.h"
#include "graph/compute_graph.h"
#include "graph/graph.h"
#include "graph/manager/graph_manager_utils.h"
#include "graph/model.h"
#include "graph/node.h"
#include "graph/partition/graph_partition.h"
#include "graph/utils/graph_utils.h"
#include "graph/utils/tensor_utils.h"

namespace ge {
class GraphBuilder {
 public:
  GraphBuilder();
  GraphBuilder(const GraphBuilder &in) = delete;
  GraphBuilder &operator=(const GraphBuilder &in) = delete;
  virtual ~GraphBuilder() = default;
  Status Build(ComputeGraphPtr &comp_graph, std::vector<SubGraphInfoPtr> &subgraph_ptr_list,
               GeModelPtr &ge_model_ptr, uint64_t session_id = INVALID_SESSION_ID);
  void SetOptions(const GraphManagerOptions &options);

 private:
  Status CalcOpParam(const ge::ComputeGraphPtr &graph);
  Status GetTaskInfo(const ge::ModelBuilder &builder, const ModelPtr &model_ptr,
                     ComputeGraphPtr &comp_graph, std::vector<SubGraphInfoPtr> &subgraph_ptr_list,
                     uint64_t session_id = INVALID_SESSION_ID);
  Status SetInputSize(const ge::NodePtr &node_ptr);
  Status SecondPartition(ge::ComputeGraphPtr &comp_graph, vector<ge::SubGraphInfoPtr> &subgraph_ptr_list);

  int build_mode_;

  std::map<std::string, int> stream_max_parallel_num_;
  bool hcom_parallel_;

  GraphPartitioner graph_partitioner_;
};
}  // namespace ge
#endif  // GE_GRAPH_BUILD_GRAPH_BUILD_H_
