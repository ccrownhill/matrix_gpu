/*
 *  Pipelined BRAM bank for frame buffer
 *  READS: Single cycle read (LW)
 *  WRITES: 3-stage RMW pipeline (SPIX, SW)
 *    (optional override by setting write_force high)
 *  - Only write new value if greater than old value
 *  - Stage 1: Read current value from RAM
 *  - Stage 2: Compare new value to current value
 *  - Stage 3: Write new value to RAM if greater
 *  Includes forwarding from stage 3->1 and 3->2 for same addresses
 */

`include "pkg/memory_pkg.svh"

module mem_bank
    import memory_pkg::*;
(
    input logic clk,
    input write_req_pkt write_req,

    output logic [8:0] read_data
);

// Vivado macro to force BRAM use
// 64x512 words per bank
(* ram_style = "block" *) reg [8:0] ram [32767:0];

logic [14:0] req_addr_s1;

logic [8:0] current_val_s2;
logic [8:0] new_val_s2;
logic [14:0] write_addr_s2;
logic write_en_s2;
logic write_force_s2;
logic write_new_s2;

logic [8:0] new_val_s3;
logic [14:0] write_addr_s3;
logic write_new_s3;

always_comb begin
    // Stage 1: Read from RAM (clocked)
    req_addr_s1 = write_req.addr[17:3];

    // Stage 2: Compare
    // Forward from S3 if address is same and new value is greater
    current_val_s2 = (write_new_s3
                   && write_addr_s2 == write_addr_s3) ? new_val_s3
                                                      : read_data;
    // Only write if write_en input was high on previous cycle
    // and input val is greater than current val in RAM
    // Always write when write_force and write_en are both high
    write_new_s2 = write_en_s2 & ((new_val_s2 > current_val_s2)
                                  | write_force_s2);
end

always_ff @(posedge clk) begin
    // Read data from RAM at end of stage 1
    read_data <= ram[req_addr_s1];
    // Stage 1-2 pipeline register
    new_val_s2 <= write_req.data;
    write_addr_s2 <= req_addr_s1;
    write_en_s2 <= write_req.en;
    write_force_s2 <= write_req.forcewrite;

    // Stage 2-3 pipeline register
    new_val_s3 <= new_val_s2;
    write_addr_s3 <= write_addr_s2;
    write_new_s3 <= write_new_s2;

    // Stage 3: Write
    if (write_new_s3) begin
        ram[write_addr_s3] <= new_val_s3;
    end
end

endmodule
