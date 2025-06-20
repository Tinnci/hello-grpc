package main

import (
	"context"
	"log"
	"net"
	"sync"
	"time"

	"google.golang.org/grpc"

	pb "github.com/yourorg/go-vemu/api/v1"
	"github.com/yourorg/go-vemu/internal/core"
)

type execCmd struct {
	cycles uint64
	reply  chan<- execResult
	pause  bool
}

type execResult struct {
	executed uint64
	err      error
}

type vemuServer struct {
	pb.UnimplementedVemuServiceServer
	mu      sync.Mutex
	core    core.Core
	execCh  chan execCmd
	traceCh chan *pb.TraceEvent
	stopCh  chan struct{}
}

func newServer() *vemuServer {
	s := &vemuServer{
		core:    core.New(),
		execCh:  make(chan execCmd, 1),
		traceCh: make(chan *pb.TraceEvent, 100),
		stopCh:  make(chan struct{}),
	}
	go s.execLoop()
	return s
}

func (s *vemuServer) execLoop() {
	ticker := time.NewTicker(10 * time.Millisecond) // Prevent busy-loop
	defer ticker.Stop()

	for {
		select {
		case cmd := <-s.execCh:
			if cmd.pause {
				if cmd.reply != nil {
					cmd.reply <- execResult{}
				}
				continue
			}

			s.mu.Lock()
			executed, err := s.core.Step(cmd.cycles)
			s.mu.Unlock()

			if len(s.traceCh) < cap(s.traceCh) {
				// Simplified trace event
				state := s.core.GetState()
				s.traceCh <- &pb.TraceEvent{
					Cycle: state.Cycle,
					Pc:    state.Pc,
				}
			}

			if cmd.reply != nil {
				cmd.reply <- execResult{executed: executed, err: err}
			}
		case <-s.stopCh:
			return
		case <-ticker.C:
			// Allows loop to check stopCh periodically
		}
	}
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
	replyCh := make(chan execResult, 1)
	s.execCh <- execCmd{cycles: r.Cycles, reply: replyCh}

	res := <-replyCh
	return &pb.StepResponse{CyclesExecuted: res.executed}, res.err
}

// ────────────── QueryState ──────────────
func (s *vemuServer) QueryState(ctx context.Context, _ *pb.Empty) (*pb.CpuState, error) {
	s.mu.Lock()
	defer s.mu.Unlock()
	return s.core.GetState(), nil
}

// ────────────── Memory ──────────────
func (s *vemuServer) ReadMemory(ctx context.Context, r *pb.ReadMemRequest) (*pb.ReadMemResponse, error) {
	s.mu.Lock()
	defer s.mu.Unlock()
	data, err := s.core.ReadMem(r.Addr, r.Length)
	if err != nil {
		return nil, err
	}
	return &pb.ReadMemResponse{Data: data}, nil
}

func (s *vemuServer) WriteMemory(ctx context.Context, r *pb.WriteMemRequest) (*pb.Status, error) {
	s.mu.Lock()
	defer s.mu.Unlock()
	if err := s.core.WriteMem(r.Addr, r.Data); err != nil {
		return &pb.Status{Ok: false, Message: err.Error()}, nil
	}
	return &pb.Status{Ok: true}, nil
}

// ────────────── Register access ──────────────
func (s *vemuServer) GetRegs(ctx context.Context, _ *pb.Empty) (*pb.RegisterFile, error) {
	s.mu.Lock()
	defer s.mu.Unlock()
	regs, err := s.core.GetRegs()
	if err != nil {
		return nil, err
	}
	return &pb.RegisterFile{Regs: regs, Pc: s.core.GetState().Pc}, nil
}

func (s *vemuServer) SetReg(ctx context.Context, req *pb.SetRegRequest) (*pb.Status, error) {
	s.mu.Lock()
	defer s.mu.Unlock()
	if err := s.core.SetReg(req.Index, req.Value); err != nil {
		return &pb.Status{Ok: false, Message: err.Error()}, nil
	}
	return &pb.Status{Ok: true}, nil
}

// ────────────── Vector / CSR ops ──────────────
func (s *vemuServer) ReadVector(ctx context.Context, r *pb.ReadVectorRequest) (*pb.ReadVectorResponse, error) {
	s.mu.Lock()
	defer s.mu.Unlock()
	vals, err := s.core.ReadVector(r.Row, r.Elems)
	if err != nil {
		return nil, err
	}
	return &pb.ReadVectorResponse{Values: vals}, nil
}

func (s *vemuServer) WriteVector(ctx context.Context, r *pb.WriteVectorRequest) (*pb.Status, error) {
	s.mu.Lock()
	defer s.mu.Unlock()
	err := s.core.WriteVector(r.Row, r.Values)
	if err != nil {
		return &pb.Status{Ok: false, Message: err.Error()}, nil
	}
	return &pb.Status{Ok: true}, nil
}

func (s *vemuServer) GetCSR(ctx context.Context, r *pb.GetCsrRequest) (*pb.GetCsrResponse, error) {
	s.mu.Lock()
	defer s.mu.Unlock()
	val, err := s.core.GetCSR(r.Id)
	if err != nil {
		return nil, err
	}
	return &pb.GetCsrResponse{Value: val}, nil
}

func (s *vemuServer) SetCSR(ctx context.Context, r *pb.SetCsrRequest) (*pb.Status, error) {
	s.mu.Lock()
	defer s.mu.Unlock()
	err := s.core.SetCSR(r.Id, r.Value)
	if err != nil {
		return &pb.Status{Ok: false, Message: err.Error()}, nil
	}
	return &pb.Status{Ok: true}, nil
}

// ────────────── Run / Pause ──────────────
func (s *vemuServer) Run(ctx context.Context, r *pb.RunRequest) (*pb.RunResponse, error) {
	replyCh := make(chan execResult, 1)
	s.execCh <- execCmd{cycles: r.MaxCycles, reply: replyCh}

	res := <-replyCh
	return &pb.RunResponse{CyclesExecuted: res.executed}, res.err
}

func (s *vemuServer) Pause(ctx context.Context, _ *pb.Empty) (*pb.Status, error) {
	s.execCh <- execCmd{pause: true}
	return &pb.Status{Ok: true}, nil
}

// ────────────── TraceStream ──────────────
func (s *vemuServer) TraceStream(req *pb.Empty, stream pb.VemuService_TraceStreamServer) error {
	for {
		select {
		case event := <-s.traceCh:
			if err := stream.Send(event); err != nil {
				return err
			}
		case <-stream.Context().Done():
			return nil
		case <-s.stopCh:
			return nil
		}
	}
}

// TODO: 其余 RPC (Run, Pause, TraceStream, etc.)

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
