@PHONY: test
test: test_converters
	./test_converters

test_converters: test_arg_converter.cc arg_converter.cc test_ret_converter.cc ret_converter.cc
	g++ -o $@ test_arg_converter.cc test_ret_converter.cc $(shell pkg-config --cflags --libs gtest)
