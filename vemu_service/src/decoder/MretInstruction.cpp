#include "decoder/MretInstruction.h"
#include "RISCV.h"

namespace Decoder {

void MretInstruction::execute(Emulator* cpu) {
    uint32_t ret_pc = cpu->csr.read(CSR::CSR_MEPC, cpu);
    cpu->next_pc = ret_pc;
    cpu->trap = false; // leave trap state
    cpu->instr_name = (char*)"mret";
}

} // namespace Decoder 