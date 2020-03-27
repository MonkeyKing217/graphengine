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

#ifndef INC_FRAMEWORK_COMMON_L2_CACHE_OPTIMIZE_H_
#define INC_FRAMEWORK_COMMON_L2_CACHE_OPTIMIZE_H_

#include <stdint.h>

#include <algorithm>
#include <functional>
#include <string>
#include <vector>

#include "common/types.h"
#include "common/util.h"
#include "graph/compute_graph.h"

namespace ge {
// Size of RC memory alignment, 2M
const size_t ALIGN_SIZE = 2097152;
const uint32_t RC_VALUE_DEFAULT = 1;
const uint32_t RC_VALUE_MAC = 32;

// RC data type classification
enum RCType {
  RC_DEFAULT,      // Such as temporary workspace memory of operator, variable (including global and local variable)
  RC_HCOM,         // Output of gradient aggregation, RC value should be set to 0
  RC_L2LOSS,       // Parameter of L2 loss operator, RC value should be set to 0
  RC_INPUTOUTPUT,  // Input and output tensor of operator, RC value is returned by FE calculation
  RC_WEIGHTS,      // The weight, fp16, RC value used by FP/BP operator should be set to 1 or the actual access numbers
  RC_DW,           // The gradient data DW and RC value output by BP operator
                   // should be set to 1 or the actual access numbers
  RC_ARGS          // Args of FlowTable, actual access numbers
};

enum MemType { INPUT_TENSOR, OUTPUT_TENSOR, WEIGHT, WORKSPACE };

// Memory usage information < node, type, number >
struct NodeInfo {
  string nodeName;
  MemType memType;
  size_t index;
};

// Memory block RC value
struct RCMemoryBlock {
  RCType type;        // RC type
  size_t blockSize;   // memory block size
  size_t headOffset;  // Start offset from base address
  size_t tailOffset;  // End offset from base address
  uint32_t rcCount;   // RC value
  NodeInfo nodeInfo;  // Input and output indexes of node objects to which RC belongs
};

// L2Cache optimizer
class L2CacheOptimize {
 public:
  explicit L2CacheOptimize(ge::ComputeGraphPtr &graph);
  ~L2CacheOptimize();

  // Collect the information L2Cache Memory optimization
  Status Gath();

 private:
  ge::ComputeGraphPtr graph_;

  // Save RC block information list
  vector<RCMemoryBlock> weightRCs;
  vector<RCMemoryBlock> opRCs;

  // Extract RC information generated by FE from compiled graph
  void RetirveRCinfo();

  // Take the maximum common divisor of RC values for the duplicate address
  void Merge(vector<RCMemoryBlock> &blocks);

  // The RC information is aligned with the 2m address
  void Align(vector<RCMemoryBlock> &blocks);

  // Weight of l2loss operator, output of gradient aggregation output, RC value set to 0
  void HandleOutputZeroRC(RCType type, ge::NodePtr node, vector<int64_t> &outputList, vector<RCMemoryBlock> &blocks);

  // Processing operator input Tensor's RC
  void HandOPInput(ge::NodePtr node, vector<int64_t> &inputList, vector<RCMemoryBlock> &blocks);

  // Processing operator output Tensor's RC
  void HandOPoutput(ge::NodePtr node, vector<int64_t> &outputList, vector<RCMemoryBlock> &blocks);

  // maximum common divisor
  uint32_t Measure(uint32_t x, uint32_t y) const {
    if (x == 0 || y == 0) return RC_VALUE_DEFAULT;
    uint32_t z = y;
    while (x % y != 0) {
      z = x % y;
      x = y;
      y = z;
    }
    return z;
  }

  bool Contain(const RCMemoryBlock &l_block, const RCMemoryBlock &r_block);
  bool Cross(const RCMemoryBlock &l_block, const RCMemoryBlock &r_block);
  bool Connect(const RCMemoryBlock &l_block, const RCMemoryBlock &r_block);
};
}  // namespace ge

#endif  // INC_FRAMEWORK_COMMON_L2_CACHE_OPTIMIZE_H_
