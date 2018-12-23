cmake_minimum_required (VERSION 2.6)
project (Sourcerer)


add_executable(sourcerer main.c c_types.c c_printer.c utils.c changes.c search.c change_proposer_0.c executor.c tasks.c membox.c)
target_include_directories(sourcerer PRIVATE ../include)
target_link_libraries (sourcerer LINK_PUBLIC m)
