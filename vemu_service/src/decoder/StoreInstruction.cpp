#include "decoder/StoreInstruction.h"
#include "RISCV.h"

namespace Decoder {

void StoreInstruction::execute(Emulator* cpu) {
    uint32_t imm11_5 = (word_ >> 25) & 0x7F;
    uint32_t imm4_0 = (word_ >> 7) & 0x1F;
    uint32_t imm = (imm11_5 << 5) | imm4_0;
    if (imm & 0x800) imm |= 0xFFFFF000; // sign extend

    uint8_t rs1 = (word_ >> 15) & 0x1F;
    uint8_t rs2 = (word_ >> 20) & 0x1F;
    uint8_t funct3 = (word_ >> 12) & 0x7;

    uint32_t addr = cpu->cpuregs[rs1] + imm;

    switch (funct3) {
    case 0b000: // SB
        cpu->mmu.write_byte(addr, static_cast<uint8_t>(cpu->cpuregs[rs2] & 0xFF));
        cpu->instr_name = (char*)"sb";
        break;
    case 0b001: // SH
        cpu->mmu.write_half(addr, static_cast<uint16_t>(cpu->cpuregs[rs2] & 0xFFFF));
        cpu->instr_name = (char*)"sh";
        break;
    case 0b010: // SW
        cpu->mmu.write_word(addr, cpu->cpuregs[rs2]);
        cpu->instr_name = (char*)"sw";
        break;
    default:
        cpu->instr_name = (char*)"unimp_store";
    }
    cpu->next_pc = cpu->pc + 4;
}

} 