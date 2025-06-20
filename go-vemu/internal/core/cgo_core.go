//go:build cgo
// +build cgo

package core

/*
#cgo LDFLAGS: -L${SRCDIR}/../../../vemu_service/cgo_bridge -lvemu -lstdc++
#include <stdint.h>
// 未来将包含 "core_bridge.h"
// 临时声明桥接函数，后续替换真实头文件
void* vemu_new();
void  vemu_delete(void* inst);
int   vemu_load_hex(void* inst, const char* txt, int len);
int   vemu_step(void* inst, uint64_t n, uint64_t* executed);
int   vemu_read(void* inst, uint32_t addr, uint32_t len, uint8_t* out);
int   vemu_write(void* inst, uint32_t addr, uint32_t len, const uint8_t* in);
void  vemu_reset(void* inst);
void  vemu_get_state(void* inst, uint32_t* regs32, uint32_t* pc, uint64_t* cycle);
void  vemu_shutdown(void* inst);
*/
import "C"

import (
	"fmt"
	"unsafe"

	v1 "github.com/yourorg/go-vemu/api/v1"
)

// cgoCore 实际包装 C++ Venus/RISCV 模型
// 注意：实现仅在启用 cgo 时编译

type cgoCore struct {
	inst *C.void
}

func newCGO() (Core, error) {
	p := C.vemu_new()
	if p == nil {
		return nil, fmt.Errorf("vemu_new returned nil")
	}
	return &cgoCore{inst: (*C.void)(p)}, nil
}

func (c *cgoCore) Reset() { C.vemu_reset(unsafe.Pointer(c.inst)) }
func (c *cgoCore) Shutdown() {
	C.vemu_shutdown(unsafe.Pointer(c.inst))
	C.vemu_delete(unsafe.Pointer(c.inst))
}

func (c *cgoCore) LoadHex(txt []byte) error {
	if len(txt) == 0 {
		return nil
	}
	r := C.vemu_load_hex(unsafe.Pointer(c.inst), (*C.char)(unsafe.Pointer(&txt[0])), C.int(len(txt)))
	if r != 0 {
		return fmt.Errorf("vemu_load_hex failed with %d", int(r))
	}
	return nil
}

func (c *cgoCore) Step(n uint64) (uint64, error) {
	var executed C.uint64_t
	r := C.vemu_step(unsafe.Pointer(c.inst), C.uint64_t(n), &executed)
	if r != 0 {
		return 0, fmt.Errorf("step error %d", int(r))
	}
	return uint64(executed), nil
}

func (c *cgoCore) ReadMem(addr uint32, length uint32) ([]byte, error) {
	buf := make([]byte, length)
	if length > 0 {
		r := C.vemu_read(unsafe.Pointer(c.inst), C.uint32_t(addr), C.uint32_t(length), (*C.uint8_t)(unsafe.Pointer(&buf[0])))
		if r != 0 {
			return nil, fmt.Errorf("read error %d", int(r))
		}
	}
	return buf, nil
}

func (c *cgoCore) WriteMem(addr uint32, data []byte) error {
	if len(data) == 0 {
		return nil
	}
	r := C.vemu_write(unsafe.Pointer(c.inst), C.uint32_t(addr), C.uint32_t(len(data)), (*C.uint8_t)(unsafe.Pointer(&data[0])))
	if r != 0 {
		return fmt.Errorf("write error %d", int(r))
	}
	return nil
}

func (c *cgoCore) GetState() *v1.CpuState {
	var regs [32]C.uint32_t
	var pc C.uint32_t
	var cycle C.uint64_t
	C.vemu_get_state(unsafe.Pointer(c.inst), &regs[0], &pc, &cycle)
	out := &v1.CpuState{Pc: uint32(pc), Cycle: uint64(cycle)}
	for _, r := range regs[:] {
		out.Regs = append(out.Regs, uint32(r))
	}
	return out
}

func (c *cgoCore) GetRegs() ([]uint32, error) {
	st := c.GetState()
	return st.Regs, nil
}

func (c *cgoCore) SetReg(index uint32, value uint32) error {
	// TODO: add C API to set register; for now return not implemented
	return fmt.Errorf("SetReg not implemented in cgo core")
}
