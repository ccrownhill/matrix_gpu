`include "pkg/program_pkg.svh"

module scheduler
    import program_pkg::*;
(
    input logic clk,
    input logic rst,
    input logic stall,
    input logic exit,
    input logic [3:0] exit_thread_num,
    input program_info new_program,

    output logic [3:0] thread_num,
    output logic [17:0] block_idx,
    output logic [9:0] pc,
    output logic busy,
    output logic valid
);

typedef struct packed {
    logic [17:0] block_idx;
    logic [9:0] pc;
    logic busy;
} thread;

program_info programs [5];
thread [15:0] threads;
logic [17:0] threads_issued;
logic threads_busy;
logic programs_queued;

always_comb begin
    // Taking bitwise or across all thread busy bits
    threads_busy = 1'b0;
    programs_queued = 1'b0;
    for (int i = 0; i < 16; i++) begin
        threads_busy = threads_busy | threads[i].busy;
    end
    for (int i = 0; i < 5; i++) begin
        programs_queued = programs_queued | programs[i].valid;
    end

    busy = threads_busy | programs_queued;

    block_idx = threads[thread_num].block_idx;
    pc = threads[thread_num].pc;
    valid = threads[thread_num].busy;
end

always_ff @(posedge clk) begin
    if (rst) begin
        for (int i = 0; i < 5; i++) begin
            programs[i].valid <= 1'b0;
        end
        for (int i = 0; i < 16; i++) begin
            threads[i].busy <= 1'b0;
        end

    end else if (~stall) begin
        if (programs[0].valid) begin
            if (threads[thread_num].busy) begin
                threads[thread_num].pc <= threads[thread_num].pc + 10'd1;
            end else if (threads_issued != programs[0].num_blocks) begin
                threads[thread_num].block_idx <= threads_issued;
                threads[thread_num].pc <= programs[0].start_addr;
                threads[thread_num].busy <= 1'b1;
                threads_issued <= threads_issued + 18'd1;
            end
        end

        thread_num <= thread_num + 4'd1;
    end

    // Condition that exit_thread_num != thread_num should always be true
    if (exit && (exit_thread_num != thread_num)) begin
        threads[exit_thread_num].busy <= 1'b0;
    end

    if (new_program.valid) begin
        // Queue input logic (leave no gaps)
        programs[0] <= programs[0].valid ? programs[0] : new_program;
        programs[1] <= programs[0].valid & ~programs[1].valid ? new_program
                                                              : programs[1];
        programs[2] <= programs[1].valid & ~programs[2].valid ? new_program
                                                              : programs[2];
        programs[3] <= programs[2].valid & ~programs[3].valid ? new_program
                                                              : programs[3];
        programs[4] <= programs[3].valid & ~programs[4].valid ? new_program
                                                              : programs[4];
        // New program should never be valid when there are already 5
        // programs in the queue
    end else if (threads_issued == programs[0].num_blocks
                 && ~threads_busy) begin
        for (int i = 0; i < 4; i++) begin
            programs[i] <= programs[i+1];
        end
        programs[4] <= {10'b0, 18'b0, 1'b0};
        threads_issued <= 18'd0;
    end
end

endmodule
