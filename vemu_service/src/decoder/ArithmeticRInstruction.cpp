#include "decoder/ArithmeticRInstruction.h"
#include "RISCV.h"

namespace Decoder {

ArithmeticRInstruction::ArithmeticRInstruction(uint32_t word)
    : Instruction(word) {
    rd_     = (word >> 7)  & 0x1F;
    funct3_ = (word >> 12) & 0x7;
    rs1_    = (word >> 15) & 0x1F;
    rs2_    = (word >> 20) & 0x1F;
    funct7_ = (word >> 25) & 0x7F;
}

void ArithmeticRInstruction::execute(Emulator* cpu) {
    uint32_t result = 0;
    switch (funct3_) {
    case 0b000: // ADD / SUB
        if (funct7_ == 0b0100000) { // SUB
            result = static_cast<int32_t>(cpu->cpuregs[rs1_]) - static_cast<int32_t>(cpu->cpuregs[rs2_]);
        } else { // ADD (funct7 == 0)
            result = cpu->cpuregs[rs1_] + cpu->cpuregs[rs2_];
        }
        break;
    case 0b001: // SLL
        result = cpu->cpuregs[rs1_] << (cpu->cpuregs[rs2_] & 0x1F);
        break;
    case 0b010: // SLT
        result = static_cast<int32_t>(cpu->cpuregs[rs1_]) < static_cast<int32_t>(cpu->cpuregs[rs2_]);
        break;
    case 0b011: // SLTU
        result = cpu->cpuregs[rs1_] < cpu->cpuregs[rs2_];
        break;
    case 0b100: // XOR
        result = cpu->cpuregs[rs1_] ^ cpu->cpuregs[rs2_];
        break;
    case 0b101: // SRL / SRA
        if (funct7_ == 0b0100000) { // SRA
            result = static_cast<int32_t>(cpu->cpuregs[rs1_]) >> (cpu->cpuregs[rs2_] & 0x1F);
        } else { // SRL
            result = cpu->cpuregs[rs1_] >> (cpu->cpuregs[rs2_] & 0x1F);
        }
        break;
    case 0b110: // OR
        result = cpu->cpuregs[rs1_] | cpu->cpuregs[rs2_];
        break;
    case 0b111: // AND
        result = cpu->cpuregs[rs1_] & cpu->cpuregs[rs2_];
        break;
    default:
        // 未知组合，保持 cpu->next_pc 更新后返回
        break;
    }

    if (rd_ != 0) {
        cpu->cpuregs[rd_] = result;
    }

    // 更新 PC
    cpu->next_pc = cpu->pc + 4;

    // 设置指令名称（可选，用于调试）
    switch (funct3_) {
    case 0: cpu->instr_name = (char*)((funct7_ == 0b0100000) ? "sub" : "add"); break;
    case 1: cpu->instr_name = (char*)"sll"; break;
    case 2: cpu->instr_name = (char*)"slt"; break;
    case 3: cpu->instr_name = (char*)"sltu"; break;
    case 4: cpu->instr_name = (char*)"xor"; break;
    case 5: cpu->instr_name = (char*)((funct7_ == 0b0100000) ? "sra" : "srl"); break;
    case 6: cpu->instr_name = (char*)"or"; break;
    case 7: cpu->instr_name = (char*)"and"; break;
    default: cpu->instr_name = (char*)"unknown"; break;
    }
}

} // namespace Decoder 