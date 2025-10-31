CXX = g++
CXXFLAGS = -std=c++17 -Wall -Wextra -g

SRCS = MemoryManager.cpp
OBJS = $(SRCS:.cpp=.o)

TARGET_LIB = libMemoryManager.a

all: $(TARGET_LIB)

$(TARGET_LIB): $(OBJS)
	ar rcs $(TARGET_LIB) $(OBJS)

%.o: %.cpp MemoryManager.h
	$(CXX) $(CXXFLAGS) -c $< -o $@

clean:
	rm -f $(OBJS) $(TARGET_LIB)

.PHONY: all clean