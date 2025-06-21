#ifndef VEMU_DECODER_JALINSTRUCTION_H
#define VEMU_DECODER_JALINSTRUCTION_H

#include "decoder/Instruction.h"

namespace Decoder {

class JalInstruction : public Instruction {
public:
    explicit JalInstruction(uint32_t word) : Instruction(word) {}
    void execute(Emulator* cpu) override;
};

} // namespace Decoder

#endif 