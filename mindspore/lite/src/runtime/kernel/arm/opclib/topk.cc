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

#include "src/runtime/kernel/arm/opclib/topk.h"

int DescendCmp(const void *a, const void *b) {
  return ((const Node *)b)->element - ((const Node *)a)->element;
}

int AscendCmp(const void *a, const void *b) {
  return ((const Node *)a)->element - ((const Node *)b)->element;
}

void Topk(float *input_data, float *output_data, float *output_index, TopkParameter *parameter) {
  int last_dim_size = parameter->last_dim_size_;
  int loop_num = parameter->loop_num_;
  int k = parameter->k_;
  Node *top_map = parameter->topk_node_list_;

  float *cur_input_data = input_data;
  float *cur_output_data = output_data;
  float *cur_output_index = output_index;
  for (int i = 0; i < loop_num; i++) {
    for (int j = 0; j < last_dim_size; j++) {
      top_map[j].element = *(cur_input_data + j);
      top_map[j].index = j;
    }
    if (parameter->sorted_) {
        qsort(top_map, last_dim_size, sizeof(top_map[0]), DescendCmp);
    } else {
        qsort(top_map, last_dim_size, sizeof(top_map[0]), AscendCmp);
    }
    for (int m = 0; m < k; m++) {
      cur_output_data[m] = top_map[m].element;
      cur_output_index[m] = top_map[m].index;
    }
    cur_input_data += last_dim_size;
    cur_output_data += k;
    cur_output_index += k;
  }
}

