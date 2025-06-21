#ifndef VEMU_DECODER_LOADINSTRUCTION_H
#define VEMU_DECODER_LOADINSTRUCTION_H

#include "decoder/Instruction.h"

namespace Decoder {

class LoadInstruction : public Instruction {
public:
    explicit LoadInstruction(uint32_t word) : Instruction(word) {}
    void execute(Emulator* cpu) override;
};

} // namespace Decoder

#endif 