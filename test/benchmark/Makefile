#CXX=/opt/llvm-3.9/bin/clang -stdlib=libstdc++
#CXX=/opt/llvm-3.9/bin/clang -stdlib=libstdc++ -S -emit-llvm
CXX=g++
all:
	${CXX} -g -O3 -march=native loggerbenchmark.cpp  /usr/local/lib/libbenchmark.a -I../../include -o loggerbenchmark -std=c++11 -Wall -Wextra  -Wno-unused-parameter -lpthread -Wpedantic -Winline
run:
	./loggerbenchmark
