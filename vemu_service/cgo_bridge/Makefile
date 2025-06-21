CXX ?= g++
AR  ?= ar
CXXFLAGS ?= -std=c++17 -O2 -fPIC -I.. -I../include -Wno-unused-result

SRC = core_bridge.cpp \
      ../src/RISCV.cpp \
      ../src/venus_ext.cpp \
      ../src/venus_mem_access.cpp \
      ../src/decoder/Decoder.cpp \
      ../src/decoder/AddInstruction.cpp \
      ../src/decoder/ArithmeticRInstruction.cpp \
      ../src/decoder/MulDivInstruction.cpp \
      ../src/decoder/ITypeImmInstruction.cpp \
      ../src/decoder/CsrInstruction.cpp \
      ../src/decoder/LoadInstruction.cpp \
      ../src/decoder/StoreInstruction.cpp \
      ../src/decoder/JalInstruction.cpp \
      ../src/decoder/JalrInstruction.cpp \
      ../src/decoder/BranchInstruction.cpp \
      ../src/decoder/EbreakInstruction.cpp \
      ../src/decoder/MretInstruction.cpp \
      ../src/csr/CsrFile.cpp \
      ../src/mem/MMU.cpp
OBJ = $(SRC:.cpp=.o)

libvemu.a: $(OBJ)
	$(AR) rcs $@ $^

%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

clean:
	rm -f $(OBJ) libvemu.a 

TEST_CSR_SRC = ../tests/compare_decoder_csr.cpp

.PHONY: test_decoder_csr

test_decoder_csr: libvemu.a $(TEST_CSR_SRC)
	$(CXX) $(CXXFLAGS) -I.. -I../include $(TEST_CSR_SRC) libvemu.a -o test_decoder_csr 

TEST_TRAP_SRC = ../tests/trap_flow_test.cpp

.PHONY: test_trap

test_trap: libvemu.a $(TEST_TRAP_SRC)
	$(CXX) $(CXXFLAGS) -I.. -I../include $(TEST_TRAP_SRC) libvemu.a -o test_trap 

TEST_MEM_SRC = ../tests/memory_access_test.cpp

.PHONY: test_mem

test_mem: libvemu.a $(TEST_MEM_SRC)
	$(CXX) $(CXXFLAGS) -I.. -I../include $(TEST_MEM_SRC) libvemu.a -o test_mem 

TEST_ISA_R_SRC = ../tests/isa_r.cpp
TEST_ISA_I_SRC = ../tests/isa_i.cpp
TEST_ISA_M_SRC = ../tests/isa_m.cpp

.PHONY: test_isa_r test_isa_i test_isa_m test_isa

test_isa_r: libvemu.a $(TEST_ISA_R_SRC)
	$(CXX) $(CXXFLAGS) -I.. -I../include $(TEST_ISA_R_SRC) libvemu.a -o test_isa_r

test_isa_i: libvemu.a $(TEST_ISA_I_SRC)
	$(CXX) $(CXXFLAGS) -I.. -I../include $(TEST_ISA_I_SRC) libvemu.a -o test_isa_i

test_isa_m: libvemu.a $(TEST_ISA_M_SRC)
	$(CXX) $(CXXFLAGS) -I.. -I../include $(TEST_ISA_M_SRC) libvemu.a -o test_isa_m

# 聚合目标
test_isa: test_isa_r test_isa_i test_isa_m

.PHONY: all
all: libvemu.a 

TEST_INT_SRC = ../tests/interrupt_basic.cpp

.PHONY: test_interrupt

test_interrupt: libvemu.a $(TEST_INT_SRC)
	$(CXX) $(CXXFLAGS) -I.. -I../include $(TEST_INT_SRC) libvemu.a -o test_interrupt 

TEST_MRET_SRC = ../tests/interrupt_mret.cpp

.PHONY: test_mret

test_mret: libvemu.a $(TEST_MRET_SRC)
	$(CXX) $(CXXFLAGS) -I.. -I../include $(TEST_MRET_SRC) libvemu.a -o test_mret 

TEST_ILLEGAL_SRC = ../tests/trap_illegal.cpp

.PHONY: test_trap_illegal

test_trap_illegal: libvemu.a $(TEST_ILLEGAL_SRC)
	$(CXX) $(CXXFLAGS) -I.. -I../include $(TEST_ILLEGAL_SRC) libvemu.a -o test_trap_illegal 

TEST_MISALIGN_SRC = ../tests/trap_misaligned.cpp

.PHONY: test_trap_misaligned

test_trap_misaligned: libvemu.a $(TEST_MISALIGN_SRC)
	$(CXX) $(CXXFLAGS) -I.. -I../include $(TEST_MISALIGN_SRC) libvemu.a -o test_trap_misaligned 