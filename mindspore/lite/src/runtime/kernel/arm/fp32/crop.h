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
#ifndef MINDSPORE_LITE_SRC_RUNTIME_KERNEL_ARM_FP32_CROP_H_
#define MINDSPORE_LITE_SRC_RUNTIME_KERNEL_ARM_FP32_CROP_H_

#include <vector>
#include "src/lite_kernel.h"

#include "src/runtime/kernel/arm/base/layout_transform.h"

namespace mindspore::kernel {
class CropCPUKernel : public LiteKernel {
 public:
  CropCPUKernel(OpParameter *parameter, const std::vector<lite::tensor::Tensor *> &inputs,
                const std::vector<lite::tensor::Tensor *> &outputs)
      : LiteKernel(parameter, inputs, outputs), packed_input_(nullptr), convert_function_(nullptr) {}
  ~CropCPUKernel() {
    if (packed_input_ != nullptr) {
      free(packed_input_);
      packed_input_ = nullptr;
    }
  }

  int Init() override;
  int ReSize() override { return 0; }
  int Run() override;

 private:
  float *packed_input_;
  LayoutConvertor convert_function_;
};
}  // namespace mindspore::kernel

#endif  // MINDSPORE_LITE_SRC_RUNTIME_KERNEL_ARM_FP32_CROP_H_

