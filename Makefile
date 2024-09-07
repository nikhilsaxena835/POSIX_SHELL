# Compiler and flags
CXX = g++
CXXFLAGS = -Wall -Wextra -std=c++11

# Source files and executable name
SRCS = main.cpp cd.cpp commandCentre.cpp echos.cpp getInfo.cpp history.cpp ls.cpp pinfo.cpp search.cpp
OBJS = $(SRCS:.cpp=.o)
EXEC = nshell

# Default target
all: $(EXEC)

# Linking
$(EXEC): $(OBJS)
	$(CXX) $(CXXFLAGS) -o $@ $(OBJS)

# Compiling
%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c $<

# Clean up
clean:
	rm -f $(OBJS) $(EXEC)

.PHONY: all clean
