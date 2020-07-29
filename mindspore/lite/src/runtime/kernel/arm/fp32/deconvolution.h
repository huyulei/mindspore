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

#ifndef MINDSPORE_LITE_SRC_RUNTIME_KERNEL_ARM_FP32_DECONVOLUTION_H_
#define MINDSPORE_LITE_SRC_RUNTIME_KERNEL_ARM_FP32_DECONVOLUTION_H_

#include <vector>
#include "src/lite_kernel.h"
#include "src/kernel_registry.h"
#include "include/errorcode.h"
#include "schema/model_generated.h"
#include "src/runtime/kernel/arm/base/convolution_base.h"
#include "src/runtime/kernel/arm/opclib/fp32/deconv.h"

namespace mindspore::kernel {
class DeConvolutionCPUKernel : public ConvolutionBaseCPUKernel {
 public:
  DeConvolutionCPUKernel(OpParameter *parameter, const std::vector<lite::tensor::Tensor *> &inputs,
                         const std::vector<lite::tensor::Tensor *> &outputs, const Context *ctx)
      : ConvolutionBaseCPUKernel(parameter, inputs, outputs, ctx) {}
  ~DeConvolutionCPUKernel() override;
  int Init() override;
  int Run() override;
  int ReSize() override;

 public:
  int DoDeconv(int task_id);
  int DoPostFunc(int task_id);

 private:
  int InitParam();
  int InitWeightBias();

 private:
  StrassenMatMulParameter *matmul_param_;
  int thread_hw_count_;
  int thread_hw_stride_;
  int thread_co4_count_;
  int thread_co_stride_;
  float *weight_ptr_;
  float *tmp_buffer_;
  float *tmp_output_;
  float *c4_input_;
  float *c4_output_;
  float *input_ptr_;
  float *output_ptr_;
};
}  // namespace mindspore::kernel
#endif  // MINDSPORE_LITE_SRC_RUNTIME_KERNEL_ARM_FP32_DECONVOLUTION_H_

