cmake_minimum_required (VERSION 2.6)
project (Sourcerer)

add_executable(check_membox check_membox.c ../src/membox.c ../src/utils.c)
target_include_directories(check_membox PRIVATE ../include)
