#ifndef VEMU_DECODER_DECODER_H
#define VEMU_DECODER_DECODER_H

#include <memory>
#include "decoder/Instruction.h"

namespace Decoder {

class Decoder {
public:
    // 根据指令字解码，返回对应指令对象；若不支持返回 nullptr
    std::unique_ptr<Instruction> decode(uint32_t word);
};

} // namespace Decoder

#endif // VEMU_DECODER_DECODER_H 