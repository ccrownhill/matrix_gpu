/*
 *  Splits memory writes between two banks:
 *  - Even addresses enter queue 0 and go to bank 0
 *  - Odd addresses enter queue 1 and go to bank 1
 *  - MMU stalls if either queue would overflow
 *  Reads are always consecutive and aligned, and do not use the queues.
 *  Reads happen on the positive clock edge, and so results appear on the
 *  next clock cycle.
 */

`include "pkg/memory_pkg.svh"

module mmu
    import memory_pkg::*;
(
    input logic clk,
    input write_req_pkt write_reqs [8],

    output logic [8:0] output_read_data [8],
    output logic stall
);

// Stall from back is equivalent to stall from MMU inputs -> banks
// Stall from front is equivalent to stall from banks -> MMU inputs
write_req_pkt s1_input_reqs [8][2];
write_req_pkt s1_output_reqs [8];
logic [7:0] s1_stalls;
logic stall_s1_back;
logic stall_s1_front;

write_req_pkt s2_input_reqs [8][2];
write_req_pkt s2_output_reqs [8];
logic [7:0] s2_stalls;
logic stall_s2_back;
logic stall_s2_front;

write_req_pkt s3_input_reqs [8][2];
write_req_pkt s3_output_reqs [8];
logic [7:0] s3_stalls;
logic stall_s3_back;
logic stall_s3_front;

write_req_pkt membank_input_reqs [8];
logic [17:0] read_req_addrs [8];
logic [8:0] membank_read_data [8];

logic [2:0] read_offset_s0;
logic [2:0] read_offset_s1;

genvar k;
generate
    // Groups adresses modulo 2
    for (k = 0; k < 8; k++) begin: Stage1Queues
        mem_queue Queue (
            .clk(clk),
            .write_req_0(s1_input_reqs[(k/2)*2][k%2]),
            .write_req_1(s1_input_reqs[((k/2)*2)+1][k%2]),
            .stall_back(stall_s1_back),
            .stall_front(stall_s1_front),

            .membank_write_req(s1_output_reqs[k]),
            .stall_out(s1_stalls[k])
        );
    end

    // Groups addresses modulo 4
    for (k = 0; k < 8; k++) begin: Stage2Queues
        mem_queue Queue (
            .clk(clk),
            .write_req_0(s2_input_reqs[((k/4)*4)+(k%2)][(k/2)%2]),
            .write_req_1(s2_input_reqs[((k/4)*4)+(k%2)+2][(k/2)%2]),
            .stall_back(stall_s2_back),
            .stall_front(stall_s2_front),

            .membank_write_req(s2_output_reqs[k]),
            .stall_out(s2_stalls[k])
        );
    end

    // Groups addresses modulo 8
    for (k = 0; k < 8; k++) begin: Stage3Queues
        mem_queue Queue (
            .clk(clk),
            .write_req_0(s3_input_reqs[k%4][k/4]),
            .write_req_1(s3_input_reqs[(k%4)+4][k/4]),
            .stall_back(stall_s3_back),
            .stall_front(stall_s3_front),

            .membank_write_req(s3_output_reqs[k]),
            .stall_out(s3_stalls[k])
        );
    end

    // Addresses are divided between the memory banks by modulo 8
    for (k = 0; k < 8; k++) begin: Banks
        mem_bank Bank (
            .clk(clk),
            .write_req(membank_input_reqs[k]),

            .read_data(membank_read_data[k])
        );
    end
endgenerate

always_comb begin
    // Inputs to stage 1 (from MMU inputs)
    for (int i = 0; i < 8; i++) begin
        s1_input_reqs[i][0].en = write_reqs[i].en & ~write_reqs[i].addr[0];
        s1_input_reqs[i][0].forcewrite = write_reqs[i].forcewrite;
        s1_input_reqs[i][0].data = write_reqs[i].data;
        s1_input_reqs[i][0].addr = write_reqs[i].addr;
        s1_input_reqs[i][1].en = write_reqs[i].en & write_reqs[i].addr[0];
        s1_input_reqs[i][1].forcewrite = write_reqs[i].forcewrite;
        s1_input_reqs[i][1].data = write_reqs[i].data;
        s1_input_reqs[i][1].addr = write_reqs[i].addr;
    end

    // Inputs to stage 2 (from stage 1 outputs)
    for (int i = 0; i < 8; i++) begin
        s2_input_reqs[i][0].en = s1_output_reqs[i].en & ~s1_output_reqs[i].addr[1];
        s2_input_reqs[i][0].forcewrite = s1_output_reqs[i].forcewrite;
        s2_input_reqs[i][0].data = s1_output_reqs[i].data;
        s2_input_reqs[i][0].addr = s1_output_reqs[i].addr;
        s2_input_reqs[i][1].en = s1_output_reqs[i].en & s1_output_reqs[i].addr[1];
        s2_input_reqs[i][1].forcewrite = s1_output_reqs[i].forcewrite;
        s2_input_reqs[i][1].data = s1_output_reqs[i].data;
        s2_input_reqs[i][1].addr = s1_output_reqs[i].addr;
    end

    // Inputs to stage 3 (from stage 2 outputs)
    for (int i = 0; i < 8; i++) begin
        s3_input_reqs[i][0].en = s2_output_reqs[i].en & ~s2_output_reqs[i].addr[2];
        s3_input_reqs[i][0].forcewrite = s2_output_reqs[i].forcewrite;
        s3_input_reqs[i][0].data = s2_output_reqs[i].data;
        s3_input_reqs[i][0].addr = s2_output_reqs[i].addr;
        s3_input_reqs[i][1].en = s2_output_reqs[i].en & s2_output_reqs[i].addr[2];
        s3_input_reqs[i][1].forcewrite = s2_output_reqs[i].forcewrite;
        s3_input_reqs[i][1].data = s2_output_reqs[i].data;
        s3_input_reqs[i][1].addr = s2_output_reqs[i].addr;
    end

    // Inputs to memory banks (from stage 3 outputs)
    for (int i = 0; i < 8; i++) begin
        // All membanks take their input address from lane 0 for
        // a read since reads are aligned and consecutive
        membank_input_reqs[i].addr = s3_output_reqs[i].en ? s3_output_reqs[i].addr
                                                          : read_req_addrs[i];
        membank_input_reqs[i].en = s3_output_reqs[i].en;
        membank_input_reqs[i].forcewrite = s3_output_reqs[i].forcewrite;
        membank_input_reqs[i].data = s3_output_reqs[i].data;
    end

    // Stall logic
    // Each stage can stall independently. When any queue in a stage stalls,
    // all other queues in that stage must stall as well.
    stall_s3_back = |s3_stalls;
    stall_s3_front = 1'b0;

    stall_s2_back = |s2_stalls;
    stall_s2_front = stall_s3_back;

    stall_s1_back = |s1_stalls;
    stall_s1_front = stall_s2_back;

    stall = stall_s1_back;
end

assign read_offset_s0 = write_reqs[0].addr[2:0];

always_comb begin
    // Separate parallel logic for muxing membank read address
    // input to avoid circular logic warnings
    read_req_addrs[read_offset_s0] = write_reqs[0].addr;
    read_req_addrs[read_offset_s0 + 3'd1] = write_reqs[1].addr;
    read_req_addrs[read_offset_s0 + 3'd2] = write_reqs[2].addr;
    read_req_addrs[read_offset_s0 + 3'd3] = write_reqs[3].addr;
    read_req_addrs[read_offset_s0 + 3'd4] = write_reqs[4].addr;
    read_req_addrs[read_offset_s0 + 3'd5] = write_reqs[5].addr;
    read_req_addrs[read_offset_s0 + 3'd6] = write_reqs[6].addr;
    read_req_addrs[read_offset_s0 + 3'd7] = write_reqs[7].addr;
end

always_comb begin
    // Separate parallel logic for muxing membank output to
    // avoid circular logic warnings
    output_read_data[0] = membank_read_data[read_offset_s1];
    output_read_data[1] = membank_read_data[read_offset_s1 + 3'd1];
    output_read_data[2] = membank_read_data[read_offset_s1 + 3'd2];
    output_read_data[3] = membank_read_data[read_offset_s1 + 3'd3];
    output_read_data[4] = membank_read_data[read_offset_s1 + 3'd4];
    output_read_data[5] = membank_read_data[read_offset_s1 + 3'd5];
    output_read_data[6] = membank_read_data[read_offset_s1 + 3'd6];
    output_read_data[7] = membank_read_data[read_offset_s1 + 3'd7];
end

always_ff @(posedge clk) begin
    // Stalls cannot happen while the MMU is reading
    // Reads cannot happen at the same time as writes (assembler enforced)
    read_offset_s1 <= read_offset_s0;
end

endmodule
