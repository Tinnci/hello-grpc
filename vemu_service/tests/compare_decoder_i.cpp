#include "RISCV.h"
#include "decoder/ITypeImmInstruction.h"
#include <cassert>
#include <cstdlib>
#include <ctime>
#include <iostream>
#include <memory>

using namespace Decoder;
class TestEmu : public Emulator { public: void init_param() override {} };

struct Case { uint8_t funct3; uint8_t funct7; const char* name; };
static const Case cases[] = {
    {0b000, 0, "addi"}, {0b010, 0, "slti"}, {0b011, 0, "sltiu"},
    {0b100, 0, "xori"}, {0b110, 0, "ori"},  {0b111, 0, "andi"},
    {0b001, 0, "slli"}, {0b101, 0, "srli"}, {0b101, 0b0100000, "srai"},
};

int main() {
    std::srand(static_cast<unsigned>(std::time(nullptr)));
    const int iterations = 1000;

    for (const auto &c : cases) {
        auto em_new = std::make_unique<TestEmu>();
        auto em_old = std::make_unique<TestEmu>();
        em_new->verbose = false;
        em_old->verbose = false;

        for (int i = 0; i < iterations; ++i) {
            uint32_t op1 = static_cast<uint32_t>(std::rand());
            uint32_t imm = static_cast<uint32_t>(std::rand()) & 0xFFF; // 12-bit immediate
            // ensure shift amount for shift ops
            if (c.funct3 == 0b001 || c.funct3 == 0b101) {
                imm = (c.funct7 << 5) | (imm & 0x1F);
            }
            uint32_t word = (c.funct7 << 25) | (imm << 20) | (1u << 15) | (c.funct3 << 12) | (5u << 7) | 0b0010011;

            em_new->cpuregs[1] = op1;
            em_old->cpuregs[1] = op1;

            // new
            {
                ITypeImmInstruction inst(word);
                em_new->pc = 0;
                inst.execute(em_new.get());
            }

            // old
            {
                em_old->instr = word;
                em_old->rd = 5;
                em_old->rs1 = 1;
                em_old->decode_arthimetic_imm();
            }

            assert(em_new->cpuregs[5] == em_old->cpuregs[5]);
        }
        std::cout << "Case " << c.name << " passed." << std::endl;
    }
    std::cout << "All I-Type immediate tests passed!" << std::endl;
    return 0;
} 