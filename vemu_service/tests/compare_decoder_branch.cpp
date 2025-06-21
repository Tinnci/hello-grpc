#include "RISCV.h"
#include "decoder/BranchInstruction.h"
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

struct Case {
    uint8_t funct3;
    const char* name;
};

static const Case cases[] = {
    {0b000, "beq"},
    {0b001, "bne"},
    {0b100, "blt"},
    {0b101, "bge"},
    {0b110, "bltu"},
    {0b111, "bgeu"},
};

static uint32_t encode_branch(uint8_t funct3, uint8_t rs1, uint8_t rs2, int32_t imm) {
    // imm 为 13 位二进制补码，LSB 必为 0
    uint32_t imm13 = static_cast<uint32_t>(imm) & 0x1FFFu; // 取低 13 位
    uint32_t word = 0;
    word |= ((imm13 >> 12) & 0x1) << 31;        // imm[12]
    word |= ((imm13 >> 5)  & 0x3F) << 25;       // imm[10:5]
    word |= (static_cast<uint32_t>(rs2 & 0x1F) << 20);
    word |= (static_cast<uint32_t>(rs1 & 0x1F) << 15);
    word |= (funct3 & 0x7) << 12;
    word |= ((imm13 >> 1)  & 0xF) << 8;         // imm[4:1]
    word |= ((imm13 >> 11) & 0x1) << 7;         // imm[11]
    word |= 0b1100011;                          // opcode
    return word;
}

int main() {
    std::srand(static_cast<unsigned>(std::time(nullptr)));
    const int iterations = 1000;

    for (const auto& c : cases) {
        auto em_new = std::make_unique<TestEmu>();
        auto em_old = std::make_unique<TestEmu>();
        em_new->verbose = false;
        em_old->verbose = false;

        for (int i = 0; i < iterations; ++i) {
            uint8_t rs1 = 1; // 使用固定寄存器，便于比较
            uint8_t rs2 = 2;

            uint32_t op1 = static_cast<uint32_t>(std::rand());
            uint32_t op2 = static_cast<uint32_t>(std::rand());

            em_new->cpuregs[rs1] = op1;
            em_new->cpuregs[rs2] = op2;
            em_old->cpuregs[rs1] = op1;
            em_old->cpuregs[rs2] = op2;

            uint32_t pc = (std::rand() & 0xFFFF) << 2;
            em_new->pc = pc;
            em_old->pc = pc;

            // 13 位立即数有符号范围 [-4096, 4094]，且 LSB 为 0
            int32_t imm_raw = (std::rand() % 8192) - 4096; // [-4096, 4095]
            if (imm_raw == 4095) imm_raw = -4096; // ensure even result after <<1
            int32_t imm = imm_raw << 1; // 保证偶数并落在 [-8192, 8188]

            uint32_t word = encode_branch(c.funct3, rs1, rs2, imm);

            // 新路径
            {
                BranchInstruction inst(word);
                inst.execute(em_new.get());
            }

            // 旧路径
            {
                em_old->instr = word;
                em_old->rs1 = rs1;
                em_old->rs2 = rs2;

                // 关键修正：让旧路径也从指令 word 解码 offset，与真实 emulate() 行为一致
                uint32_t offset_temp = (((word >> 31) & 0x1) << 12) |
                                       (((word >> 25) & 0x3F) << 5) |
                                       (((word >> 8)  & 0xF)  << 1) |
                                       (((word >> 7)  & 0x1)  << 11);
                em_old->branch_offset = ((word >> 31) == 0)
                                       ? static_cast<int32_t>(offset_temp)
                                       : static_cast<int32_t>(offset_temp | 0xFFFFE000);

                em_old->decode_branch();
            }

            if (em_new->next_pc != em_old->next_pc) {
                std::cerr << "Mismatch on " << c.name
                          << " iteration=" << i
                          << " pc=" << std::hex << pc
                          << " imm=" << imm
                          << " op1=" << op1 << " op2=" << op2
                          << " new_pc=" << em_new->next_pc
                          << " old_pc=" << em_old->next_pc << std::dec << std::endl;
            }
            assert(em_new->next_pc == em_old->next_pc);
            assert(std::string(em_new->instr_name) == std::string(em_old->instr_name));
        }
        std::cout << "Case " << c.name << " passed." << std::endl;
    }

    std::cout << "All Branch comparison tests passed!" << std::endl;
    return 0;
} 