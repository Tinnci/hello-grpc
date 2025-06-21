#include "decoder/EbreakInstruction.h"
#include "RISCV.h"

namespace Decoder {

void EbreakInstruction::execute(Emulator* cpu) {
    // EBREAK cause = 3 per RISC-V Privileged Spec
    cpu->raise_trap(3);
}

} // namespace Decoder 