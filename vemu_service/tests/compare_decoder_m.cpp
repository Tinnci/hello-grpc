#include "RISCV.h"
#include "decoder/MulDivInstruction.h"
#include <cassert>
#include <cstdlib>
#include <ctime>
#include <iostream>
#include <memory>

using namespace Decoder;

class TestEmu : public Emulator {
public:
    void init_param() override {}
};

struct Case { uint8_t funct3; const char* name; };
static const Case cases[] = {
    {0b000, "mul"}, {0b001, "mulh"}, {0b010, "mulhsu"}, {0b011, "mulhu"},
    {0b100, "div"}, {0b101, "divu"}, {0b110, "rem"}, {0b111, "remu"},
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

            em_new->cpuregs[1] = op1;
            em_new->cpuregs[2] = op2;
            em_old->cpuregs[1] = op1;
            em_old->cpuregs[2] = op2;

            uint32_t word = (0b0000001u << 25) |
                             (2u << 20) | (1u << 15) |
                             (static_cast<uint32_t>(c.funct3) << 12) |
                             (5u << 7) | 0b0110011;

            // 新路径
            {
                MulDivInstruction inst(word);
                em_new->pc = 0;
                inst.execute(em_new.get());
            }

            // 旧路径
            {
                em_old->instr = word;
                em_old->rd = 5;
                em_old->rs1 = 1;
                em_old->rs2 = 2;
                em_old->decode_RV32M();
            }

            assert(em_new->cpuregs[5] == em_old->cpuregs[5]);
        }
        std::cout << "Case " << c.name << " passed." << std::endl;
    }
    std::cout << "All M-Extension comparison tests passed!" << std::endl;
    return 0;
} 