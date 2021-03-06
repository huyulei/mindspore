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

#include "src/ops/custom_extract_features.h"
#include "src/ops/primitive_c.h"
#include "src/ops/populate/populate_register.h"

namespace mindspore {
namespace lite {

OpParameter *PopulateExtractFeaturesParameter(const mindspore::lite::PrimitiveC *primitive) {
  OpParameter *param = reinterpret_cast<OpParameter *>(malloc(sizeof(OpParameter)));
  if (param == nullptr) {
    MS_LOG(ERROR) << "new OpParameter failed.";
    return nullptr;
  }
  memset(param, 0, sizeof(OpParameter));
  param->type_ = primitive->Type();
  return param;
}
Registry CustomExtractFeaturesParameterRegistry(schema::PrimitiveType_CustomExtractFeatures,
                                                PopulateExtractFeaturesParameter);

}  // namespace lite
}  // namespace mindspore
