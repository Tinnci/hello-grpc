#ifndef VEMU_DECODER_STOREINSTRUCTION_H
#define VEMU_DECODER_STOREINSTRUCTION_H

#include "decoder/Instruction.h"

namespace Decoder {

class StoreInstruction : public Instruction {
public:
    explicit StoreInstruction(uint32_t word) : Instruction(word) {}
    void execute(Emulator* cpu) override;
};

} // namespace Decoder

#endif 