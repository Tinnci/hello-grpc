#include "decoder/Decoder.h"
#include "decoder/AddInstruction.h"
#include "decoder/ArithmeticRInstruction.h"
#include "decoder/MulDivInstruction.h"
#include "decoder/ITypeImmInstruction.h"

namespace Decoder {

std::unique_ptr<Instruction> Decoder::decode(uint32_t word) {
    uint8_t opcode = word & 0x7F;

    // 仅实现 R-Type (opcode 0b0110011) 中的 ADD 指令作为 PoC
    if (opcode == 0b0110011) {
        uint8_t funct7 = (word >> 25) & 0x7F;
        if (funct7 == 0b0000001) {
            return std::make_unique<MulDivInstruction>(word);
        }
        else {
            return std::make_unique<ArithmeticRInstruction>(word);
        }
    }

    // I-Type immediate arithmetic
    if (opcode == 0b0010011) {
        return std::make_unique<ITypeImmInstruction>(word);
    }

    // 其他指令暂未实现，返回 nullptr 触发旧路径
    return nullptr;
}

} // namespace Decoder 