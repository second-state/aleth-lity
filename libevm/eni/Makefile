.PHONY: clean
clean:
	rm -f test_converters test_eni

.PHONY: test
test: test_converters test_eni
	./test_converters
	./test_eni

test_converters: test_arg_converter.cpp arg_converter.cpp test_ret_converter.cpp ret_converter.cpp
	g++ -o $@ test_arg_converter.cpp test_ret_converter.cpp $(shell pkg-config --cflags --libs gtest_main)

test_eni: test_eni.cpp
	g++ -o $@ test_eni.cpp $(shell pkg-config --cflags --libs gtest_main) -lboost_system -lboost_filesystem -ldl
