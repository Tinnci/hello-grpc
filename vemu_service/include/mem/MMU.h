#ifndef VEMU_MEM_MMU_H
#define VEMU_MEM_MMU_H

#include <cstdint>
class Emulator;

namespace Mem {

class MMU {
public:
    explicit MMU(Emulator* cpu) : cpu_(cpu) {}

    uint32_t read_word(uint32_t addr) const;
    void write_word(uint32_t addr, uint32_t data);
    int8_t read_byte(int32_t addr) const;
    uint8_t read_byte_u(uint32_t addr) const;
    int16_t read_half(int32_t addr) const;
    uint16_t read_half_u(uint32_t addr) const;

    void write_byte(uint32_t addr, uint8_t data);
    void write_half(uint32_t addr, uint16_t data);
private:
    Emulator* cpu_;
};

} // namespace Mem

#endif // VEMU_MEM_MMU_H 