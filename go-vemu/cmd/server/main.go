package main

import (
	"context"
	"log"
	"net"
	"sync"

	"google.golang.org/grpc"

	pb "github.com/yourorg/go-vemu/api/v1"
	"github.com/yourorg/go-vemu/internal/core"
)

type vemuServer struct {
	pb.UnimplementedVemuServiceServer
	mu   sync.Mutex
	core core.Core
}

func newServer() *vemuServer {
	return &vemuServer{core: core.New()}
}

// ────────────── 生命周期 ──────────────
func (s *vemuServer) Reset(ctx context.Context, _ *pb.Empty) (*pb.Status, error) {
	s.mu.Lock()
	defer s.mu.Unlock()
	s.core.Reset()
	return &pb.Status{Ok: true}, nil
}

func (s *vemuServer) Shutdown(ctx context.Context, _ *pb.Empty) (*pb.Status, error) {
	s.mu.Lock()
	defer s.mu.Unlock()
	s.core.Shutdown()
	return &pb.Status{Ok: true}, nil
}

// ────────────── LoadFirmware ──────────────
func (s *vemuServer) LoadFirmware(ctx context.Context, r *pb.LoadFirmwareRequest) (*pb.Status, error) {
	s.mu.Lock()
	defer s.mu.Unlock()
	if err := s.core.LoadHex(r.HexText); err != nil {
		return &pb.Status{Ok: false, Message: err.Error()}, nil
	}
	return &pb.Status{Ok: true}, nil
}

// ────────────── Step ──────────────
func (s *vemuServer) Step(ctx context.Context, r *pb.StepRequest) (*pb.StepResponse, error) {
	s.mu.Lock()
	defer s.mu.Unlock()
	exec, err := s.core.Step(r.Cycles)
	if err != nil {
		return nil, err
	}
	return &pb.StepResponse{CyclesExecuted: exec}, nil
}

// ────────────── QueryState ──────────────
func (s *vemuServer) QueryState(ctx context.Context, _ *pb.Empty) (*pb.CpuState, error) {
	s.mu.Lock()
	defer s.mu.Unlock()
	return s.core.GetState(), nil
}

// TODO: 其余 RPC (ReadMemory, WriteMemory, etc.)

func main() {
	lis, err := net.Listen("tcp", ":50051")
	if err != nil {
		log.Fatalf("failed to listen: %v", err)
	}

	grpcSrv := grpc.NewServer()
	pb.RegisterVemuServiceServer(grpcSrv, newServer())

	log.Println("Vemu gRPC server listening on :50051")
	if err := grpcSrv.Serve(lis); err != nil {
		log.Fatalf("failed to serve: %v", err)
	}
}
