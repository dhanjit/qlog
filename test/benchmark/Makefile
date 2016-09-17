#CXX=/opt/llvm-3.9/bin/clang -stdlib=libstdc++
#CXX=/opt/llvm-3.9/bin/clang -stdlib=libstdc++ -S -emit-llvm
CXX=g++
all:
	${CXX} -g -O3 -march=native loggerbenchmark.cpp  -I../../include -o loggerbenchmark -std=c++11 -Wall -Wextra  -Wno-unused-parameter -l:libbenchmark.so -lpthread -Wpedantic -Winline
run:
	./loggerbenchmark
