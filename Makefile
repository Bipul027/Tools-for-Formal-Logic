CXX = g++
CXXFLAGS = -O3 -Wall -std=c++17 -g
LDFLAGS = -lz3
SRCS = main.cpp validity/validity.cpp cnf/cnf.cpp cnf/converting_functions.cpp wff/validator.cpp wff/tree_to_str.cpp
OBJS = $(SRCS:.cpp=.o)

all: benchmark run plot

benchmark: $(OBJS)
	$(CXX) $(CXXFLAGS) -o benchmark $(OBJS) $(LDFLAGS)

run: benchmark
	./benchmark

plot: run
	python3 plot.py

clean:
	rm -f benchmark $(OBJS) results.csv benchmark_plot.png