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

#ifndef GE_GRAPH_LOAD_NEW_MODEL_MANAGER_TASK_INFO_STREAM_ACTIVE_TASK_INFO_H_
#define GE_GRAPH_LOAD_NEW_MODEL_MANAGER_TASK_INFO_STREAM_ACTIVE_TASK_INFO_H_
#include "graph/load/new_model_manager/task_info/task_info.h"

namespace ge {
class StreamActiveTaskInfo : public TaskInfo {
 public:
  StreamActiveTaskInfo() : active_stream_(nullptr), active_stream_id_(0) {}

  ~StreamActiveTaskInfo() override { active_stream_ = nullptr; }

  Status Init(const domi::TaskDef &task_def, DavinciModel *davinci_model) override;

  Status Distribute() override;

 private:
  rtStream_t active_stream_;
  uint32_t active_stream_id_;
};
}  // namespace ge
#endif  // GE_GRAPH_LOAD_NEW_MODEL_MANAGER_TASK_INFO_STREAM_ACTIVE_TASK_INFO_H_
