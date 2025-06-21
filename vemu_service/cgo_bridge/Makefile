CXX ?= g++
AR  ?= ar
CXXFLAGS ?= -std=c++17 -O2 -fPIC -I.. -I../include

SRC = core_bridge.cpp ../src/RISCV.cpp ../src/venus_ext.cpp ../src/decoder/Decoder.cpp ../src/decoder/AddInstruction.cpp ../src/decoder/ArithmeticRInstruction.cpp ../src/decoder/MulDivInstruction.cpp ../src/decoder/ITypeImmInstruction.cpp
OBJ = $(SRC:.cpp=.o)

libvemu.a: $(OBJ)
	$(AR) rcs $@ $^

%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

clean:
	rm -f $(OBJ) libvemu.a 

TEST_SRC = ../tests/compare_decoder.cpp

.PHONY: test_decoder

test_decoder: libvemu.a $(TEST_SRC)
	$(CXX) $(CXXFLAGS) -I.. -I../include $(TEST_SRC) libvemu.a -o test_decoder 

TEST_M_SRC = ../tests/compare_decoder_m.cpp

.PHONY: test_decoder_m

test_decoder_m: libvemu.a $(TEST_M_SRC)
	$(CXX) $(CXXFLAGS) -I.. -I../include $(TEST_M_SRC) libvemu.a -o test_decoder_m 

TEST_I_SRC = ../tests/compare_decoder_i.cpp

.PHONY: test_decoder_i

test_decoder_i: libvemu.a $(TEST_I_SRC)
	$(CXX) $(CXXFLAGS) -I.. -I../include $(TEST_I_SRC) libvemu.a -o test_decoder_i 