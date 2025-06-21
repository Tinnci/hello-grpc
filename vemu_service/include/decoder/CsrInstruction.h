#ifndef VEMU_DECODER_CSR_INSTRUCTION_H
#define VEMU_DECODER_CSR_INSTRUCTION_H

#include "decoder/Instruction.h"
#include "RISCV.h"
#include "csr/CsrFile.h"

namespace Decoder {

class CsrInstruction : public Instruction {
public:
    explicit CsrInstruction(uint32_t word);
    void execute(Emulator* cpu) override;
private:
    uint8_t funct3_;
    uint16_t csr_addr_;
    uint8_t rd_;
    uint8_t rs1_;
    uint8_t zimm5_;
};

} // namespace Decoder

#endif // VEMU_DECODER_CSR_INSTRUCTION_H 