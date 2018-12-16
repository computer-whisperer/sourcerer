cmake_minimum_required (VERSION 2.6)
project (Sourcerer)


add_executable(sourcerer main.c executor.c c_printer.c c_types.c changer_0.c)
target_include_directories(sourcerer PRIVATE ../include)
target_link_libraries (sourcerer LINK_PUBLIC m)
