#CXX=/opt/llvm-3.9/bin/clang -stdlib=libstdc++
#CXX=/opt/llvm-3.9/bin/clang -stdlib=libstdc++ -S -emit-llvm
CXX=g++
CXXFLAGS=-g -O3 -march=native -std=c++11 -Wall -Wextra -Wno-unused-parameter -Wpedantic -Winline
INCLUDES=-I../../include
LIBS=-l:libbenchmark.so -lpthread

all: loggerbenchmark cyclebenchmark

loggerbenchmark: loggerbenchmark.cpp
	${CXX} ${CXXFLAGS} loggerbenchmark.cpp ${INCLUDES} -o loggerbenchmark ${LIBS}

cyclebenchmark: cyclebenchmark.cpp
	${CXX} ${CXXFLAGS} cyclebenchmark.cpp ${INCLUDES} -o cyclebenchmark ${LIBS}

run: loggerbenchmark
	./loggerbenchmark

run-cycles: cyclebenchmark
	./cyclebenchmark

clean:
	rm -f loggerbenchmark cyclebenchmark *.log
