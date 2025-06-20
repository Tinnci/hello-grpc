#include <grpcpp/grpcpp.h>
#include "vemu_service.pb.h"
#include "vemu_service.grpc.pb.h"
#include "RISCV.h"
#include "venus_ext.h"
#include <iostream>
#include <memory>
#include <mutex>
#include <string>
#include <vector>

using grpc::Server;
using grpc::ServerBuilder;
using grpc::ServerContext;
using grpc::Status;

// --- Proto 命名空间别名 ---
namespace proto = vemu::v1;

// ------------------ Emulator Backend ------------------
class EmulatorBackend {
public:
    EmulatorBackend() {
        emulator_.init_param();
        configured_ = false;
    }

    bool configured() const { return configured_; }

    void configure(uint64_t /*mem_size*/) {
        std::lock_guard<std::mutex> lock(mu_);
        configured_ = true; // SRAM 大小暂时固定，忽略传入参数
    }

    bool writeMemory(uint64_t addr, const std::string& data, std::string& err) {
        std::lock_guard<std::mutex> lock(mu_);
        if (!configured_) { err = "Emulator not configured"; return false; }
        if (addr + data.size() > CPU::SRAMSIZE * sizeof(uint32_t)) {
            err = "WriteMemory out of range"; return false; }
        for (size_t i = 0; i < data.size(); ++i) {
            size_t byte_addr = addr + i;
            size_t word_idx  = byte_addr / 4;
            size_t byte_off  = byte_addr % 4;
            uint32_t word = emulator_.sram[word_idx];
            word &= ~(0xFFu << (byte_off * 8));
            word |=  (static_cast<uint32_t>(static_cast<uint8_t>(data[i])) << (byte_off * 8));
            emulator_.sram[word_idx] = word;
        }
        return true;
    }

    bool readMemory(uint64_t addr, uint32_t size, std::string& out, std::string& err) {
        std::lock_guard<std::mutex> lock(mu_);
        if (!configured_) { err = "Emulator not configured"; return false; }
        if (addr + size > CPU::SRAMSIZE * sizeof(uint32_t)) {
            err = "ReadMemory out of range"; return false; }
        out.resize(size);
        for (uint32_t i = 0; i < size; ++i) {
            size_t byte_addr = addr + i;
            uint32_t word = emulator_.sram[byte_addr / 4];
            out[i] = static_cast<char>((word >> ((byte_addr % 4) * 8)) & 0xFF);
        }
        return true;
    }

    bool loadFirmware(const std::string& fw, uint64_t load_addr, std::string& err) {
        return writeMemory(load_addr, fw, err);
    }

    enum class ExecExit { HALTED, CYCLE_LIMIT, ERROR };

    ExecExit execute(uint64_t start_pc, uint64_t cycle_limit,
                     uint64_t& cycles_executed, uint64_t& final_pc) {
        std::lock_guard<std::mutex> lock(mu_);
        if (!configured_) return ExecExit::ERROR;
        emulator_.pc = static_cast<uint32_t>(start_pc);
        cycles_executed = 0;
        for (; cycle_limit == 0 || cycles_executed < cycle_limit; ++cycles_executed) {
            emulator_.emulate();
            if (emulator_.ebreak) { final_pc = emulator_.pc; return ExecExit::HALTED; }
            if (emulator_.trap)   { final_pc = emulator_.pc; return ExecExit::ERROR; }
        }
        final_pc = emulator_.pc;
        return ExecExit::CYCLE_LIMIT;
    }

private:
    Venus_Emulator emulator_;
    bool configured_;
    std::mutex mu_;
};

static EmulatorBackend g_backend;

// ------------------ gRPC Service 实现 ------------------
class VemuServiceImpl final : public proto::VemuService::Service {
public:
    Status Configure(ServerContext*, const proto::ConfigRequest* req,
                     proto::StatusResponse* res) override {
        g_backend.configure(req->memory_size());
        res->set_success(true);
        return Status::OK;
    }

    Status WriteMemory(ServerContext*, const proto::MemoryWriteRequest* req,
                       proto::StatusResponse* res) override {
        std::string err;
        bool ok = g_backend.writeMemory(req->address(), req->data(), err);
        res->set_success(ok);
        if (!ok) res->set_error_message(err);
        return Status::OK;
    }

    Status ReadMemory(ServerContext*, const proto::MemoryReadRequest* req,
                      proto::MemoryReadResponse* res) override {
        std::string data, err;
        bool ok = g_backend.readMemory(req->address(), req->size(), data, err);
        if (ok) res->set_data(std::move(data));
        return Status::OK;
    }

    Status LoadFirmware(ServerContext*, const proto::FirmwareRequest* req,
                        proto::StatusResponse* res) override {
        std::string err;
        bool ok = g_backend.loadFirmware(req->firmware_image(), req->load_address(), err);
        res->set_success(ok);
        if (!ok) res->set_error_message(err);
        return Status::OK;
    }

    Status Execute(ServerContext*, const proto::ExecuteRequest* req,
                   proto::ExecuteResponse* res) override {
        uint64_t cycles=0, pc=0;
        auto exit_type = g_backend.execute(req->start_address(), req->cycle_limit(), cycles, pc);
        switch(exit_type){
            case EmulatorBackend::ExecExit::HALTED: res->set_exit_reason(proto::ExecuteResponse::HALTED); break;
            case EmulatorBackend::ExecExit::CYCLE_LIMIT: res->set_exit_reason(proto::ExecuteResponse::CYCLE_LIMIT_REACHED); break;
            case EmulatorBackend::ExecExit::ERROR: default: res->set_exit_reason(proto::ExecuteResponse::ERROR); break;
        }
        res->set_cycles_executed(cycles);
        res->set_final_pc(pc);
        return Status::OK;
    }

    // 其余方法暂未实现
    Status ReadRegisters(ServerContext*, const proto::ReadRegistersRequest*,
                         proto::ReadRegistersResponse*) override {
        return Status(grpc::StatusCode::UNIMPLEMENTED, "ReadRegisters not implemented");
    }
    Status WriteRegisters(ServerContext*, const proto::WriteRegistersRequest*,
                          proto::StatusResponse*) override {
        return Status(grpc::StatusCode::UNIMPLEMENTED, "WriteRegisters not implemented");
    }
    Status SetBreakpoint(ServerContext*, const proto::BreakpointRequest*,
                         proto::StatusResponse*) override {
        return Status(grpc::StatusCode::UNIMPLEMENTED, "SetBreakpoint not implemented");
    }
    Status ClearBreakpoint(ServerContext*, const proto::BreakpointRequest*,
                           proto::StatusResponse*) override {
        return Status(grpc::StatusCode::UNIMPLEMENTED, "ClearBreakpoint not implemented");
    }
    Status SubscribeEvents(ServerContext*, const proto::SubscribeEventsRequest*,
                           grpc::ServerWriter< proto::Event >*) override {
        return Status(grpc::StatusCode::UNIMPLEMENTED, "SubscribeEvents not implemented");
    }
};

// ------------------ 服务器启动 ------------------
static void RunServer(const std::string& address) {
    VemuServiceImpl service;
    ServerBuilder builder;
    builder.AddListeningPort(address, grpc::InsecureServerCredentials());
    builder.RegisterService(&service);
    std::unique_ptr<Server> server(builder.BuildAndStart());
    std::cout << "VEMU Service listening on " << address << std::endl;
    server->Wait();
}

int main(int /*argc*/, char** /*argv*/){
    RunServer("0.0.0.0:50051");
    return 0;
}
