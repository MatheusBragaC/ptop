enable_testing()

add_executable(test_parser test_parser.c ../src/core/parser.c)
target_include_directories(test_parser PRIVATE ../src/common ../src/core)

add_test(NAME ParserTest COMMAND test_parser)
