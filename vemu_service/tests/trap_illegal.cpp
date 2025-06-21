#include "RISCV.h"
#include <cassert>
#include <memory>
#include <iostream>

using namespace CSR;
class TestEmu : public Emulator { public: void init_param() override {} };

int main(){
    auto em = std::make_unique<TestEmu>();
    // write illegal instruction (all 1s)
    em->sram[0] = 0xFFFFFFFF;
    em->pc = 0;
    em->csr.write(CSR_MTVEC, 0x100);
    em->emulate();
    assert(em->next_pc == 0x100);
    assert(em->csr.read(CSR_MEPC, em.get()) == 0);
    assert(em->csr.read(CSR_MCAUSE, em.get()) == 2);
    assert(em->csr.read(CSR_MTVAL, em.get()) == 0xFFFFFFFF);
    std::cout << "trap illegal test passed!" << std::endl;
    return 0;
} 