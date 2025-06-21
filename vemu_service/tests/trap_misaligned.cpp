#include "RISCV.h"
#include <cassert>
#include <memory>
#include <iostream>
using namespace CSR;
class TestEmu : public Emulator { public: void init_param() override {} };

int main(){
    auto em = std::make_unique<TestEmu>();
    em->csr.write(CSR_MTVEC, 0x100);
    // Misaligned LW (addr 0x3)
    uint32_t lw_word = (0b11 | (1<<15) | (0b000<<12) | (2<<7) | 0b0000011); // simplified encoding not used directly
    em->sram[0] = lw_word; // placeholder
    em->load_store_addr = 0x3;
    em->pc = 0;
    em->raise_trap(4);
    assert(em->next_pc == 0x100);
    assert(em->csr.read(CSR_MCAUSE, em.get()) == 4);
    assert(em->csr.read(CSR_MTVAL, em.get()) == 0x3);
    std::cout << "trap misaligned test passed!" << std::endl;
    return 0;
} 