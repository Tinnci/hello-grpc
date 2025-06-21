#include "decoder/MulDivInstruction.h"
#include <cstdint>
#include <cinttypes>

namespace Decoder {

MulDivInstruction::MulDivInstruction(uint32_t word)
    : Instruction(word) {
    rd_     = (word >> 7) & 0x1F;
    funct3_ = (word >> 12) & 0x7;
    rs1_    = (word >> 15) & 0x1F;
    rs2_    = (word >> 20) & 0x1F;
}

static inline int32_t sign_extend32(uint32_t v) {
    return static_cast<int32_t>(v);
}

void MulDivInstruction::execute(Emulator* cpu) {
    uint32_t result = 0;
    int32_t op1_s  = sign_extend32(cpu->cpuregs[rs1_]);
    int32_t op2_s  = sign_extend32(cpu->cpuregs[rs2_]);
    uint32_t op1_u = cpu->cpuregs[rs1_];
    uint32_t op2_u = cpu->cpuregs[rs2_];

    switch (funct3_) {
    case 0b000: { // MUL (low 32 bits)
        int64_t mul = static_cast<int64_t>(op1_s) * static_cast<int64_t>(op2_s);
        result = static_cast<uint32_t>(mul & 0xFFFFFFFF);
        cpu->instr_name = (char*)"mul";
        break;
    }
    case 0b001: { // MULH signed * signed high
        int64_t mul = static_cast<int64_t>(op1_s) * static_cast<int64_t>(op2_s);
        result = static_cast<uint32_t>((mul >> 32) & 0xFFFFFFFF);
        cpu->instr_name = (char*)"mulh";
        break;
    }
    case 0b010: { // MULHSU signed * unsigned high
        int64_t mul = static_cast<int64_t>(op1_s) * static_cast<int64_t>(op2_u);
        result = static_cast<uint32_t>((mul >> 32) & 0xFFFFFFFF);
        cpu->instr_name = (char*)"mulhsu";
        break;
    }
    case 0b011: { // MULHU unsigned * unsigned high
        uint64_t mul = static_cast<uint64_t>(op1_u) * static_cast<uint64_t>(op2_u);
        result = static_cast<uint32_t>((mul >> 32) & 0xFFFFFFFF);
        cpu->instr_name = (char*)"mulhu";
        break;
    }
    case 0b100: { // DIV (signed)
        if (op2_s == 0) {
            result = 0xFFFFFFFF;
        } else if (op1_s == INT32_MIN && op2_s == -1) {
            result = static_cast<uint32_t>(INT32_MIN);
        } else {
            result = static_cast<uint32_t>(op1_s / op2_s);
        }
        cpu->instr_name = (char*)"div";
        break;
    }
    case 0b101: { // DIVU (unsigned)
        if (op2_u == 0) {
            result = 0xFFFFFFFF;
        } else {
            result = op1_u / op2_u;
        }
        cpu->instr_name = (char*)"divu";
        break;
    }
    case 0b110: { // REM (signed)
        if (op2_s == 0) {
            result = static_cast<uint32_t>(op1_s);
        } else if (op1_s == INT32_MIN && op2_s == -1) {
            result = 0;
        } else {
            result = static_cast<uint32_t>(op1_s % op2_s);
        }
        cpu->instr_name = (char*)"rem";
        break;
    }
    case 0b111: { // REMU (unsigned)
        if (op2_u == 0) {
            result = op1_u;
        } else {
            result = op1_u % op2_u;
        }
        cpu->instr_name = (char*)"remu";
        break;
    }
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