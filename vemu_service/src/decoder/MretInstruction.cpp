#include "decoder/MretInstruction.h"
#include "RISCV.h"

namespace Decoder {

void MretInstruction::execute(Emulator* cpu) {
    // Restore PC
    uint32_t ret_pc = cpu->csr.read(CSR::CSR_MEPC, cpu);
    cpu->next_pc = ret_pc;

    // Restore mstatus: MIE <= MPIE, MPIE <= 1, MPP <= 0
    uint32_t mstatus = cpu->csr.read(CSR::CSR_MSTATUS, cpu);
    bool mpie = mstatus & CSR::MSTATUS_MPIE;
    // Clear MIE and MPIE and MPP bits
    mstatus &= ~(CSR::MSTATUS_MIE | CSR::MSTATUS_MPIE | CSR::MSTATUS_MPP_MASK);
    if (mpie) mstatus |= CSR::MSTATUS_MIE; // restore
    mstatus |= CSR::MSTATUS_MPIE;          // set MPIE=1
    // MPP set to 0 (already cleared above)
    cpu->csr.write(CSR::CSR_MSTATUS, mstatus);

    cpu->trap = false; // leave trap state
    cpu->instr_name = (char*)"mret";
}

} // namespace Decoder 