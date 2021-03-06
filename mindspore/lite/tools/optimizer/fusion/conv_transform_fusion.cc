/**
 * Copyright 2020 Huawei Technologies Co., Ltd
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *conv_activation_fusion.h
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "tools/optimizer/fusion/conv_transform_fusion.h"
#include <memory>
#include "src/ops/primitive_c.h"
#include "src/ops/conv2d.h"
#include "src/ops/deconv2d.h"
#include "src/ops/depthwise_conv2d.h"
#include "src/param_value_lite.h"
#include "schema/inner/model_generated.h"
#include "tools/optimizer/common/gllo_utils.h"
#include "securec/include/securec.h"

namespace mindspore::opt {
namespace {
constexpr size_t kConvWeightIndex = 2;
constexpr size_t kConvBiasIndex = 3;
constexpr size_t kConvNoBiasLen = 3;
constexpr size_t kConvWithBiasLen = 4;
int GetOutChannels(const CNodePtr &conv_node) {
  MS_ASSERT(conv_node != nullptr);
  auto value_primitive = conv_node->input(0);
  auto value_node = value_primitive->cast<ValueNodePtr>();
  MS_ASSERT(value_node != nullptr);
  auto value = value_node->value();
  MS_ASSERT(value != nullptr);
  auto primitive = value->cast<PrimitiveCPtr>();
  MS_ASSERT(primitive != nullptr);
  auto type = (schema::PrimitiveType)primitive->Type();

  if (type == schema::PrimitiveType_Conv2D) {
    MS_ASSERT(utils::isa<std::shared_ptr<mindspore::lite::Conv2D>>(primitive));
    auto primc = utils::cast<std::shared_ptr<mindspore::lite::Conv2D>>(primitive);
    MS_ASSERT(primc != nullptr);
    return primc->GetChannelOut();
  } else if (type == schema::PrimitiveType_DeConv2D) {
    MS_ASSERT(utils::isa<std::shared_ptr<mindspore::lite::DeConv2D>>(primitive));
    auto primc = utils::cast<std::shared_ptr<mindspore::lite::DeConv2D>>(primitive);
    MS_ASSERT(primc != nullptr);
    return primc->GetChannelOut();
  } else if (type == schema::PrimitiveType_DepthwiseConv2D) {
    MS_ASSERT(utils::isa<std::shared_ptr<mindspore::lite::DepthwiseConv2D>>(primitive));
    auto primc = utils::cast<std::shared_ptr<mindspore::lite::DepthwiseConv2D>>(primitive);
    MS_ASSERT(primc != nullptr);
    return primc->GetChannelMultiplier() * primc->GetChannelIn();
  } else {
    MS_LOG(ERROR) << "Unsupported opType, " << type;
    return 0;
  }
}
}  // namespace

const AnfNodePtr ConvTransformFusion::Process(const FuncGraphPtr &func_graph, const AnfNodePtr &node,
                                              const EquivPtr &) const {
  MS_LOG(DEBUG) << "conv activation pass process";
  if (CheckIfFuncGraphIsNull(func_graph) != lite::RET_OK || CheckIfAnfNodeIsNull(node) != lite::RET_OK) {
    return nullptr;
  }
  // transform node means scale,bn
  auto transform_node = node->cast<CNodePtr>();
  if (CheckIfCNodeIsNull(transform_node) != lite::RET_OK || CheckLeastInputSize(transform_node, 2) != lite::RET_OK) {
    return nullptr;
  }

  auto pre_node = transform_node->input(1);
  auto conv_node = pre_node->cast<CNodePtr>();
  if (IsMultiOutputTensors(func_graph, conv_node)) {
    return nullptr;
  }

  auto abstr = transform_node->abstract();
  int kernel_nums = GetOutChannels(conv_node);
  if (kernel_nums <= 0) {
    MS_LOG(INFO) << "Unsupported conv node, " << conv_node->DebugString();
    return node;
  }
  auto trans_scale = new (std::nothrow) float[kernel_nums];
  if (trans_scale == nullptr) {
    MS_LOG(ERROR) << "tensor_data is nullptr";
    return nullptr;
  }
  auto trans_bias = new (std::nothrow) float[kernel_nums];
  if (trans_bias == nullptr) {
    MS_LOG(ERROR) << "tensor_data is nullptr";
    delete[] trans_scale;
    return nullptr;
  }
  GenTransParam(transform_node, kernel_nums, trans_scale, trans_bias);
  GenNewConvTensor(func_graph, conv_node, kernel_nums, trans_scale, trans_bias);
  delete[] trans_bias;
  delete[] trans_scale;
  pre_node->set_abstract(abstr);
  return pre_node;
}

void ConvTransformFusion::GenTransParam(const CNodePtr &transform_node, int kernel_nums, float *trans_scale,
                                        float *trans_bias) const {
  if (trans_scale == nullptr) {
    MS_LOG(ERROR) << "new transScale failed";
    lite::ReturnCode::GetSingleReturnCode()->UpdateReturnCode(lite::RET_NULL_PTR);
    return;
  }
  if (trans_bias == nullptr) {
    MS_LOG(ERROR) << "new transBias failed";
    lite::ReturnCode::GetSingleReturnCode()->UpdateReturnCode(lite::RET_NULL_PTR);
    return;
  }
  if (0 != memset_s(trans_scale, kernel_nums * sizeof(float), 0, kernel_nums * sizeof(float))) {
    MS_LOG(ERROR) << "memset transScale failed";
    lite::ReturnCode::GetSingleReturnCode()->UpdateReturnCode(lite::RET_MEMORY_FAILED);
    return;
  }
  if (0 != memset_s(trans_bias, kernel_nums * sizeof(float), 0, kernel_nums * sizeof(float))) {
    MS_LOG(ERROR) << "memset transBias failed";
    lite::ReturnCode::GetSingleReturnCode()->UpdateReturnCode(lite::RET_MEMORY_FAILED);
    return;
  }

  InitTransParam(transform_node, kernel_nums, trans_scale, trans_bias);
}

void ConvTransformFusion::GenNewConvTensor(const FuncGraphPtr &func_graph, const CNodePtr &conv_node, int kernel_num,
                                           const float *trans_scale, const float *trans_bias) const {
  MS_ASSERT(conv_node != nullptr);
  AnfNodePtr conv_weight_node = nullptr;
  AnfNodePtr conv_bias_node = nullptr;
  if (conv_node->inputs().size() == kConvNoBiasLen) {
    conv_weight_node = conv_node->input(kConvWeightIndex);
  } else if (conv_node->inputs().size() == kConvWithBiasLen) {
    conv_weight_node = conv_node->input(kConvWeightIndex);
    conv_bias_node = conv_node->input(kConvBiasIndex);
  } else {
    MS_LOG(ERROR) << "conv node:" << conv_node->DebugString() << "inputs size must 3 or 4";
    return;
  }
  if (!conv_weight_node->isa<Parameter>()) {
    MS_LOG(ERROR) << "scale weight node not parameter node";
    lite::ReturnCode::GetSingleReturnCode()->UpdateReturnCode(lite::RET_INVALID_OP_ATTR);
    return;
  }
  if (conv_bias_node != nullptr && !conv_bias_node->isa<Parameter>()) {
    MS_LOG(ERROR) << "scale bias node not parameter node";
    lite::ReturnCode::GetSingleReturnCode()->UpdateReturnCode(lite::RET_INVALID_OP_ATTR);
    return;
  }
  auto conv_weight_param = conv_weight_node->cast<ParameterPtr>()->default_param();
  auto weight_tensor = std::dynamic_pointer_cast<ParamValueLite>(conv_weight_param);
  if (kernel_num <= 0) {
    MS_LOG(ERROR) << "kernel num less than 0";
    lite::ReturnCode::GetSingleReturnCode()->UpdateReturnCode(lite::RET_INVALID_OP_ATTR);
    return;
  }
  CalNewWeightTensor(conv_node, weight_tensor, kernel_num, trans_scale);
  float *bias_data = nullptr;
  // conv has bias,bias_flag true
  bool bias_flag = false;
  if (conv_bias_node != nullptr) {
    auto conv_bias_param = conv_bias_node->cast<ParameterPtr>()->default_param();
    auto bias_tensor = std::dynamic_pointer_cast<ParamValueLite>(conv_bias_param);
    bias_data = reinterpret_cast<float *>(bias_tensor->tensor_addr());
    bias_flag = true;
  } else {
    bias_data = new (std::nothrow) float[kernel_num];
    if (bias_data == nullptr) {
      MS_LOG(ERROR) << "tensor_data is nullptr";
      return;
    }
  }
  CalNewBiasTensor(bias_data, kernel_num, bias_flag, trans_scale, trans_bias);
  if (!bias_flag) {
    auto bias_node = AddNewBiasNode(bias_data, func_graph, kernel_num, weight_tensor);
    bias_node->set_name(conv_node->fullname_with_scope() + "_bias");
    conv_node->add_input(bias_node);
  }
}
void ConvTransformFusion::CalNewWeightTensor(const CNodePtr &conv_node, const ParamValueLitePtr &weight_tensor,
                                             int kernel_num, const float *trans_scale) const {
  MS_ASSERT(weight_data != nullptr);
  MS_ASSERT(trans_scale != nullptr);
  auto weight_shape_size = weight_tensor->tensor_shape_size();
  auto tmp_weight_data = new (std::nothrow) float[weight_shape_size];
  if (tmp_weight_data == nullptr) {
    lite::ReturnCode::GetSingleReturnCode()->UpdateReturnCode(lite::RET_MEMORY_FAILED);
    return;
  }
  MS_ASSERT(new_weight_data != nullptr);
  auto data_size = weight_shape_size * sizeof(float);
  if (0 != memset_s(tmp_weight_data, data_size, 0, data_size)) {
    MS_LOG(ERROR) << "memset newWeightData failed";
    delete[] tmp_weight_data;
    lite::ReturnCode::GetSingleReturnCode()->UpdateReturnCode(lite::RET_MEMORY_FAILED);
    return;
  }
  auto weight_data = reinterpret_cast<float *>(weight_tensor->tensor_addr());
  auto conv_type = GetCNodeType(conv_node);
  if (conv_type == schema::PrimitiveType_DeConv2D) {
    auto value_node = conv_node->input(0)->cast<ValueNodePtr>();
    MS_ASSERT(value_node != nullptr);
    auto value = value_node->value();
    MS_ASSERT(value != nullptr);
    auto primitive = value->cast<PrimitivePtr>();
    MS_ASSERT(utils::isa<std::shared_ptr<mindspore::lite::DeConv2D>>(primitive));
    auto primc = utils::cast<std::shared_ptr<mindspore::lite::DeConv2D>>(primitive);
    MS_ASSERT(primc != nullptr);
    if (weight_tensor->tensor_shape().size() != 4) {
      MS_LOG(ERROR) << "deconv2d weight tensor shape error";
      delete[] tmp_weight_data;
      return;
    }
    auto group = primc->GetGroup();
    auto cin_group = weight_tensor->tensor_shape()[0] / group;
    int area_size = weight_tensor->tensor_shape()[2] * weight_tensor->tensor_shape()[3];
    int cout_size = kernel_num * area_size;
    for (int k = 0; k < cin_group; ++k) {
      for (int i = 0; i < kernel_num; ++i) {
        auto row_addr = weight_data + k * cout_size + i * area_size;
        auto new_row_addr = tmp_weight_data + k * cout_size + i * area_size;
        for (int j = 0; j < area_size; j++) {
          new_row_addr[j] = row_addr[j] * trans_scale[i];
        }
      }
    }
  } else {
    if (this->fmk_type_ == lite::converter::FmkType_TF) {
      for (int i = 0; i < weight_shape_size; i++) {
        tmp_weight_data[i] = weight_data[i] * trans_scale[i % kernel_num];
      }
    } else {
      auto kernel_size = weight_shape_size / kernel_num;
      for (int i = 0; i < kernel_num; i++) {
        for (int j = 0; j < kernel_size; j++) {
          tmp_weight_data[i * kernel_size + j] = weight_data[i * kernel_size + j] * trans_scale[i];
        }
      }
    }
  }

  auto ret = memcpy_s(weight_data, data_size, tmp_weight_data, data_size);
  if (ret != EOK) {
    MS_LOG(ERROR) << "memcpy error: " << ret;
    lite::ReturnCode::GetSingleReturnCode()->UpdateReturnCode(lite::RET_MEMORY_FAILED);
    delete[] tmp_weight_data;
    return;
  }
  delete[] tmp_weight_data;
}

void ConvTransformFusion::CalNewBiasTensor(float *bias_data, int kernel_num, bool bias_flag, const float *trans_scale,
                                           const float *trans_bias) const {
  MS_ASSERT(bias_data != nullptr);
  MS_ASSERT(trans_bias != nullptr);
  MS_ASSERT(trans_scale != nullptr);
  if (bias_flag) {
    auto tmp_bias_data = new (std::nothrow) float[kernel_num];
    if (tmp_bias_data == nullptr) {
      MS_LOG(ERROR) << "tensor_data is nullptr";
      return;
    }
    if (EOK != memset_s(tmp_bias_data, kernel_num * sizeof(float), 0, kernel_num * sizeof(float))) {
      MS_LOG(ERROR) << "memset bias data failed";
      lite::ReturnCode::GetSingleReturnCode()->UpdateReturnCode(lite::RET_MEMORY_FAILED);
      delete[] tmp_bias_data;
      return;
    }
    for (int i = 0; i < kernel_num; i++) {
      tmp_bias_data[i] = bias_data[i] * trans_scale[i] + trans_bias[i];
    }

    auto ret = memcpy_s(bias_data, kernel_num * sizeof(float), tmp_bias_data, kernel_num * sizeof(float));
    delete[] tmp_bias_data;
    if (ret != EOK) {
      MS_LOG(ERROR) << "memcpy error: " << ret;
      lite::ReturnCode::GetSingleReturnCode()->UpdateReturnCode(lite::RET_MEMORY_FAILED);
      return;
    }
  } else {
    if (EOK != memset_s(bias_data, kernel_num * sizeof(float), 0, kernel_num * sizeof(float))) {
      MS_LOG(ERROR) << "memset bias data failed";
      lite::ReturnCode::GetSingleReturnCode()->UpdateReturnCode(lite::RET_MEMORY_FAILED);
      return;
    }
    auto ret = memcpy_s(bias_data, kernel_num * sizeof(float), trans_bias, kernel_num * sizeof(float));
    if (ret != EOK) {
      MS_LOG(ERROR) << "memcpy error: " << ret;
      lite::ReturnCode::GetSingleReturnCode()->UpdateReturnCode(lite::RET_MEMORY_FAILED);
    }
  }
}
}  // namespace mindspore::opt
