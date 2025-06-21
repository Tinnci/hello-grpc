#ifndef VEMU_DECODER_BRANCHINSTRUCTION_H
#define VEMU_DECODER_BRANCHINSTRUCTION_H

#include "decoder/Instruction.h"

namespace Decoder {

class BranchInstruction : public Instruction {
public:
    explicit BranchInstruction(uint32_t word) : Instruction(word) {}
    void execute(Emulator* cpu) override;
};

} // namespace Decoder

#endif 