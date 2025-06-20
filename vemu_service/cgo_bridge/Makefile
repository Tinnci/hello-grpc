CXX ?= g++
AR  ?= ar
CXXFLAGS ?= -std=c++17 -O2 -I..

SRC = core_bridge.cpp
OBJ = $(SRC:.cpp=.o)

libvemu.a: $(OBJ)
	$(AR) rcs $@ $^

%.o: %.cpp core_bridge.h
	$(CXX) $(CXXFLAGS) -c $< -o $@

clean:
	rm -f $(OBJ) libvemu.a 