cmake_minimum_required(VERSION 3.10)
project(DefUsePlugin)

find_package(LLVM REQUIRED CONFIG)

list(APPEND CMAKE_MODULE_PATH ${LLVM_CMAKE_DIR})
include(AddLLVM)

set(SRC_DIR ${CMAKE_CURRENT_SOURCE_DIR}/src)
set(INC_DIR ${CMAKE_CURRENT_SOURCE_DIR}/include)

# adding paths to includes for debugging in different systems
foreach(dir ${LLVM_INCLUDE_DIRS})
    if(IS_DIRECTORY "${dir}/llvm/Passes")
        include_directories(SYSTEM "${dir}/llvm/Passes")
    endif()
    if(IS_DIRECTORY "${dir}/llvm/Plugins")
        include_directories(SYSTEM "${dir}/llvm/Plugins")
    endif()
endforeach()

include_directories(${LLVM_INCLUDE_DIRS} ${INC_DIR})
add_definitions(${LLVM_DEFINITIONS})

set(POLL_SOURCES
    ${SRC_DIR}/defuse_plugin.cpp
    ${SRC_DIR}/defuse_dump_dot.cpp
    ${SRC_DIR}/defuse_dump_json.cpp
)
add_llvm_pass_plugin(DefUsePlugin ${POLL_SOURCES})

set_target_properties(DefUsePlugin PROPERTIES PREFIX "")

target_link_libraries(DefUsePlugin PRIVATE LLVM)
