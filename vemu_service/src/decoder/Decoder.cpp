#include "decoder/Decoder.h"
#include "decoder/AddInstruction.h"

namespace Decoder {

std::unique_ptr<Instruction> Decoder::decode(uint32_t word) {
    uint8_t opcode = word & 0x7F;

    // 仅实现 R-Type (opcode 0b0110011) 中的 ADD 指令作为 PoC
    if (opcode == 0b0110011) {
        uint8_t funct3 = (word >> 12) & 0x7;
        uint8_t funct7 = (word >> 25) & 0x7F;
        if (funct3 == 0 && funct7 == 0) {
            return std::make_unique<AddInstruction>(word);
        }
    }

    // 其他指令暂未实现，返回 nullptr 触发旧路径
    return nullptr;
}

} // namespace Decoder 