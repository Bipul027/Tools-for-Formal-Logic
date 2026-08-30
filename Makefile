CXX = g++
CXXFLAGS = -Wall -Wextra -g -std=c++17 -I./wff -I./evaluator

TARGET = program

SRCS = main.cpp wff/validator.cpp evaluator/evaluator.cpp wff/tree_to_str.cpp cnf/converting_functions.cpp

OBJS = $(SRCS:.cpp=.o)

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CXX) $(CXXFLAGS) -o $@ $^

%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

clean:
	rm -f $(OBJS) $(TARGET)

.PHONY: all clean