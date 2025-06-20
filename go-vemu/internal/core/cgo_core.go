//go:build cgo
// +build cgo

package core

/*
#cgo CFLAGS: -I${SRCDIR}/../../../vemu_service/cgo_bridge
#cgo LDFLAGS: -L${SRCDIR}/../../../vemu_service/cgo_bridge -lvemu -lstdc++
#include "core_bridge.h"

// This is a forward declaration of a Go function
void go_trace_callback(void* user_data, uint64_t cycle, uint32_t pc);
*/
import "C"

import (
	"fmt"
	"sync"
	"unsafe"

	"runtime/cgo"

	v1 "github.com/yourorg/go-vemu/api/v1"
)

// cgoCore 实际包装 C++ Venus/RISCV 模型
// 注意：实现仅在启用 cgo 时编译

//export go_trace_callback
func go_trace_callback(user_data unsafe.Pointer, cycle C.uint64_t, pc C.uint32_t) {
	defer func() { recover() }()

	if user_data == nil {
		return
	}

	// Reconstruct Go handle from pointer
	h := cgo.Handle(uintptr(user_data))
	v := h.Value()
	ch, ok := v.(chan<- *v1.TraceEvent)
	if !ok {
		return
	}

	event := &v1.TraceEvent{
		Cycle: uint64(cycle),
		Pc:    uint32(pc),
	}
	// Non-blocking send
	select {
	case ch <- event:
	default:
		// Drop if channel is full.
	}
}

type cgoCore struct {
	inst        *C.void
	traceCh     chan<- *v1.TraceEvent
	traceHandle cgo.Handle
	mu          sync.Mutex
}

func newCGO() (Core, error) {
	p := C.vemu_new()
	if p == nil {
		return nil, fmt.Errorf("vemu_new returned nil")
	}
	return &cgoCore{inst: (*C.void)(p)}, nil
}

func (c *cgoCore) Reset() {
	c.mu.Lock()
	defer c.mu.Unlock()
	C.vemu_reset(unsafe.Pointer(c.inst))
}
func (c *cgoCore) Shutdown() {
	c.mu.Lock()
	defer c.mu.Unlock()
	C.vemu_shutdown(unsafe.Pointer(c.inst))
	C.vemu_delete(unsafe.Pointer(c.inst))
}

func (c *cgoCore) LoadHex(txt []byte) error {
	c.mu.Lock()
	defer c.mu.Unlock()
	if len(txt) == 0 {
		return nil
	}
	r := C.vemu_load_hex(unsafe.Pointer(c.inst), (*C.char)(unsafe.Pointer(&txt[0])), C.int(len(txt)))
	if r != 0 {
		return fmt.Errorf("vemu_load_hex failed with %d", int(r))
	}
	return nil
}

func (c *cgoCore) Run(cycles uint64, breakpoints map[uint32]struct{}) (uint64, int, error) {
	c.mu.Lock()
	defer c.mu.Unlock()

	var bps []C.uint32_t
	for pc := range breakpoints {
		bps = append(bps, C.uint32_t(pc))
	}

	var executed C.uint64_t
	var cb C.trace_cb
	var userData unsafe.Pointer

	if c.traceCh != nil {
		// Create a handle pointing to traceCh for C callback.
		c.traceHandle = cgo.NewHandle(c.traceCh)
		cb = (C.trace_cb)(C.go_trace_callback)
		userData = unsafe.Pointer(uintptr(c.traceHandle))
	}

	var bpsPtr *C.uint32_t
	if len(bps) > 0 {
		bpsPtr = &bps[0]
	}

	r := C.vemu_run(unsafe.Pointer(c.inst), C.uint64_t(cycles), bpsPtr, C.int(len(bps)), &executed, cb, userData)

	if int(r) < 0 {
		return 0, 0, fmt.Errorf("run error %d", int(r))
	}
	return uint64(executed), int(r), nil
}

func (c *cgoCore) ReadMem(addr uint32, length uint32) ([]byte, error) {
	c.mu.Lock()
	defer c.mu.Unlock()
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
	c.mu.Lock()
	defer c.mu.Unlock()
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
	c.mu.Lock()
	defer c.mu.Unlock()
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
	// This is lock-free because GetState acquires the lock.
	st := c.GetState()
	return st.Regs, nil
}

func (c *cgoCore) SetReg(index uint32, value uint32) error {
	c.mu.Lock()
	defer c.mu.Unlock()
	r := C.vemu_set_reg(unsafe.Pointer(c.inst), C.uint32_t(index), C.uint32_t(value))
	if r != 0 {
		return fmt.Errorf("SetReg failed with code %d", int(r))
	}
	return nil
}

func (c *cgoCore) Pause(paused bool) {
	// This is a fast, non-blocking call to the C side,
	// but we take the lock to ensure consistency with other operations.
	c.mu.Lock()
	defer c.mu.Unlock()
	p := 0
	if paused {
		p = 1
	}
	C.vemu_pause(unsafe.Pointer(c.inst), C.int(p))
}

func (c *cgoCore) SetTraceChan(ch chan<- *v1.TraceEvent) {
	c.mu.Lock()
	defer c.mu.Unlock()
	// Release previous handle if exists
	if c.traceHandle != 0 {
		c.traceHandle.Delete()
		c.traceHandle = 0
	}
	c.traceCh = ch
}

func (c *cgoCore) ReadVector(row, elems uint32) ([]uint32, error) {
	c.mu.Lock()
	defer c.mu.Unlock()
	if elems == 0 {
		return nil, nil
	}
	buf := make([]uint32, elems)
	r := C.vemu_read_vector(unsafe.Pointer(c.inst), C.uint32_t(row), C.uint32_t(elems), (*C.uint32_t)(unsafe.Pointer(&buf[0])))
	if r != 0 {
		return nil, fmt.Errorf("read_vector error %d", int(r))
	}
	return buf, nil
}

func (c *cgoCore) WriteVector(row uint32, values []uint32) error {
	c.mu.Lock()
	defer c.mu.Unlock()
	if len(values) == 0 {
		return nil
	}
	r := C.vemu_write_vector(unsafe.Pointer(c.inst), C.uint32_t(row), C.uint32_t(len(values)), (*C.uint32_t)(unsafe.Pointer(&values[0])))
	if r != 0 {
		return fmt.Errorf("write_vector error %d", int(r))
	}
	return nil
}

func (c *cgoCore) GetCSR(id uint32) (uint32, error) {
	c.mu.Lock()
	defer c.mu.Unlock()
	var val C.uint32_t
	r := C.vemu_get_csr(unsafe.Pointer(c.inst), C.uint32_t(id), &val)
	if r != 0 {
		return 0, fmt.Errorf("get_csr error %d", int(r))
	}
	return uint32(val), nil
}

func (c *cgoCore) SetCSR(id, value uint32) error {
	c.mu.Lock()
	defer c.mu.Unlock()
	r := C.vemu_set_csr(unsafe.Pointer(c.inst), C.uint32_t(id), C.uint32_t(value))
	if r != 0 {
		return fmt.Errorf("set_csr error %d", int(r))
	}
	return nil
}
