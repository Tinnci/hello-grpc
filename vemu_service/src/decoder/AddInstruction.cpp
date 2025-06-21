#include "decoder/AddInstruction.h"
#include "RISCV.h"

namespace Decoder {

AddInstruction::AddInstruction(uint32_t word)
    : Instruction(word) {
    rd_  = (word >> 7)  & 0x1F;
    rs1_ = (word >> 15) & 0x1F;
    rs2_ = (word >> 20) & 0x1F;
}

void AddInstruction::execute(Emulator* cpu) {
    if (rd_ == 0) {
        // x0 寄存器恒为 0，忽略写回
        cpu->next_pc = cpu->pc + 4;
        return;
    }
    cpu->cpuregs[rd_] = cpu->cpuregs[rs1_] + cpu->cpuregs[rs2_];
    cpu->next_pc = cpu->pc + 4;
}

} // namespace Decoder 