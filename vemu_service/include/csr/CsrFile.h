#ifndef VEMU_CSR_FILE_H
#define VEMU_CSR_FILE_H

#include <cstdint>
#include <unordered_map>

class Emulator; // forward decl

namespace CSR {
// 常用 CSR 地址宏
constexpr uint16_t CSR_CYCLE   = 0xC00;
constexpr uint16_t CSR_CYCLEH  = 0xC80;
constexpr uint16_t CSR_INSTRET = 0xC02;
constexpr uint16_t CSR_INSTRETH= 0xC82;
constexpr uint16_t CSR_MSTATUS = 0x300;
constexpr uint16_t CSR_MIE     = 0x304;
constexpr uint16_t CSR_MTVEC   = 0x305;
constexpr uint16_t CSR_MEPC    = 0x341;
constexpr uint16_t CSR_MCAUSE  = 0x342;
constexpr uint16_t CSR_MIP     = 0x344;
constexpr uint16_t CSR_MTVAL   = 0x343;
constexpr uint32_t MSTATUS_MIE = 1u << 3;   // Machine Interrupt Enable
constexpr uint32_t MIP_MEIP   = 1u << 11;  // Machine External Interrupt Pending
constexpr uint32_t MIE_MEIE   = 1u << 11;  // Machine External Interrupt Enable
constexpr uint32_t MSTATUS_MPIE = 1u << 7;   // Previous MIE
constexpr uint32_t MSTATUS_MPP_MASK = 0b11u << 11; // Bits 12:11
}

class CsrFile {
public:
    uint32_t read(uint16_t addr, const Emulator* cpu) const;
    void write(uint16_t addr, uint32_t value);
private:
    std::unordered_map<uint16_t, uint32_t> storage_;
};

#endif // VEMU_CSR_FILE_H 