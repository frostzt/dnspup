CXX = clang++
CXXFLAGS = -std=c++20 -Wall -Wextra -g -O0 -I .
TARGET = bin/dnspup

HEADERS = $(shell find . -name '*.hpp')

all: $(TARGET)

$(TARGET): main.cpp $(HEADERS)
	$(CXX) $(CXXFLAGS) main.cpp -o $(TARGET)

clean:
	rm -f $(TARGET)

run: $(TARGET)
	./$(TARGET)

.PHONY: all clean run
