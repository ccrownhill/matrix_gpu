/*
 * Register addresses shared between threads:
 * r0: read-only 0 (zero)
 * r1: per-thread scheduler-assigned block index (%blockIdx)
 * r2: read-only block dimension (%blockDim)
 * r3: read-only thread index (%threadIdx)
 *
 * General-purpose per-thread registers:
 * r4-r11
 */

module reg_file #(
    parameter BLOCK_DIM = 1,
              THREAD_IDX = 0
)(
    input logic clk,
    input logic [3:0] read_thread,
    input logic [17:0] block_idx,
    input logic [3:0] rs1,
    input logic [3:0] rs2,
    input logic [3:0] write_thread,
    input logic [3:0] write_rd,
    input logic [17:0] write_data,
    input logic write_en,
    input logic predicate_write_en,
    input logic predicate_in,

    output logic [17:0] reg1_out,
    output logic [17:0] reg2_out,
    output logic        predicate_out
);

logic [17:0] shared_registers [4];
logic [17:0] thread_registers [16][8];
logic        thread_predicates [16];

always_comb begin
    shared_registers[0] = 18'b0;
    shared_registers[1] = block_idx;
    shared_registers[2] = BLOCK_DIM;
    shared_registers[3] = THREAD_IDX;

    reg1_out = (rs1 > 4'd3) ? thread_registers[read_thread][{rs1 - 4'd4}[2:0]]
                            : shared_registers[rs1[1:0]];
    reg2_out = (rs2 > 4'd3) ? thread_registers[read_thread][{rs2 - 4'd4}[2:0]]
                            : shared_registers[rs2[1:0]];
    predicate_out = thread_predicates[read_thread];
end

always_ff @(posedge clk) begin
    if (write_en && write_rd > 4'd3) begin
        thread_registers[write_thread][{write_rd - 4'd4}[2:0]]
            <= write_data;
    end
    if (predicate_write_en) begin
        thread_predicates[write_thread] <= predicate_in;
    end
end

endmodule
