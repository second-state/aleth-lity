@PHONY: test
test: test_converters
	./test_converters

test_converters: test_arg_converter.cpp arg_converter.cpp test_ret_converter.cpp ret_converter.cpp
	g++ -o $@ test_arg_converter.cpp test_ret_converter.cpp $(shell pkg-config --cflags --libs gtest_main)
