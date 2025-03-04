# Copyright (c) 2023 Advanced Micro Devices, Inc. All Rights Reserved.
#
# Permission is hereby granted, free of charge, to any person obtaining a copy
# of this software and associated documentation files (the "Software"), to deal
# in the Software without restriction, including without limitation the rights
# to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
# copies of the Software, and to permit persons to whom the Software is
# furnished to do so, subject to the following conditions:
#
# The above copyright notice and this permission notice shall be included in
# all copies or substantial portions of the Software.
#
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
# IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
# FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL THE
# AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
# LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
# OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
# THE SOFTWARE.

set(TEST_SRC
    launch_bounds.cc
)

if(HIP_PLATFORM MATCHES "nvidia")
    hip_add_exe_to_target(NAME LaunchBoundsTest
                          TEST_SRC ${TEST_SRC}
                          TEST_TARGET_NAME build_tests
                          LINKER_LIBS nvrtc)
elseif(HIP_PLATFORM MATCHES "amd")
    hip_add_exe_to_target(NAME LaunchBoundsTest
                          TEST_SRC ${TEST_SRC}
                          TEST_TARGET_NAME build_tests
                          LINKER_LIBS hiprtc)
endif()

if(UNIX)
file(GLOB NEGATIVE_TEST_SRC
     "launch_bounds_compiler_error_kernels.cc"
      "launch_bounds_parse_error_kernels.cc")

file(COPY ${NEGATIVE_TEST_SRC} DESTINATION ${CMAKE_CURRENT_BINARY_DIR}/src)

#if(HIP_PLATFORM MATCHES "nvidia")
# set(EXPECTED_ERRORS 2)
#elseif(HIP_PLATFORM MATCHES "amd")
# set(EXPECTED_ERRORS 3)
#endif()
#
#add_test(NAME Unit_Kernel_Launch_bounds_Negative_Parameters_CompilerError
#         COMMAND ${Python3_EXECUTABLE} ../compileAndCaptureOutput.py
#         ./src ${HIP_PLATFORM} ${HIP_PATH}
#         launch_bounds_compiler_error_kernels.cc ${EXPECTED_ERRORS})
#
#if(HIP_PLATFORM MATCHES "amd")
#	add_test(NAME Unit_Kernel_Launch_bounds_Negative_Parameters_ParseError
#        COMMAND ${Python3_EXECUTABLE} ../compileAndCaptureOutput.py
#        ./src ${HIP_PLATFORM} ${HIP_PATH}
#        launch_bounds_parse_error_kernels.cc 0 0)
#endif()
endif()