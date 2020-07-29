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

#ifndef MINDSPORE_LITE_SRC_RUNTIME_KERNEL_ARM_FP32_GATHERND_H_
#define MINDSPORE_LITE_SRC_RUNTIME_KERNEL_ARM_FP32_GATHERND_H_

#include <vector>
#include "src/runtime/kernel/arm/opclib/fp32/gatherNd.h"
#include "src/lite_kernel.h"

#include "include/context.h"
#include "src/runtime/kernel/arm/opclib/op_base.h"

using mindspore::lite::Context;

namespace mindspore::kernel {
class GatherNdCPUKernel : public LiteKernel {
 public:
  GatherNdCPUKernel(OpParameter *parameter, const std::vector<lite::tensor::Tensor *> &inputs,
                    const std::vector<lite::tensor::Tensor *> &outputs, const Context *ctx)
      : LiteKernel(parameter, inputs, outputs), ctx_(ctx), thread_count_(ctx->threadNum) {}
  ~GatherNdCPUKernel() override;

  int Init() override;
  int ReSize() override;
  int Run() override;
  int DoGatherNd(int task_id);

 private:
  int thread_count_;
  int thread_sz_count_;
  int thread_sz_stride_;
  int count_;
  int area_;
  int *in_offset_ = nullptr;
  float *in_ptr_;
  float *out_ptr_;
  const Context *ctx_;
};
}  // namespace mindspore::kernel

#endif  // MINDSPORE_LITE_SRC_RUNTIME_KERNEL_ARM_FP32_GATHERND_H_

