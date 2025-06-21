#include "RISCV.h"
#include "decoder/ITypeImmInstruction.h"
#include <cassert>
#include <cstdlib>
#include <ctime>
#include <iostream>
#include <memory>

using namespace Decoder;
class TestEmu : public Emulator { public: void init_param() override {} };

static uint32_t encode_i(uint32_t imm12, uint8_t funct3, uint8_t rs1, uint8_t rd) {
    return (imm12 << 20) | (rs1 << 15) | (funct3 << 12) | (rd << 7) | 0b0010011;
}

static inline int32_t sign_extend12(uint32_t v) {
    return (v & 0x800) ? static_cast<int32_t>(v | 0xFFFFF000) : static_cast<int32_t>(v);
}

static uint32_t compute_expected(uint32_t op1, uint32_t imm_raw, uint8_t funct3, uint8_t funct7) {
    int32_t imm_signed = sign_extend12(imm_raw);
    switch (funct3) {
    case 0b000: return op1 + static_cast<uint32_t>(imm_signed); // ADDI
    case 0b010: return static_cast<int32_t>(op1) < imm_signed;   // SLTI
    case 0b011: return op1 < static_cast<uint32_t>(imm_signed); // SLTIU
    case 0b100: return op1 ^ static_cast<uint32_t>(imm_signed); // XORI
    case 0b110: return op1 | static_cast<uint32_t>(imm_signed); // ORI
    case 0b111: return op1 & static_cast<uint32_t>(imm_signed); // ANDI
    case 0b001: return op1 << (imm_raw & 0x1F);                 // SLLI
    case 0b101:
        if (funct7 == 0b0000000) // SRLI
            return op1 >> (imm_raw & 0x1F);
        else                      // SRAI
            return static_cast<uint32_t>(static_cast<int32_t>(op1) >> (imm_raw & 0x1F));
    default: return 0;
    }
}

struct Case { uint8_t funct3; uint8_t funct7; const char* name; };
static const Case cases[] = {
    {0b000, 0b0000000, "addi"}, {0b010, 0b0000000, "slti"}, {0b011, 0b0000000, "sltiu"},
    {0b100, 0b0000000, "xori"}, {0b110, 0b0000000, "ori"},  {0b111, 0b0000000, "andi"},
    {0b001, 0b0000000, "slli"}, {0b101, 0b0000000, "srli"}, {0b101, 0b0100000, "srai"},
};

int main() {
    std::srand(static_cast<unsigned>(std::time(nullptr)));
    const int iterations = 1000;
    for (const auto &c : cases) {
        for (int i = 0; i < iterations; ++i) {
            auto em = std::make_unique<TestEmu>();
            uint32_t op1 = static_cast<uint32_t>(std::rand());
            em->cpuregs[1] = op1;
            uint32_t imm_raw;
            if (c.funct3 == 0b001 || c.funct3 == 0b101) {
                uint32_t shamt = std::rand() & 0x1F; // 0..31 valid for RV32
                imm_raw = shamt;
                if (c.funct3 == 0b101 && c.funct7 == 0b0100000) {
                    imm_raw |= (c.funct7 << 5); // embed funct7 for SRAI
                }
            } else {
                imm_raw = std::rand() & 0xFFF;
            }
            uint32_t word = encode_i(imm_raw, c.funct3, /*rs1=*/1, /*rd=*/5);
            ITypeImmInstruction inst(word);
            inst.execute(em.get());
            uint32_t expected = compute_expected(op1, imm_raw & 0xFFF, c.funct3, c.funct7);
            assert(em->cpuregs[5] == expected);
            assert(em->next_pc == em->pc + 4);
        }
        std::cout << "I-Type case " << c.name << " passed." << std::endl;
    }
    std::cout << "All I-Type tests passed!" << std::endl;
    return 0;
} 