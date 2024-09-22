/*
 *  Table of possibilities for how enqueue entries are ordered
 *       |          enqueue data
 *  -----|--------|--------|--------|-----
 *  loc1 | input1 |    x   |    x   |  x
 *  loc0 | input0 | input0 | input1 |  x
 */

`include "pkg/memory_pkg.svh"

module mem_queue
    import memory_pkg::*;
(
    input logic clk,
    input write_req_pkt write_req_0,
    input write_req_pkt write_req_1,
    input logic stall_back,
    input logic stall_front,

    output write_req_pkt membank_write_req,
    output logic stall_out
);

// Space for up to 3 simulatenous entries in the queue
write_req_pkt current_queue [3];
write_req_pkt next_queue [3];

write_req_pkt enqueue_0;
write_req_pkt enqueue_1;

// Controls whether inputs are enqueued
logic stall;

always_comb begin
    // Logic for determining enqueued packets
    enqueue_0.en = write_req_0.en | write_req_1.en;
    enqueue_0.forcewrite = write_req_0.en ? write_req_0.forcewrite
                                          : write_req_1.forcewrite;
    enqueue_0.data = write_req_0.en ? write_req_0.data
                                    : write_req_1.data;
    enqueue_0.addr = write_req_0.en ? write_req_0.addr
                                    : write_req_1.addr;

    enqueue_1.en = write_req_0.en & write_req_1.en;
    enqueue_1.forcewrite = write_req_1.forcewrite;
    enqueue_1.data = write_req_1.data;
    enqueue_1.addr = write_req_1.addr;

    // Output logic
    membank_write_req = current_queue[0];

    // Logic to determine state of queue in next cycle
    next_queue[0].en = current_queue[1].en | enqueue_0.en;
    next_queue[0].forcewrite = current_queue[1].en ? current_queue[1].forcewrite
                                                   : enqueue_0.forcewrite;
    next_queue[0].data = current_queue[1].en ? current_queue[1].data
                                             : enqueue_0.data;
    next_queue[0].addr = current_queue[1].en ? current_queue[1].addr
                                             : enqueue_0.addr;

    next_queue[1].en = current_queue[2].en
                     | (current_queue[1].en & enqueue_0.en)
                     | enqueue_1.en;
    next_queue[1].forcewrite = current_queue[2].en ? current_queue[2].forcewrite :
                               current_queue[1].en ? enqueue_0.forcewrite
                                                   : enqueue_1.forcewrite;
    next_queue[1].data = current_queue[2].en ? current_queue[2].data :
                         current_queue[1].en ? enqueue_0.data
                                             : enqueue_1.data;
    next_queue[1].addr = current_queue[2].en ? current_queue[2].addr :
                         current_queue[1].en ? enqueue_0.addr
                                             : enqueue_1.addr;

    next_queue[2].en = (current_queue[2].en
                        & enqueue_0.en & ~enqueue_1.en)
                     | (current_queue[1].en & ~current_queue[2].en
                        & enqueue_1.en);
    next_queue[2].forcewrite = current_queue[2].en ? enqueue_0.forcewrite
                                                   : enqueue_1.forcewrite;
    next_queue[2].data = current_queue[2].en ? enqueue_0.data
                                             : enqueue_1.data;
    next_queue[2].addr = current_queue[2].en ? enqueue_0.addr
                                             : enqueue_1.addr;

    // Stall if both inputs valid and queue is full
    stall_out = (current_queue[1].en & enqueue_1.en
              || current_queue[2].en & enqueue_0.en);
end

// Also stall if any of the other queues stall
// This needs to be outside the always_comb to avoid circular logic
assign stall = stall_out | stall_back;

always_ff @(posedge clk) begin
    if (stall & stall_front) begin
        // Queue stays the same
        current_queue <= current_queue;

    end else if (stall) begin
        // Queue outputs but does not take any inputs
        current_queue[0] <= current_queue[1];
        current_queue[1] <= current_queue[2];
        current_queue[2].en <= 1'b0; // Sufficient to disregard entry

    end else if (stall_front) begin
        // Queue takes inputs but does not output
        current_queue[0] <= current_queue[0].en ? current_queue[0]
                                                : enqueue_0;
        current_queue[1] <= next_queue[0];
        current_queue[2] <= next_queue[1];

    end else begin
        // Queue operates normally
        current_queue <= next_queue;

    end
end

endmodule
