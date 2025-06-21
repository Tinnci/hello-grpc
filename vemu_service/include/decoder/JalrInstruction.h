#ifndef VEMU_DECODER_JALRINSTRUCTION_H
#define VEMU_DECODER_JALRINSTRUCTION_H

#include "decoder/Instruction.h"

namespace Decoder {

class JalrInstruction : public Instruction {
public:
    explicit JalrInstruction(uint32_t word) : Instruction(word) {}
    void execute(Emulator* cpu) override;
};

} // namespace Decoder

#endif 