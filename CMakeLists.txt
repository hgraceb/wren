cmake_minimum_required(VERSION 3.19)
project(wren)

set(CMAKE_CXX_STANDARD 14)

include_directories(include)
include_directories(vm)
include_directories(test)

add_executable(
    wren
    include/wren.h
    include/wren.hpp
    vm/wren_common.h
    vm/wren_compiler.c
    vm/wren_compiler.h
    vm/wren_core.c
    vm/wren_core.h
    vm/wren_debug.c
    vm/wren_debug.h
    vm/wren_math.h
    vm/wren_opcodes.h
    vm/wren_primitive.c
    vm/wren_primitive.h
    vm/wren_utils.c
    vm/wren_utils.h
    vm/wren_value.c
    vm/wren_value.h
    vm/wren_vm.c
    vm/wren_vm.h
    test/main.c
    test/test.c
    test/test.h
)
