#include "decoder/Decoder.h"
#include "decoder/AddInstruction.h"
#include "decoder/ArithmeticRInstruction.h"
#include "decoder/MulDivInstruction.h"
#include "decoder/ITypeImmInstruction.h"
#include "decoder/CsrInstruction.h"
#include "decoder/EbreakInstruction.h"
#include "decoder/MretInstruction.h"
#include "decoder/LoadInstruction.h"
#include "decoder/StoreInstruction.h"
#include "decoder/JalInstruction.h"
#include "decoder/JalrInstruction.h"
#include "decoder/BranchInstruction.h"

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

    if (opcode == 0b0000011) { // LOAD
        return std::make_unique<LoadInstruction>(word);
    }
    if (opcode == 0b0100011) { // STORE
        return std::make_unique<StoreInstruction>(word);
    }

    if (opcode == 0b1101111) { // JAL
        return std::make_unique<JalInstruction>(word);
    }

    if (opcode == 0b1100111) { // JALR
        return std::make_unique<JalrInstruction>(word);
    }

    if (opcode == 0b1100011) { // BRANCH instructions
        return std::make_unique<BranchInstruction>(word);
    }

    if (opcode == 0b1110011) {
        uint8_t funct3 = (word >> 12) & 0x7;
        if (funct3 == 0b001 || funct3 == 0b010 || funct3 == 0b011 ||
            funct3 == 0b101 || funct3 == 0b110 || funct3 == 0b111) {
            return std::make_unique<CsrInstruction>(word);
        }
    }

    if (word == 0x00100073) {
        return std::make_unique<EbreakInstruction>(word);
    }

    if (word == 0x30200073) {
        return std::make_unique<MretInstruction>(word);
    }

    // 其他指令暂未实现，返回 nullptr 触发旧路径
    return nullptr;
}

} // namespace Decoder 