cmake_minimum_required (VERSION 2.6)
project (Sourcerer)

add_executable(pentae main.c pentae.c ../../src/academy.c ../../src/executor.c ../../src/c_parser.c ../../src/c_types.c ../../src/c_printer.c ../../src/changes.c ../../src/utils.c ../../src/membox.c ../../src/change_proposer_0.c)
target_include_directories(pentae PRIVATE ../../include)
target_link_libraries(pentae m)