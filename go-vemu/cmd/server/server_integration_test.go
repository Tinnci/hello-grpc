//go:build cgo
// +build cgo

package main

import (
	"bytes"
	"context"
	"log"
	"net"
	"testing"
	"time"

	"google.golang.org/grpc"
	"google.golang.org/grpc/test/bufconn"

	pb "github.com/yourorg/go-vemu/api/v1"
)

const bufSize = 1024 * 1024

// set up an in-memory gRPC server and return a connected client and a teardown func.
func setupGRPCServer(t *testing.T) (pb.VemuServiceClient, func()) {
	t.Helper()

	lis := bufconn.Listen(bufSize)

	server := grpc.NewServer()
	pb.RegisterVemuServiceServer(server, newServer())

	go func() {
		if err := server.Serve(lis); err != nil {
			log.Fatalf("gRPC server exited: %v", err)
		}
	}()

	// custom dialer that connects via the listener
	ctx := context.Background()
	dialer := func(context.Context, string) (net.Conn, error) { return lis.Dial() }

	conn, err := grpc.DialContext(ctx, "bufnet", grpc.WithContextDialer(dialer), grpc.WithInsecure())
	if err != nil {
		t.Fatalf("failed to dial bufnet: %v", err)
	}

	client := pb.NewVemuServiceClient(conn)

	teardown := func() {
		conn.Close()
		server.Stop()
	}

	return client, teardown
}

func TestIntegration_LoadAndRun(t *testing.T) {
	client, teardown := setupGRPCServer(t)
	defer teardown()

	ctx, cancel := context.WithTimeout(context.Background(), 5*time.Second)
	defer cancel()

	// Simple firmware: addi x1, x0, 5; addi x2, x0, 10; add x3, x1, x2; ebreak
	hexProg := []byte("00500093\n00a00113\n002081b3\n00100073\n")

	// Load firmware
	if _, err := client.LoadFirmware(ctx, &pb.LoadFirmwareRequest{HexText: hexProg}); err != nil {
		t.Fatalf("LoadFirmware failed: %v", err)
	}

	// Run up to 10 cycles (should stop at ebreak in 4 cycles)
	runResp, err := client.Run(ctx, &pb.RunRequest{MaxCycles: 10})
	if err != nil {
		t.Fatalf("Run RPC failed: %v", err)
	}

	if runResp.CyclesExecuted != 4 {
		t.Errorf("CyclesExecuted = %d; want 4", runResp.CyclesExecuted)
	}

	if runResp.Reason != 1 { // StopReasonEbreak = 1
		t.Errorf("Run stop reason = %d; want 1 (Ebreak)", runResp.Reason)
	}

	// Verify register results
	regsResp, err := client.GetRegs(ctx, &pb.Empty{})
	if err != nil {
		t.Fatalf("GetRegs RPC failed: %v", err)
	}

	regs := regsResp.GetRegs()
	if len(regs) < 4 {
		t.Fatalf("GetRegs returned %d registers; want >=4", len(regs))
	}
	if regs[1] != 5 {
		t.Errorf("x1 = %d; want 5", regs[1])
	}
	if regs[2] != 10 {
		t.Errorf("x2 = %d; want 10", regs[2])
	}
	if regs[3] != 15 {
		t.Errorf("x3 = %d; want 15", regs[3])
	}
}

func TestIntegration_Breakpoints(t *testing.T) {
	client, teardown := setupGRPCServer(t)
	defer teardown()

	ctx, cancel := context.WithTimeout(context.Background(), 5*time.Second)
	defer cancel()

	// Load same demo program
	hexProg := []byte("00500093\n00a00113\n002081b3\n00100073\n")
	if _, err := client.LoadFirmware(ctx, &pb.LoadFirmwareRequest{HexText: hexProg}); err != nil {
		t.Fatalf("LoadFirmware failed: %v", err)
	}

	// Breakpoint at PC = 8 (third instruction)
	if _, err := client.SetBreakpoint(ctx, &pb.BreakpointRequest{Pc: 8}); err != nil {
		t.Fatalf("SetBreakpoint failed: %v", err)
	}

	// Run indefinitely (0 cycles means until stop)
	runResp, err := client.Run(ctx, &pb.RunRequest{})
	if err != nil {
		t.Fatalf("Run RPC failed: %v", err)
	}
	if runResp.Reason != 2 { // StopReasonBp = 2
		t.Fatalf("expected stop reason 2 (Breakpoint), got %d", runResp.Reason)
	}
	// Should have executed three instructions (pc 0,4,8)
	if runResp.CyclesExecuted != 3 {
		t.Errorf("CyclesExecuted = %d; want 3", runResp.CyclesExecuted)
	}

	// Clear breakpoint and continue
	if _, err := client.ClearBreakpoint(ctx, &pb.BreakpointRequest{Pc: 8}); err != nil {
		t.Fatalf("ClearBreakpoint failed: %v", err)
	}

	runResp2, err := client.Run(ctx, &pb.RunRequest{})
	if err != nil {
		t.Fatalf("Run after clear failed: %v", err)
	}
	if runResp2.Reason != 1 { // Ebreak
		t.Fatalf("expected stop reason 1 (Ebreak) after clearing breakpoint, got %d", runResp2.Reason)
	}
}

func TestIntegration_MemoryIO(t *testing.T) {
	client, teardown := setupGRPCServer(t)
	defer teardown()

	ctx, cancel := context.WithTimeout(context.Background(), 5*time.Second)
	defer cancel()

	addr := uint32(0x200)
	payload := []byte{0xde, 0xad, 0xbe, 0xef}

	if _, err := client.WriteMemory(ctx, &pb.WriteMemRequest{Addr: addr, Data: payload}); err != nil {
		t.Fatalf("WriteMemory failed: %v", err)
	}

	readResp, err := client.ReadMemory(ctx, &pb.ReadMemRequest{Addr: addr, Length: uint32(len(payload))})
	if err != nil {
		t.Fatalf("ReadMemory failed: %v", err)
	}
	if got := readResp.GetData(); !bytes.Equal(got, payload) {
		t.Errorf("memory mismatch: got %x, want %x", got, payload)
	}
}

func TestIntegration_VectorOps(t *testing.T) {
	client, teardown := setupGRPCServer(t)
	defer teardown()

	ctx, cancel := context.WithTimeout(context.Background(), 5*time.Second)
	defer cancel()

	row := uint32(5)
	values := []uint32{1, 2, 3, 4}

	if _, err := client.WriteVector(ctx, &pb.WriteVectorRequest{Row: row, Values: values}); err != nil {
		t.Fatalf("WriteVector failed: %v", err)
	}

	readResp, err := client.ReadVector(ctx, &pb.ReadVectorRequest{Row: row, Elems: uint32(len(values))})
	if err != nil {
		t.Fatalf("ReadVector failed: %v", err)
	}

	got := readResp.GetValues()
	if len(got) != len(values) {
		t.Fatalf("ReadVector returned %d elements, want %d", len(got), len(values))
	}
	for i := range values {
		if got[i] != values[i] {
			t.Errorf("vector[%d] = %d, want %d", i, got[i], values[i])
		}
	}
}
