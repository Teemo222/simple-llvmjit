cmake_minimum_required(VERSION 3.4.3)
project(kaleidoscope)

find_package(LLVM 11.1.0 REQUIRED CONFIG)
message(STATUS "Found LLVM ${LLVM_PACKAGE_VERSION}")
message(STATUS "Using LLVMConfig.cmake in ${LLVM_DIR}")
message(STATUS "LLVM Root: ${LLVM_TOOLS_BINARY_DIR}")
message(STATUS "LLVM Include dirs: ${LLVM_INCLUDE_DIRS}")

set(CMAKE_CXX_STANDARD 14)

message(STATUS "Found LLVM ${LLVM_PACKAGE_VERSION}")
message(STATUS "Using LLVMConfig.cmake in: ${LLVM_DIR}")

# Set your project compile flags.
# E.g. if using the C++ header files
# you will need to enable C++11 support
# for your compiler.

add_subdirectory(easy-just-in-time)

include_directories(${LLVM_INCLUDE_DIRS} include)

add_definitions(${LLVM_DEFINITIONS})

# Now build our tools
add_executable(kaleidoscope main.cpp)

target_compile_options(kaleidoscope PUBLIC "SHELL:-Xclang -load" "SHELL:-Xclang ${PROJECT_SOURCE_DIR}/build/easy-just-in-time/bin/EasyJitPass.so" )

target_include_directories(kaleidoscope PUBLIC ${PROJECT_SOURCE_DIR}/easy-just-in-time/include)

# Find the libraries that correspond to the LLVM components
# that we wish to use
llvm_map_components_to_libnames(llvm_libs executionengine orcjit object)

# Link against LLVM libraries
target_link_libraries(kaleidoscope ${llvm_libs} EasyJitRuntime )

add_dependencies(kaleidoscope EasyJitPass)

#target_include_directories(kaleidoscope PUBLIC "${PROJECT_SOURCE_DIR}/include")
