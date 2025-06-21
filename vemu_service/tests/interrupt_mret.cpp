#include "RISCV.h"
#include "decoder/MretInstruction.h"
#include <cassert>
#include <iostream>
#include <memory>

using namespace CSR;
class TestEmu : public Emulator { public: void init_param() override {} };

int main(){
    auto em = std::make_unique<TestEmu>();
    // Setup state: set mepc to 0x200
    em->csr.write(CSR_MEPC, 0x200);
    // mstatus: MIE=0, MPIE=1, MPP=3 (machine)
    uint32_t ms = MSTATUS_MPIE | MSTATUS_MPP_MASK; // MPIE=1, MPP=3
    em->csr.write(CSR_MSTATUS, ms);

    Decoder::MretInstruction inst(0x30200073); // dummy word
    inst.execute(em.get());

    // Expect pc to jump to mepc
    assert(em->next_pc == 0x200);
    // After mret, MIE should equal 1 (copied from MPIE) , MPIE should be 1, MPP cleared
    uint32_t new_ms = em->csr.read(CSR_MSTATUS, em.get());
    assert(new_ms & MSTATUS_MIE);
    assert(new_ms & MSTATUS_MPIE);
    assert((new_ms & MSTATUS_MPP_MASK) == 0);

    std::cout << "mret interrupt restore test passed!" << std::endl;
    return 0;
} 