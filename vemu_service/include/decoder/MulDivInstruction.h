#ifndef VEMU_DECODER_MULDIV_INSTRUCTION_H
#define VEMU_DECODER_MULDIV_INSTRUCTION_H

#include "decoder/Instruction.h"
#include "RISCV.h"

namespace Decoder {

class MulDivInstruction : public Instruction {
public:
    explicit MulDivInstruction(uint32_t word);
    void execute(Emulator* cpu) override;
private:
    uint8_t rd_;
    uint8_t rs1_;
    uint8_t rs2_;
    uint8_t funct3_;
};

} // namespace Decoder

#endif // VEMU_DECODER_MULDIV_INSTRUCTION_H 