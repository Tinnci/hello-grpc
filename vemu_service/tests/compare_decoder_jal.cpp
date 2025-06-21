#include "RISCV.h"
#include "decoder/JalInstruction.h"
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

static uint32_t encode_jal(uint8_t rd, int32_t imm) {
    // imm 必须为 21 位、LSB 恒 0（跳转按字节寻址）
    uint32_t word = 0;
    word |= ((imm >> 20) & 0x1) << 31;          // imm[20]
    word |= ((imm >> 1) & 0x3FF) << 21;          // imm[10:1]
    word |= ((imm >> 11) & 0x1) << 20;           // imm[11]
    word |= ((imm >> 12) & 0xFF) << 12;          // imm[19:12]
    word |= (rd & 0x1F) << 7;                    // rd
    word |= 0b1101111;                           // opcode 0x6F
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

        // 生成随机且 4 字节对齐的 PC
        uint32_t pc = (std::rand() & 0xFFFF) << 2;
        em_new->pc = pc;
        em_old->pc = pc;

        // 随机 rd
        uint8_t rd = std::rand() % 32;

        // 随机 21 位立即数（偶数）
        int32_t imm = (std::rand() % (1 << 20));
        if (std::rand() & 1) imm = -imm;
        imm <<= 1; // LSB 恒 0

        uint32_t word = encode_jal(rd, imm);

        // 新路径
        {
            JalInstruction inst(word);
            inst.execute(em_new.get());
        }

        // 旧路径
        {
            em_old->instr = word;
            em_old->rd = rd;
            em_old->decode_jal();
        }

        assert(em_new->next_pc == em_old->next_pc);
        assert(em_new->cpuregs[rd] == em_old->cpuregs[rd]);
        assert(std::string(em_new->instr_name) == std::string(em_old->instr_name));
    }

    std::cout << "All JAL comparison tests passed!" << std::endl;
    return 0;
} 