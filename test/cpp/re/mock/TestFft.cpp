/*
 * Copyright (c) 2021 pongasoft
 *
 * Licensed under the Apache License, Version 2.0 (the "License"); you may not
 * use this file except in compliance with the License. You may obtain a copy of
 * the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS, WITHOUT
 * WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. See the
 * License for the specific language governing permissions and limitations under
 * the License.
 *
 * @author Yan Pujante
 */

#include <re/mock/fft.h>
#include <re/mock/MockDevices.h>
#include <gtest/gtest.h>


namespace re::mock::Test {

using namespace mock;

// Fft.compute
TEST(Fft, compute)
{
  constexpr auto N = 64;

  // input[i] = i/N
  std::array<TJBox_Float32, N> input{
    0.000000, 0.015625, 0.031250, 0.046875, 0.062500, 0.078125, 0.093750, 0.109375,
    0.125000, 0.140625, 0.156250, 0.171875, 0.187500, 0.203125, 0.218750, 0.234375,
    0.250000, 0.265625, 0.281250, 0.296875, 0.312500, 0.328125, 0.343750, 0.359375,
    0.375000, 0.390625, 0.406250, 0.421875, 0.437500, 0.453125, 0.468750, 0.484375,
    0.500000, 0.515625, 0.531250, 0.546875, 0.562500, 0.578125, 0.593750, 0.609375,
    0.625000, 0.640625, 0.656250, 0.671875, 0.687500, 0.703125, 0.718750, 0.734375,
    0.750000, 0.765625, 0.781250, 0.796875, 0.812500, 0.828125, 0.843750, 0.859375,
    0.875000, 0.890625, 0.906250, 0.921875, 0.937500, 0.953125, 0.968750, 0.984375};

  std::array<TJBox_Float32, N> recon_computed_result{
    31.500004, -0.500000, -0.500000, 10.177735, -0.500000, 5.076585, -0.500000, 3.370726,
    -0.500000, 2.513670, -0.500000, 1.996112, -0.500000, 1.648279, -0.500000, 1.397406,
    -0.500000, 1.207107, -0.500000, 1.057161, -0.500000, 0.935434, -0.500000, 0.834200,
    -0.500000, 0.748303, -0.500000, 0.674172, -0.500000, 0.609252, -0.500000, 0.551665,
    -0.500000, 0.500000, -0.500000, 0.453173, -0.500000, 0.410340, -0.500000, 0.370825,
    -0.500000, 0.334089, -0.500000, 0.299688, -0.500000, 0.267256, -0.500000, 0.236482,
    -0.500000, 0.207107, -0.500000, 0.178903, -0.500000, 0.151673, -0.500000, 0.125243,
    -0.500000, 0.099456, -0.500000, 0.074168, -0.500000, 0.049246, -0.500000, 0.024563
  };

  std::array<TJBox_Float32, N> a(input);

  computeFFTRealForward(6, a.data());

  for(int i = 0; i < N; i++)
    ASSERT_TRUE(MockAudioDevice::eqWithPrecision(1.0e-5, recon_computed_result[i], a[i]));

  computeFFTRealInverse(6, a.data());

  for(int i = 0; i < N; i++)
    ASSERT_TRUE(MockAudioDevice::eqWithPrecision(1.0e-5, input[i], a[i]));
}


}