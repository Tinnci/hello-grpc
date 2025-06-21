#include "RISCV.h"
#include "decoder/ArithmeticRInstruction.h"
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

static uint32_t encode_r(uint8_t funct7, uint8_t funct3, uint8_t rs2, uint8_t rs1, uint8_t rd) {
    return (static_cast<uint32_t>(funct7) << 25) |
           (static_cast<uint32_t>(rs2)   << 20) |
           (static_cast<uint32_t>(rs1)   << 15) |
           (static_cast<uint32_t>(funct3)<< 12) |
           (static_cast<uint32_t>(rd)    << 7)  |
           0b0110011;
}

static uint32_t compute_expected(uint32_t op1, uint32_t op2, uint8_t funct3, uint8_t funct7) {
    switch (funct3) {
    case 0b000: // ADD / SUB
        return (funct7 == 0b0100000) ?
               static_cast<uint32_t>(static_cast<int32_t>(op1) - static_cast<int32_t>(op2)) :
               op1 + op2;
    case 0b001: // SLL
        return op1 << (op2 & 0x1F);
    case 0b010: // SLT
        return static_cast<int32_t>(op1) < static_cast<int32_t>(op2);
    case 0b011: // SLTU
        return op1 < op2;
    case 0b100: // XOR
        return op1 ^ op2;
    case 0b101: // SRL / SRA
        if (funct7 == 0b0100000) {
            return static_cast<uint32_t>(static_cast<int32_t>(op1) >> (op2 & 0x1F));
        } else {
            return op1 >> (op2 & 0x1F);
        }
    case 0b110: // OR
        return op1 | op2;
    case 0b111: // AND
        return op1 & op2;
    default:
        return 0;
    }
}

struct Case { uint8_t funct3; uint8_t funct7; const char* name; };
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

    for (const auto &c : cases) {
        for (int i = 0; i < iterations; ++i) {
            auto em = std::make_unique<TestEmu>();
            uint32_t op1 = static_cast<uint32_t>(std::rand());
            uint32_t op2 = static_cast<uint32_t>(std::rand());
            em->cpuregs[1] = op1;
            em->cpuregs[2] = op2;
            uint32_t word = encode_r(c.funct7, c.funct3, /*rs2=*/2, /*rs1=*/1, /*rd=*/5);
            ArithmeticRInstruction inst(word);
            em->pc = 0;
            inst.execute(em.get());
            uint32_t expected = compute_expected(op1, op2, c.funct3, c.funct7);
            assert(em->cpuregs[5] == expected);
            assert(em->next_pc == 4);
        }
        std::cout << "R-Type case " << c.name << " passed." << std::endl;
    }
    std::cout << "All R-Type tests passed!" << std::endl;
    return 0;
} 