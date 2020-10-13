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

#include "graph/common/local_context.h"

#include "common/ge_inner_error_codes.h"
#include "common/debug/ge_log.h"
#include "omg/omg_inner_types.h"

namespace ge {
namespace {
thread_local OmgContext *omg_context = nullptr;
}

void SetLocalOmgContext(OmgContext &context) {
  omg_context = &context;
}

OmgContext &GetLocalOmgContext() {
  if (omg_context != nullptr) {
    return *omg_context;
  } else {
    GELOGW("omg_context is nullptr.");
    return domi::GetContext();
  }
}
}
