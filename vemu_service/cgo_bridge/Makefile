CXX ?= g++
AR  ?= ar
CXXFLAGS ?= -std=c++17 -O2 -fPIC -I.. -I../include

SRC = core_bridge.cpp ../src/RISCV.cpp ../src/venus_ext.cpp ../src/decoder/Decoder.cpp ../src/decoder/AddInstruction.cpp ../src/decoder/ArithmeticRInstruction.cpp
OBJ = $(SRC:.cpp=.o)

libvemu.a: $(OBJ)
	$(AR) rcs $@ $^

%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

clean:
	rm -f $(OBJ) libvemu.a 