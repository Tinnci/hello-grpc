#ifndef VEMU_DECODER_INSTRUCTION_H
#define VEMU_DECODER_INSTRUCTION_H

#include <cstdint>

class Emulator; // 前向声明，避免循环依赖

namespace Decoder {

// 指令抽象基类，所有具体指令需继承并实现 execute()
class Instruction {
public:
    explicit Instruction(uint32_t word) : word_(word) {}
    virtual ~Instruction() = default;

    // 执行指令
    virtual void execute(Emulator* cpu) = 0;

protected:
    uint32_t word_;
};

} // namespace Decoder

#endif // VEMU_DECODER_INSTRUCTION_H 