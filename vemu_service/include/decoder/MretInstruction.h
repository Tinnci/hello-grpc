#ifndef VEMU_DECODER_MRETINSTRUCTION_H
#define VEMU_DECODER_MRETINSTRUCTION_H

#include "decoder/Instruction.h"

namespace Decoder {

class MretInstruction : public Instruction {
public:
    explicit MretInstruction(uint32_t word) : Instruction(word) {}
    void execute(Emulator* cpu) override;
};

} // namespace Decoder

#endif // VEMU_DECODER_MRETINSTRUCTION_H 