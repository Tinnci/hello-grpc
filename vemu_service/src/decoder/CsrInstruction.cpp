#include "decoder/CsrInstruction.h"

namespace Decoder {

CsrInstruction::CsrInstruction(uint32_t word)
    : Instruction(word) {
    rd_ = (word >> 7) & 0x1F;
    funct3_ = (word >> 12) & 0x7;
    csr_addr_ = static_cast<uint16_t>((word >> 20) & 0xFFF);
    rs1_ = (word >> 15) & 0x1F;
    zimm5_ = rs1_; // overlap for immediate forms
}

void CsrInstruction::execute(Emulator* cpu) {
    uint32_t csr_old = cpu->csr.read(csr_addr_, cpu);
    uint32_t write_val = 0;
    bool do_write = false;
    uint32_t rs1_val = cpu->cpuregs[rs1_];

    switch (funct3_) {
    case 0b001: // CSRRW
        write_val = rs1_val;
        do_write = true;
        cpu->instr_name = (char*)"csrrw";
        break;
    case 0b010: // CSRRS
        write_val = csr_old | rs1_val;
        do_write = (rs1_ != 0);
        cpu->instr_name = (char*)"csrrs";
        break;
    case 0b011: // CSRRC
        write_val = csr_old & (~rs1_val);
        do_write = (rs1_ != 0);
        cpu->instr_name = (char*)"csrrc";
        break;
    case 0b101: // CSRRWI
        write_val = zimm5_;
        do_write = true;
        cpu->instr_name = (char*)"csrrwi";
        break;
    case 0b110: // CSRRSI
        write_val = csr_old | zimm5_;
        do_write = (zimm5_ != 0);
        cpu->instr_name = (char*)"csrrsi";
        break;
    case 0b111: // CSRRCI
        write_val = csr_old & (~zimm5_);
        do_write = (zimm5_ != 0);
        cpu->instr_name = (char*)"csrrci";
        break;
    default:
        cpu->instr_name = (char*)"illegal";
        break;
    }

    if (do_write) {
        cpu->csr.write(csr_addr_, write_val);
    }
    if (rd_ != 0) {
        cpu->cpuregs[rd_] = csr_old;
    }
    cpu->next_pc = cpu->pc + 4;
}

} // namespace Decoder 