#include "decoder/ITypeImmInstruction.h"
#include <cstdint>

namespace Decoder {

static inline int32_t sign_extend12(uint32_t value) {
    return (value & 0x800) ? static_cast<int32_t>(value | 0xFFFFF000) : static_cast<int32_t>(value);
}

ITypeImmInstruction::ITypeImmInstruction(uint32_t word)
    : Instruction(word) {
    rd_ = (word >> 7) & 0x1F;
    funct3_ = (word >> 12) & 0x7;
    rs1_ = (word >> 15) & 0x1F;
    imm_raw_ = (word >> 20) & 0xFFF;
    imm_signed_ = sign_extend12(imm_raw_);
    shamt5_ = imm_raw_ & 0x1F;
    funct7_ = (word >> 25) & 0x7F;
}

void ITypeImmInstruction::execute(Emulator* cpu) {
    uint32_t result = 0;
    switch (funct3_) {
    case 0b000: // ADDI
        result = cpu->cpuregs[rs1_] + static_cast<uint32_t>(imm_signed_);
        cpu->instr_name = (char*)"addi";
        break;
    case 0b010: // SLTI
        result = static_cast<int32_t>(cpu->cpuregs[rs1_]) < imm_signed_;
        cpu->instr_name = (char*)"slti";
        break;
    case 0b011: // SLTIU
        result = cpu->cpuregs[rs1_] < static_cast<uint32_t>(imm_signed_);
        cpu->instr_name = (char*)"sltiu";
        break;
    case 0b100: // XORI
        result = cpu->cpuregs[rs1_] ^ static_cast<uint32_t>(imm_signed_);
        cpu->instr_name = (char*)"xori";
        break;
    case 0b110: // ORI
        result = cpu->cpuregs[rs1_] | static_cast<uint32_t>(imm_signed_);
        cpu->instr_name = (char*)"ori";
        break;
    case 0b111: // ANDI
        result = cpu->cpuregs[rs1_] & static_cast<uint32_t>(imm_signed_);
        cpu->instr_name = (char*)"andi";
        break;
    case 0b001: // SLLI
        result = cpu->cpuregs[rs1_] << shamt5_;
        cpu->instr_name = (char*)"slli";
        break;
    case 0b101: // SRLI or SRAI
        if (funct7_ == 0b0000000) {
            result = cpu->cpuregs[rs1_] >> shamt5_;
            cpu->instr_name = (char*)"srli";
        } else if (funct7_ == 0b0100000) {
            result = static_cast<uint32_t>(static_cast<int32_t>(cpu->cpuregs[rs1_]) >> shamt5_);
            cpu->instr_name = (char*)"srai";
        } else {
            cpu->instr_name = (char*)"illegal";
        }
        break;
    default:
        cpu->instr_name = (char*)"illegal";
        break;
    }

    if (rd_ != 0) {
        cpu->cpuregs[rd_] = result;
    }
    cpu->next_pc = cpu->pc + 4;
}

} // namespace Decoder 