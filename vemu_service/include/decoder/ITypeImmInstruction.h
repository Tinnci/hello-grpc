#ifndef VEMU_DECODER_ITYPE_IMM_INSTRUCTION_H
#define VEMU_DECODER_ITYPE_IMM_INSTRUCTION_H

#include "decoder/Instruction.h"
#include "RISCV.h"

namespace Decoder {

class ITypeImmInstruction : public Instruction {
public:
    explicit ITypeImmInstruction(uint32_t word);
    void execute(Emulator* cpu) override;
private:
    uint8_t rd_;
    uint8_t rs1_;
    uint32_t imm_raw_;
    int32_t imm_signed_;
    uint8_t funct3_;
    uint8_t shamt5_;
    uint8_t funct7_;
};

} // namespace Decoder

#endif // VEMU_DECODER_ITYPE_IMM_INSTRUCTION_H 