//go:build cgo
// +build cgo

package core

import (
	"bytes"
	"strings"
	"testing"
)

func TestCGO_Core_HexLoadAndStep(t *testing.T) {
	c, err := newCGO()
	if err != nil {
		t.Fatalf("newCGO() failed: %v", err)
	}
	defer c.Shutdown()

	// A simple hex program:
	//   addi x1, x0, 5
	//   addi x2, x0, 10
	//   add  x3, x1, x2
	//   ebreak
	hexProg := []byte("00500093\n00a00113\n002081b3\n00100073\n")

	if err := c.LoadHex(hexProg); err != nil {
		t.Fatalf("LoadHex() failed: %v", err)
	}

	executed, reason, err := c.Run(10, nil) // Should stop at ebreak after 4 cycles
	if err != nil {
		t.Fatalf("Run() failed: %v", err)
	}

	if executed != 4 {
		t.Errorf("Run() executed %d cycles, want 4", executed)
	}

	if reason != StopReasonEbreak {
		t.Errorf("Run() stop reason = %d, want %d (ebreak)", reason, StopReasonEbreak)
	}

	regs, err := c.GetRegs()
	if err != nil {
		t.Fatalf("GetRegs() failed: %v", err)
	}

	if len(regs) < 4 {
		t.Fatalf("GetRegs() returned %d registers, want at least 4", len(regs))
	}

	if regs[1] != 5 {
		t.Errorf("x1 = %d, want 5", regs[1])
	}
	if regs[2] != 10 {
		t.Errorf("x2 = %d, want 10", regs[2])
	}
	if regs[3] != 15 {
		t.Errorf("x3 = %d, want 15", regs[3])
	}
}

func TestCGO_Core_Memory(t *testing.T) {
	c, _ := newCGO()
	defer c.Shutdown()

	writeData := []byte{0xde, 0xad, 0xbe, 0xef}
	addr := uint32(0x100)

	if err := c.WriteMem(addr, writeData); err != nil {
		t.Fatalf("WriteMem() failed: %v", err)
	}

	readData, err := c.ReadMem(addr, uint32(len(writeData)))
	if err != nil {
		t.Fatalf("ReadMem() failed: %v", err)
	}

	if !bytes.Equal(writeData, readData) {
		t.Errorf("ReadMem() got %x, want %x", readData, writeData)
	}
}

func TestCGO_Core_Vector(t *testing.T) {
	c, _ := newCGO()
	defer c.Shutdown()

	writeData := []uint32{1, 2, 3, 4}
	row := uint32(10)

	if err := c.WriteVector(row, writeData); err != nil {
		t.Fatalf("WriteVector() failed: %v", err)
	}

	readData, err := c.ReadVector(row, uint32(len(writeData)))
	if err != nil {
		t.Fatalf("ReadVector() failed: %v", err)
	}

	if len(readData) != len(writeData) {
		t.Fatalf("ReadVector() read %d elements, want %d", len(readData), len(writeData))
	}
	for i := range writeData {
		if writeData[i] != readData[i] {
			t.Errorf("ReadVector()[%d] got %d, want %d", i, readData[i], writeData[i])
		}
	}
}

func TestCGO_ReadMem_OutOfBoundsError(t *testing.T) {
	c, err := newCGO()
	if err != nil {
		t.Fatalf("newCGO() failed: %v", err)
	}
	defer c.Shutdown()

	// An address far beyond the 4MB SRAM size
	addr := uint32(0x100000 * 4)
	_, err = c.ReadMem(addr, 4)

	if err == nil {
		t.Fatal("ReadMem at out-of-bounds address should have returned an error, but got nil")
	}

	expectedErrStr := "out of bounds"
	if !strings.Contains(err.Error(), expectedErrStr) {
		t.Errorf("Error message did not contain expected substring.\nGot:    %s\nWanted: %s", err.Error(), expectedErrStr)
	}
	t.Logf("Received expected error: %v", err)
}

func TestCGO_SetReg_InvalidIndex(t *testing.T) {
	c, _ := newCGO()
	defer c.Shutdown()

	err := c.SetReg(32, 123) // Index 32 is invalid
	if err == nil {
		t.Fatal("SetReg with invalid index should have returned an error, but got nil")
	}

	expectedErrStr := "invalid register index"
	if !strings.Contains(err.Error(), expectedErrStr) {
		t.Errorf("Error message did not contain expected substring.\nGot:    %s\nWanted: %s", err.Error(), expectedErrStr)
	}
	t.Logf("Received expected error: %v", err)
}
