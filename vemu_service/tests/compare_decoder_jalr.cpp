#include "RISCV.h"
#include "decoder/JalrInstruction.h"
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

static uint32_t encode_jalr(uint8_t rd, uint8_t rs1, int32_t imm) {
    uint32_t word = 0;
    word |= (imm & 0xFFF) << 20; // imm[11:0]
    word |= (rs1 & 0x1F) << 15;  // rs1
    word |= 0b000 << 12;         // funct3 = 0
    word |= (rd & 0x1F) << 7;    // rd
    word |= 0b1100111;           // opcode 0x67
    return word;
}

int main() {
    std::srand(static_cast<unsigned>(std::time(nullptr)));
    const int iterations = 1000;

    for (int i = 0; i < iterations; ++i) {
        auto em_new = std::make_unique<TestEmu>();
        auto em_old = std::make_unique<TestEmu>();
        em_new->verbose = false;
        em_old->verbose = false;

        uint32_t pc = (std::rand() & 0xFFFF) << 2;
        em_new->pc = pc;
        em_old->pc = pc;

        uint8_t rd = std::rand() % 32;
        uint8_t rs1 = 1; // 固定使用 x1，便于校验

        uint32_t base = (std::rand() & 0xFFFFFFFC); // 保证最低 2 位为 0
        em_new->cpuregs[rs1] = base;
        em_old->cpuregs[rs1] = base;

        int32_t imm = (std::rand() % 2048);
        if (std::rand() & 1) imm = -imm;
        imm &= ~1; // 立即数也保证偶数，确保旧实现 & ~1 差异被屏蔽

        uint32_t word = encode_jalr(rd, rs1, imm);

        // 新路径
        {
            JalrInstruction inst(word);
            inst.execute(em_new.get());
        }

        // 旧路径
        {
            em_old->instr = word;
            em_old->rd = rd;
            em_old->rs1 = rs1;
            em_old->decode_jalr();
        }

        if (em_new->next_pc != em_old->next_pc) {
            std::cerr << "Mismatch next_pc: base=" << std::hex << base
                      << " imm=" << imm << " new=" << em_new->next_pc
                      << " old=" << em_old->next_pc << std::dec << std::endl;
        }
        assert(em_new->next_pc == em_old->next_pc);
        assert(em_new->cpuregs[rd] == em_old->cpuregs[rd]);
        assert(std::string(em_new->instr_name) == std::string(em_old->instr_name));
    }

    std::cout << "All JALR comparison tests passed!" << std::endl;
    return 0;
} 