CXX = clang++
CXXFLAGS = -std=c++20 -Wall -Wextra -g -O0
TARGET = dns_resolver

all: $(TARGET)

$(TARGET): main.cpp *.hpp
	$(CXX) $(CXXFLAGS) main.cpp -o $(TARGET)

clean:
	rm -f $(TARGET)

run: $(TARGET)
	./$(TARGET)

.PHONY: all clean run
