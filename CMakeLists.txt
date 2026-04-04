cmake_minimum_required(VERSION 3.10)
project(DefUsePlugin)

set(SRC_DIR ${CMAKE_CURRENT_SOURCE_DIR}/src)
set(INC_DIR ${CMAKE_CURRENT_SOURCE_DIR}/include)

# finding installed llvm
find_package(LLVM REQUIRED CONFIG)

# adding paths to includes
foreach(dir ${LLVM_INCLUDE_DIRS})
    if(IS_DIRECTORY "${dir}/llvm/Passes")
        include_directories(SYSTEM "${dir}/llvm/Passes")
    endif()
    if(IS_DIRECTORY "${dir}/llvm/Plugins")
        include_directories(SYSTEM "${dir}/llvm/Plugins")
    endif()
endforeach()
include_directories(${LLVM_INCLUDE_DIRS} ${INC_DIR})
link_directories(${LLVM_LIBRARY_DIRS})
add_definitions(${LLVM_DEFINITIONS})

# creating library (our plugin)
add_library(DefUsePlugin MODULE
    ${SRC_DIR}/defuse_plugin.cpp
)

# throwing out prefix 'lib'
set_target_properties(DefUsePlugin PROPERTIES PREFIX "")

llvm_map_components_to_libnames(llvm_libs core support irreader)
target_link_libraries(DefUsePlugin LLVM)
