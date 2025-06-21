#include "decoder/LoadInstruction.h"
#include "RISCV.h"

namespace Decoder {

void LoadInstruction::execute(Emulator* cpu) {
    uint32_t imm = (int32_t)word_ >> 20; // sign-extend imm[11:0]
    uint8_t rd = (word_ >> 7) & 0x1F;
    uint8_t rs1 = (word_ >> 15) & 0x1F;
    uint32_t addr = cpu->cpuregs[rs1] + imm;
    uint8_t funct3 = (word_ >> 12) & 0x7;

    switch (funct3) {
    case 0b000: // LB
        cpu->cpuregs[rd] = cpu->mmu.read_byte(addr);
        cpu->instr_name = (char*)"lb";
        break;
    case 0b001: // LH
        cpu->cpuregs[rd] = cpu->mmu.read_half(addr);
        cpu->instr_name = (char*)"lh";
        break;
    case 0b010: // LW
        cpu->cpuregs[rd] = cpu->mmu.read_word(addr);
        cpu->instr_name = (char*)"lw";
        break;
    case 0b100: // LBU
        cpu->cpuregs[rd] = cpu->mmu.read_byte_u(addr);
        cpu->instr_name = (char*)"lbu";
        break;
    case 0b101: // LHU
        cpu->cpuregs[rd] = cpu->mmu.read_half_u(addr);
        cpu->instr_name = (char*)"lhu";
        break;
    default:
        cpu->instr_name = (char*)"unimp_load";
    }
    cpu->next_pc = cpu->pc + 4;
}

} 