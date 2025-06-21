#include "decoder/Decoder.h"
#include "decoder/AddInstruction.h"
#include "decoder/ArithmeticRInstruction.h"

namespace Decoder {

std::unique_ptr<Instruction> Decoder::decode(uint32_t word) {
    uint8_t opcode = word & 0x7F;

    // 仅实现 R-Type (opcode 0b0110011) 中的 ADD 指令作为 PoC
    if (opcode == 0b0110011) {
        uint8_t funct7 = (word >> 25) & 0x7F;
        if (funct7 != 0b0000001) { // 排除 M 扩展暂未支持
            return std::make_unique<ArithmeticRInstruction>(word);
        }
        // M 扩展保留
        return nullptr;
    }

    // 其他指令暂未实现，返回 nullptr 触发旧路径
    return nullptr;
}

} // namespace Decoder 