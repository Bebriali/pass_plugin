cmake_minimum_required(VERSION 3.10)
project(DefUsePlugin)

# finding installed llvm
find_package(LLVM REQUIRED CONFIG)

# adding paths to includes
include_directories(${LLVM_INCLUDE_DIRS} include/)
add_definitions(${LLVM_DEFINITIONS})

# creating library (our plugin)
add_library(DefUsePlugin MODULE
    defuse_plugin.cpp
)

# throwing out prefix 'lib'
set_target_properties(DefUsePlugin PROPERTIES PREFIX "")

llvm_map_components_to_libnames(llvm_libs core support irreader)
target_link_libraries(DefUsePlugin LLVM)