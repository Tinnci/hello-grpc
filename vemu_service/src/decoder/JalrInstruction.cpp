#include "decoder/JalrInstruction.h"
#include "RISCV.h"

namespace Decoder {

static inline int32_t sign_extend(int32_t val, int bits) {
    int32_t m = 1u << (bits - 1);
    return (val ^ m) - m;
}

void JalrInstruction::execute(Emulator* cpu) {
    int32_t imm = sign_extend(static_cast<int32_t>(word_ >> 20) & 0xFFF, 12);
    uint8_t rd = (word_ >> 7) & 0x1F;
    uint8_t rs1 = (word_ >> 15) & 0x1F;

    uint32_t target = (cpu->cpuregs[rs1] + imm) & ~1u;
    if (rd != 0) {
        cpu->cpuregs[rd] = cpu->pc + 4;
    }
    cpu->next_pc = target;
    cpu->instr_name = (char*)"jalr";
}

} 