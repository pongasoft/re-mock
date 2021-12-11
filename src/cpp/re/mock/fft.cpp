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

#include "Errors.h"
#include <JukeboxTypes.h>
#include <complex>
#include <valarray>
#include "fft.h"

namespace re::mock {

using Complex = std::complex<TJBox_Float32>;
using ComplexArray = std::valarray<Complex>;

namespace impl {

//------------------------------------------------------------------------
// impl::inPlaceFFT | Implementation based on Cooley–Tukey FFT
//------------------------------------------------------------------------
void inPlaceFFT(ComplexArray &ioData)
{
  constexpr TJBox_Float32 PI = 3.14159265358979323846264338327950288;

  const size_t N = ioData.size();

  if(N <= 1)
    return;

  ComplexArray even = ioData[std::slice(0, N / 2, 2)];
  ComplexArray odd = ioData[std::slice(1, N / 2, 2)];

  inPlaceFFT(even);
  inPlaceFFT(odd);

  const auto factor = -2.0 * PI / N;

  for(size_t k = 0; k < N / 2; ++k)
  {
    auto t = std::polar<TJBox_Float32>(1.0, factor * k) * odd[k];
    ioData[k] = even[k] + t;
    ioData[k + N / 2] = even[k] - t;
  }

}

//------------------------------------------------------------------------
// impl::inPlaceInverseFFT
//------------------------------------------------------------------------
void inPlaceInverseFFT(ComplexArray &ioData)
{
  ioData = ioData.apply(std::conj);
  inPlaceFFT(ioData);
  ioData = ioData.apply(std::conj);
  ioData /= ioData.size();
}

}

//------------------------------------------------------------------------
// computeFFTRealForward
//------------------------------------------------------------------------
void computeFFTRealForward(TJBox_Int32 iFFTSize, TJBox_Float32 ioData[])
{
  // This implementation is not meant to be optimized. It uses an algorithm that works on complex numbers.
  RE_MOCK_ASSERT(iFFTSize >=6 && iFFTSize <= 15,
                 "iFFTSize must be larger than or equal to 6 and must be smaller than or equal to 15");

  // computes the size of the array: per documentation "expressed as the log2 of the input data array size"
  const auto arraySize = 2 << (iFFTSize - 1); // 2 ^ iFFTSize

  ComplexArray c(arraySize);

  // populate the complex array with the real part only
  for(int i = 0; i < arraySize; i++)
  {
    c[i].real(ioData[i]);
    c[i].imag(0);
  }

  // compute the FFT (in place)
  impl::inPlaceFFT(c);

  auto ptr = ioData;

  // because the input was real, we only need half of the input, because it is mirrored
  for(int i = 0; i < arraySize / 2; i++)
  {
    *ptr++ = c[i].real();
    *ptr++ = c[i].imag();
  }

  // this is due to the fact that (although it is not specified!) c[0] and c[N/2] are both real and
  // both (real) values are packed into ioData[0] and ioData[1]!!
  ioData[1] = c[arraySize / 2].real();
}

//------------------------------------------------------------------------
// computeFFTRealInverse
//------------------------------------------------------------------------
void computeFFTRealInverse(TJBox_Int32 iFFTSize, TJBox_Float32 ioData[])
{
  // This implementation is not meant to be optimized. It uses an algorithm that works on complex numbers.
  RE_MOCK_ASSERT(iFFTSize >=6 && iFFTSize <= 15,
                 "iFFTSize must be larger than or equal to 6 and must be smaller than or equal to 15");

  // computes the size of the array: per documentation "expressed as the log2 of the input data array size"
  const auto arraySize = 2 << (iFFTSize - 1); // 2 ^ iFFTSize

  ComplexArray c(arraySize);

  auto ptr = ioData;

  // revert the "hack" of packing c[0].real() and c[N/2].real() into ioData[0] and ioData[1]
  c[0].real(*ptr++);
  c[0].imag(0);

  c[arraySize / 2].real(*ptr++);
  c[arraySize / 2].imag(0);

  for(int i = 1; i < arraySize / 2; i++)
  {
    c[i].real(*ptr++);
    c[i].imag(*ptr++);
    c[arraySize - i] = std::conj(c[i]); // mirror
  }

  // compute the inverse FFT (in place)
  impl::inPlaceInverseFFT(c);

  // we copy the real part
  for(int i = 0; i < arraySize; i++)
    ioData[i] = c[i].real();
}

}