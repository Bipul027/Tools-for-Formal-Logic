CXX = g++
CXXFLAGS = -Wall -Wextra -g -std=c++17 -I./wff -I./evaluator

TARGET = program

SRCS = main.cpp wff/validator.cpp evaluator/evaluator.cpp

OBJS = $(SRCS:.cpp=.o)

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CXX) $(CXXFLAGS) -o $@ $^

%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

clean:
	rm -f $(OBJS) $(TARGET)

.PHONY: all clean