cmake_minimum_required(VERSION 3.10)
project(DefUsePlugin)

find_package(LLVM REQUIRED CONFIG COMPONENTS Core Support Passes)

list(APPEND CMAKE_MODULE_PATH ${LLVM_CMAKE_DIR})
include(AddLLVM)

set(SRC_DIR ${CMAKE_CURRENT_SOURCE_DIR}/src)
set(INC_DIR ${CMAKE_CURRENT_SOURCE_DIR}/include)

set(PLUGIN_SOURCES
    ${SRC_DIR}/defuse_plugin.cpp
    ${SRC_DIR}/defuse_dump_json.cpp
)

add_llvm_pass_plugin(DefUsePlugin ${PLUGIN_SOURCES})

target_include_directories(DefUsePlugin PRIVATE 
    ${INC_DIR}
    ${LLVM_INCLUDE_DIRS}
)

target_link_libraries(DefUsePlugin PRIVATE LLVM)

set_target_properties(DefUsePlugin PROPERTIES PREFIX "")
