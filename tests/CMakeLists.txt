cmake_minimum_required (VERSION 2.6)
project (Sourcerer)

add_executable(check_membox check_membox.c ../src/membox.c ../src/utils.c)
target_include_directories(check_membox PRIVATE ../include)

add_executable(check_parser check_parser.c ../src/c_parser.c ../src/c_types.c ../src/c_printer.c ../src/changes.c ../src/utils.c ../src/membox.c ../src/change_proposer_0.c)
target_include_directories(check_parser PRIVATE ../include)

add_executable(check_parser_and_executor check_parser_and_executor.c ../src/executor.c ../src/c_parser.c ../src/c_types.c ../src/c_printer.c ../src/changes.c ../src/utils.c ../src/membox.c ../src/change_proposer_0.c)
target_include_directories(check_parser_and_executor PRIVATE ../include)

add_executable(check_printer check_printer.c ../src/c_types.c ../src/c_printer.c ../src/changes.c ../src/utils.c ../src/membox.c ../src/change_proposer_0.c)
target_include_directories(check_printer PRIVATE ../include)
