#include "RISCV.h"
#include "decoder/MulDivInstruction.h"
#include <cassert>
#include <cstdlib>
#include <ctime>
#include <iostream>
#include <memory>

using namespace Decoder;
class TestEmu : public Emulator { public: void init_param() override {} };

static uint32_t encode_m(uint8_t funct3, uint8_t rs2, uint8_t rs1, uint8_t rd) {
    uint8_t funct7 = 0b0000001; // fixed for M extension
    return (static_cast<uint32_t>(funct7) << 25) |
           (rs2 << 20) | (rs1 << 15) |
           (funct3 << 12) | (rd << 7) | 0b0110011;
}

static uint32_t hi32(uint64_t v) { return static_cast<uint32_t>((v >> 32) & 0xFFFFFFFF); }
static int32_t hi32(int64_t v) { return static_cast<int32_t>((v >> 32) & 0xFFFFFFFF); }

static uint32_t compute_expected(uint32_t op1_u, uint32_t op2_u, int32_t op1_s, int32_t op2_s, uint8_t funct3) {
    switch (funct3) {
    case 0b000: { // MUL (low)
        return static_cast<uint32_t>(static_cast<uint64_t>(op1_u) * op2_u);
    }
    case 0b001: { // MULH (signed * signed) high
        int64_t prod = static_cast<int64_t>(op1_s) * static_cast<int64_t>(op2_s);
        return hi32(prod);
    }
    case 0b010: { // MULHSU (signed * unsigned)
        int64_t prod = static_cast<int64_t>(op1_s) * static_cast<int64_t>(op2_u);
        return hi32(prod);
    }
    case 0b011: { // MULHU (unsigned * unsigned)
        uint64_t prod = static_cast<uint64_t>(op1_u) * static_cast<uint64_t>(op2_u);
        return hi32(prod);
    }
    case 0b100: { // DIV (signed)
        if (op2_s == 0) return 0xFFFFFFFF;
        if (op1_s == INT32_MIN && op2_s == -1) return static_cast<uint32_t>(INT32_MIN);
        return static_cast<uint32_t>(op1_s / op2_s);
    }
    case 0b101: { // DIVU
        if (op2_u == 0) return 0xFFFFFFFF;
        return op1_u / op2_u;
    }
    case 0b110: { // REM
        if (op2_s == 0) return static_cast<uint32_t>(op1_s);
        if (op1_s == INT32_MIN && op2_s == -1) return 0;
        return static_cast<uint32_t>(op1_s % op2_s);
    }
    case 0b111: {
        if (op2_u == 0) return op1_u;
        return op1_u % op2_u;
    }
    }
    return 0;
}

struct Case { uint8_t funct3; const char* name; };
static const Case cases[] = {
    {0b000, "mul"}, {0b001, "mulh"}, {0b010, "mulhsu"}, {0b011, "mulhu"},
    {0b100, "div"}, {0b101, "divu"}, {0b110, "rem"}, {0b111, "remu"},
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
            uint32_t word = encode_m(c.funct3, /*rs2=*/2, /*rs1=*/1, /*rd=*/5);
            MulDivInstruction inst(word);
            inst.execute(em.get());
            uint32_t expected = compute_expected(op1, op2, static_cast<int32_t>(op1), static_cast<int32_t>(op2), c.funct3);
            assert(em->cpuregs[5] == expected);
        }
        std::cout << "M-Ext case " << c.name << " passed." << std::endl;
    }
    std::cout << "All M-Ext tests passed!" << std::endl;
    return 0;
} 