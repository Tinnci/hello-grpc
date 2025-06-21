#include "decoder/JalInstruction.h"
#include "RISCV.h"

namespace Decoder {

static inline int32_t sign_extend(int32_t val, int bits) {
    int32_t m = 1u << (bits - 1);
    return (val ^ m) - m;
}

void JalInstruction::execute(Emulator* cpu) {
    // imm[20|10:1|11|19:12] << 1 (sign-extended)
    int32_t imm20 = (word_ >> 31) & 0x1;
    int32_t imm10_1 = (word_ >> 21) & 0x3FF;
    int32_t imm11 = (word_ >> 20) & 0x1;
    int32_t imm19_12 = (word_ >> 12) & 0xFF;
    int32_t imm = (imm20 << 20) | (imm19_12 << 12) | (imm11 << 11) | (imm10_1 << 1);
    imm = sign_extend(imm, 21);

    uint8_t rd = (word_ >> 7) & 0x1F;

    uint32_t next = cpu->pc + imm;
    if (rd != 0) {
        cpu->cpuregs[rd] = cpu->pc + 4;
    }
    cpu->next_pc = next;
    cpu->instr_name = (char*)"jal";
}

} 