/**
 * Copyright 2019 Huawei Technologies Co., Ltd
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
#ifndef MINDSPORE_ANF_ACTIVATION_PARSER_H
#define MINDSPORE_ANF_ACTIVATION_PARSER_H
#include "src/common/anf_exporter/anf_populater/anf_node_populater.h"
#include <vector>
namespace mindspore::lite {
class AnfTensorAddPopulater : public AnfNodePopulater {
 public:
  AnfTensorAddPopulater() = default;
  ~AnfTensorAddPopulater() override = default;
  int Parse(CNodePtr cnodePtr, schema::CNodeT *node, std::vector<schema::TensorT *> *outputs) override;
};
}  // namespace mindspore::lite

#endif  // MINDSPORE_ANF_ACTIVATION_PARSER_H
