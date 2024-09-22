`include "pkg/memory_pkg.svh"
`include "pkg/pipeline_reg_pkg.svh"
`include "pkg/program_pkg.svh"

module simd_processor
    import memory_pkg::*;
    import pipeline_reg_pkg::*;
    import program_pkg::*;
#(
    parameter BLOCK_DIM = 8
)(
    input logic clk,
    input logic vdma_ready,
    input logic [31:0] from_cpu,

    output logic disp_valid_out,
    output logic [24*BLOCK_DIM-1:0] rgb_out,
    output logic reset_frame,
    output logic [1:0] to_cpu
);

logic scheduler_busy;
logic [9:0] instr_write_addr;
logic [31:0] instr_write_data;
logic instr_write_en;
logic program_ready;
program_info new_program;

cpu_interface CPUInterface (
    .clk(clk),
    .scheduler_busy(scheduler_busy),
    .from_cpu(from_cpu),

    .instr_write_addr(instr_write_addr),
    .instr_write_data(instr_write_data),
    .instr_write_en(instr_write_en),
    .program_ready(program_ready),
    .new_program(new_program),
    .reset_frame(reset_frame),
    .to_cpu(to_cpu)
);

logic stall;
logic [9:0] pc;
logic valid_instr;

scheduler Scheduler (
    .clk(clk),
    .rst(program_ready),
    .stall(stall),
    .exit(decodeLanesRegOut.exit),
    .exit_thread_num(decodeLanesRegOut.thread_num),
    .new_program(new_program),

    .thread_num(fetchDecodeRegIn.thread_num),
    .block_idx(fetchDecodeRegIn.block_idx),
    .pc(pc),
    .busy(scheduler_busy),
    .valid(valid_instr)
);

logic [31:0] fetched_instr;

// Fetch -> Decode pipeline register
fetch_decode_reg fetchDecodeRegIn;
fetch_decode_reg fetchDecodeRegOut;

instr_mem InstrMem (
    .clk(clk),
    .read_addr(pc),
    .write_addr(instr_write_addr),
    .write_instr(instr_write_data),
    .write_en(instr_write_en),

    .read_instr(fetched_instr)
);

// Decode -> SIMD FU pipeline register
decode_lanes_reg decodeLanesRegIn;
decode_lanes_reg decodeLanesRegOut;

decode Decode (
    .instr(fetchDecodeRegOut.instr),

    .func5(decodeLanesRegIn.func5),
    .imm(decodeLanesRegIn.imm),
    .rs1(decodeLanesRegIn.rs1),
    .rs2(decodeLanesRegIn.rs2),
    .rd(decodeLanesRegIn.rd),
    .is_int(decodeLanesRegIn.is_int),
    .is_imm(decodeLanesRegIn.is_imm),
    .is_fadd(decodeLanesRegIn.is_fadd),
    .is_cordic(decodeLanesRegIn.is_cordic),
    .is_mem(decodeLanesRegIn.is_mem),
    .is_memwrite(decodeLanesRegIn.is_memwrite),
    .is_memforce(decodeLanesRegIn.is_memforce),
    .is_cvtfc(decodeLanesRegIn.is_cvtfc),
    .is_disp(decodeLanesRegIn.is_disp),
    .is_predicate_setter(decodeLanesRegIn.is_predicate_setter),
    .is_predicate_getter(decodeLanesRegIn.is_predicate_getter),
    .exit(decodeLanesRegIn.exit)
);

// One read/write request from every lane
write_req_pkt write_reqs [BLOCK_DIM];
logic [8:0] read_data [BLOCK_DIM];
logic [BLOCK_DIM-1:0] disp_valid;

generate
    genvar i;
    for (i = 0; i < BLOCK_DIM; i++) begin: Simd_Lanes
        // i is the threadIdx
        simd_lane #(BLOCK_DIM, i) Simd_Lane (
            .clk(clk),
            .stall(stall),
            .thread_num(decodeLanesRegOut.thread_num),
            .block_idx(decodeLanesRegOut.block_idx),
            .func5(decodeLanesRegOut.func5),
            .imm(decodeLanesRegOut.imm),
            .rs1(decodeLanesRegOut.rs1),
            .rs2(decodeLanesRegOut.rs2),
            .rd(decodeLanesRegOut.rd),
            .is_int(decodeLanesRegOut.is_int),
            .is_imm(decodeLanesRegOut.is_imm),
            .is_cordic(decodeLanesRegOut.is_cordic),
            .is_fadd(decodeLanesRegOut.is_fadd),
            .is_mem(decodeLanesRegOut.is_mem),
            .is_memwrite(decodeLanesRegOut.is_memwrite),
            .is_memforce(decodeLanesRegOut.is_memforce),
            .is_cvtfc(decodeLanesRegOut.is_cvtfc),
            .is_disp(decodeLanesRegOut.is_disp),
            .is_predicate_setter(decodeLanesRegOut.is_predicate_setter),
            .is_predicate_getter(decodeLanesRegOut.is_predicate_getter),
            .mmu_write_en(write_reqs[i].en),
            .mmu_write_force(write_reqs[i].forcewrite),
            .mmu_write_data(write_reqs[i].data),
            .mmu_addr(write_reqs[i].addr),

            .mmu_read_data(read_data[i]),
            .disp_valid(disp_valid[i]),
            .rgb_val(rgb_out[24*i+23:24*i])
        );
    end
endgenerate

logic mmu_stall;

mmu MMU (
    .clk(clk),
    .write_reqs(write_reqs),

    .output_read_data(read_data),
    .stall(mmu_stall)
);

always_comb begin
    stall = mmu_stall | ~vdma_ready;
    disp_valid_out = |disp_valid & ~stall;

    fetchDecodeRegIn.instr = valid_instr ? fetched_instr : 32'h0;
    decodeLanesRegIn.thread_num = fetchDecodeRegOut.thread_num;
    decodeLanesRegIn.block_idx = fetchDecodeRegOut.block_idx;
end

always_ff @(posedge clk) begin
    fetchDecodeRegOut <= ~stall ? fetchDecodeRegIn : fetchDecodeRegOut;
    decodeLanesRegOut <= ~stall ? decodeLanesRegIn : decodeLanesRegOut;
end

endmodule
