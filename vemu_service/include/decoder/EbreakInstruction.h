#ifndef VEMU_DECODER_EBREAKINSTRUCTION_H
#define VEMU_DECODER_EBREAKINSTRUCTION_H

#include "decoder/Instruction.h"

namespace Decoder {

class EbreakInstruction : public Instruction {
public:
    explicit EbreakInstruction(uint32_t word) : Instruction(word) {}
    void execute(Emulator* cpu) override;
};

} // namespace Decoder

#endif // VEMU_DECODER_EBREAKINSTRUCTION_H 