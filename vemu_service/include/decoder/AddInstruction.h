#ifndef VEMU_DECODER_ADD_INSTRUCTION_H
#define VEMU_DECODER_ADD_INSTRUCTION_H

#include "decoder/Instruction.h"
#include "RISCV.h" // 需要 Emulator 定义

namespace Decoder {

class AddInstruction : public Instruction {
public:
    explicit AddInstruction(uint32_t word);
    void execute(Emulator* cpu) override;

private:
    uint8_t rd_;
    uint8_t rs1_;
    uint8_t rs2_;
};

} // namespace Decoder

#endif // VEMU_DECODER_ADD_INSTRUCTION_H 