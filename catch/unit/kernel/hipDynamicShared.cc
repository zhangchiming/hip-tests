/*
Copyright (c) 2023 Advanced Micro Devices, Inc. All rights reserved.
Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:
The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.
THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
THE SOFTWARE.
*/

#include <hip_test_kernels.hh>
#include <hip_test_checkers.hh>
#include <hip_test_common.hh>
 

#pragma clang diagnostic ignored "-Wunused-parameter"

unsigned threadsPerBlock = 256;

template <unsigned batch, typename T>
__device__ void sum(T* sdata, unsigned groupElements, unsigned tid) {
  T tmp;
  if (groupElements < batch)
      return;
  // sdata[tid] += sdata[tid - batch/2] does not work when block size is
  // greater than wave size because one wave may complete before another
  // wave.
  if (tid >= batch/2 && tid < groupElements)
      tmp = sdata[tid - batch/2];
  __syncthreads();
  if (tid >= batch/2 && tid < groupElements)
      sdata[tid] += tmp;
  __syncthreads();
}

template <typename T>
__global__ void testExternSharedKernel(const T* A_d, const T* B_d, T* C_d,
                                    size_t numElements, size_t groupElements) {
  // declare dynamic shared memory
  extern __shared__ double sdata0[];
  T* sdata = reinterpret_cast<T *>(sdata0);

  size_t gid = (blockIdx.x * blockDim.x + threadIdx.x);
  size_t tid = threadIdx.x;

  // initialize dynamic shared memory
  if (tid < groupElements) {
      sdata[tid] = static_cast<T>(tid);
  }
  __syncthreads();

  // prefix sum inside dynamic shared memory
  sum<512>(sdata, groupElements, tid);
  sum<256>(sdata, groupElements, tid);
  sum<128>(sdata, groupElements, tid);
  sum<64>(sdata, groupElements, tid);
  sum<32>(sdata, groupElements, tid);
  sum<16>(sdata, groupElements, tid);
  sum<8>(sdata, groupElements, tid);
  sum<4>(sdata, groupElements, tid);
  sum<2>(sdata, groupElements, tid);
  C_d[gid] = A_d[gid] + B_d[gid] + sdata[tid % groupElements];
}

template <typename T>
void testExternShared(size_t N, unsigned groupElements) {
  size_t Nbytes = N * sizeof(T);

  T *A_d, *B_d, *C_d;
  T *A_h, *B_h, *C_h;

  HipTest::initArrays(&A_d, &B_d, &C_d, &A_h, &B_h, &C_h, N, false);
  unsigned blocks = N/threadsPerBlock;
  assert(N == blocks * threadsPerBlock);

  HIP_CHECK(hipMemcpy(A_d, A_h, Nbytes, hipMemcpyHostToDevice));
  HIP_CHECK(hipMemcpy(B_d, B_h, Nbytes, hipMemcpyHostToDevice));

  // calculate the amount of dynamic shared memory required
  size_t groupMemBytes = groupElements * sizeof(T);

  // launch kernel with dynamic shared memory
  hipLaunchKernelGGL(HIP_KERNEL_NAME(testExternSharedKernel<T>), dim3(blocks),
                     dim3(threadsPerBlock), groupMemBytes, 0, A_d, B_d, C_d,
                     N, groupElements);

  HIP_CHECK(hipDeviceSynchronize());
  HIP_CHECK(hipMemcpy(C_h, C_d, Nbytes, hipMemcpyDeviceToHost));

  // verify
  for (size_t i = 0; i < N; ++i) {
    size_t tid = (i % min(threadsPerBlock, groupElements));
    T sumFromSharedMemory = static_cast<T>(tid * (tid + 1) / 2);
    T expected = A_h[i] + B_h[i] + sumFromSharedMemory;
    REQUIRE(C_h[i] == expected);
  }
  HipTest::freeArrays(A_d, B_d, C_d, A_h, B_h, C_h, false);
}

/**
* @addtogroup hipLaunchKernelGGL hipLaunchKernelGGL
* @{
* @ingroup KernelTest
* `void hipLaunchKernelGGL(F kernel, const dim3& numBlocks, const dim3& dimBlocks,
   std::uint32_t sharedMemBytes, hipStream_t stream, Args... args)` -
* Method to invocate kernel functions
*/

/**
 * Test Description
 * ------------------------
 *    - launch kernel with dynamic shared memory for float and double
 * datatypes and verify the results.

 * Test source
 * ------------------------
 *    - catch/unit/kernel/hipDynamicShared.cc
 * Test requirements
 * ------------------------
 *    - HIP_VERSION >= 5.5
 */

TEST_CASE("Unit_hipDynamicShared") {
  SECTION("test case with float for least size") {
    testExternShared<float>(1024, 4);
    testExternShared<float>(1024, 8);
    testExternShared<float>(1024, 16);
    testExternShared<float>(1024, 32);
    testExternShared<float>(1024, 64);
  }

  SECTION("test case with float for max size") {
    testExternShared<float>(65536, 4);
    testExternShared<float>(65536, 8);
    testExternShared<float>(65536, 16);
    testExternShared<float>(65536, 32);
    testExternShared<float>(65536, 64);
  }

  SECTION("test case with double for least size") {
    testExternShared<double>(1024, 4);
    testExternShared<double>(1024, 8);
    testExternShared<double>(1024, 16);
    testExternShared<double>(1024, 32);
    testExternShared<double>(1024, 64);
  }

  SECTION("test case with double for max size") {
    testExternShared<double>(65536, 4);
    testExternShared<double>(65536, 8);
    testExternShared<double>(65536, 16);
    testExternShared<double>(65536, 32);
    testExternShared<double>(65536, 64);
  }

  SECTION("test case with float for max LDS size") {
    int maxLDS = 0;
    HIP_CHECK(hipDeviceGetAttribute(&maxLDS,
                                  hipDeviceAttributeMaxSharedMemoryPerBlock, 0));
    testExternShared<float>(1024, maxLDS/sizeof(float));
  }
}

/**
* End doxygen group KernelTest.
* @}
*/
