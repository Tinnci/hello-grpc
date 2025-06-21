#include "RISCV.h"
#include "decoder/ArithmeticRInstruction.h"
#include <cassert>
#include <cstdlib>
#include <ctime>
#include <iostream>
#include <memory> // Added for std::make_unique

using namespace Decoder;

class TestEmu : public Emulator {
public:
    void init_param() override {}
};

struct Case {
    uint8_t funct3;
    uint8_t funct7;
    const char* name;
};

static const Case cases[] = {
    {0b000, 0b0000000, "add"},
    {0b000, 0b0100000, "sub"},
    {0b001, 0b0000000, "sll"},
    {0b010, 0b0000000, "slt"},
    {0b011, 0b0000000, "sltu"},
    {0b100, 0b0000000, "xor"},
    {0b101, 0b0000000, "srl"},
    {0b101, 0b0100000, "sra"},
    {0b110, 0b0000000, "or"},
    {0b111, 0b0000000, "and"},
};

int main() {
    std::srand(static_cast<unsigned>(std::time(nullptr)));
    const int iterations = 1000;

    for (const auto& c : cases) {
        auto em_new = std::make_unique<TestEmu>();
        auto em_old = std::make_unique<TestEmu>();

        em_new->verbose = false;
        em_old->verbose = false;

        for (int i = 0; i < iterations; ++i) {
            uint32_t op1 = static_cast<uint32_t>(std::rand());
            uint32_t op2 = static_cast<uint32_t>(std::rand());

            // 统一寄存器设置
            em_new->cpuregs[1] = op1;
            em_new->cpuregs[2] = op2;
            em_old->cpuregs[1] = op1;
            em_old->cpuregs[2] = op2;

            uint32_t word = (static_cast<uint32_t>(c.funct7) << 25) |
                             (2u << 20) | (1u << 15) |
                             (static_cast<uint32_t>(c.funct3) << 12) |
                             (5u << 7) | 0b0110011;

            // 新路径
            {
                ArithmeticRInstruction inst(word);
                em_new->pc = 0;
                inst.execute(em_new.get());
            }

            // 旧路径
            {
                em_old->instr = word;
                em_old->rd = 5;
                em_old->rs1 = 1;
                em_old->rs2 = 2;
                em_old->decode_arthimetic_reg();
            }

            assert(em_new->cpuregs[5] == em_old->cpuregs[5]);
        }
        std::cout << "Case " << c.name << " passed." << std::endl;
    }
    std::cout << "All R-Type comparison tests passed!" << std::endl;
    return 0;
} 