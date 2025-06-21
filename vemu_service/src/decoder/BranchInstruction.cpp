#include "decoder/BranchInstruction.h"
#include "RISCV.h"
#include <cstdint>

namespace Decoder {

static inline int32_t sign_extend(int32_t val, int bits) {
    int32_t m = 1u << (bits - 1);
    return (val ^ m) - m;
}

void BranchInstruction::execute(Emulator* cpu) {
    uint8_t rs1 = (word_ >> 15) & 0x1F;
    uint8_t rs2 = (word_ >> 20) & 0x1F;
    uint8_t funct3 = (word_ >> 12) & 0x7;

    uint32_t offset_temp = (((word_ >> 31) & 0x1) << 12) |
                           (((word_ >> 25) & 0x3F) << 5) |
                           (((word_ >> 8)  & 0xF)  << 1) |
                           (((word_ >> 7)  & 0x1)  << 11);
    int32_t imm = ((word_ >> 31) == 0)
                       ? static_cast<int32_t>(offset_temp)
                       : static_cast<int32_t>(offset_temp | 0xFFFFE000);

    bool take = false;
    uint32_t op1 = cpu->cpuregs[rs1];
    uint32_t op2 = cpu->cpuregs[rs2];

    switch (funct3) {
    case 0b000: // BEQ
        take = (op1 == op2);
        cpu->instr_name = (char*)"beq";
        break;
    case 0b001: // BNE
        take = (op1 != op2);
        cpu->instr_name = (char*)"bne";
        break;
    case 0b100: // BLT
        take = ((int32_t)op1 < (int32_t)op2);
        cpu->instr_name = (char*)"blt";
        break;
    case 0b101: // BGE
        take = ((int32_t)op1 >= (int32_t)op2);
        cpu->instr_name = (char*)"bge";
        break;
    case 0b110: // BLTU
        take = (op1 < op2);
        cpu->instr_name = (char*)"bltu";
        break;
    case 0b111: // BGEU
        take = (op1 >= op2);
        cpu->instr_name = (char*)"bgeu";
        break;
    default:
        cpu->instr_name = (char*)"illegal_branch";
    }

    cpu->next_pc = take ? cpu->pc + imm : cpu->pc + 4;
}

} 