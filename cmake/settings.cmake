# this is rather ugly (since out-of-source builds have benefits...),
# but we want the executables to be located there...
set(EXECUTABLE_OUTPUT_PATH "${CMAKE_BINARY_DIR}")

# really no optimization in debug mode
if(CMAKE_COMPILER_IS_GNUCC)
  set(CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS_DEBUG} -O0 -Wall")
else()
  message(FATAL_ERROR "Non-gcc compiler not supported")
endif()

# set default build type if unspecified so far
if(NOT CMAKE_BUILD_TYPE)
  message(STATUS "No build type selected, default to Release")
  set(CMAKE_BUILD_TYPE "Release")
endif()

# figure out compile flags
string(TOUPPER ${CMAKE_BUILD_TYPE} BUILD_TYPE)
set(DEFAULT_COMPILE_FLAGS ${CMAKE_CXX_FLAGS_${BUILD_TYPE}})
