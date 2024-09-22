`include "pkg/opcode_pkg.svh"
`include "pkg/program_pkg.svh"

module cpu_interface
    import opcode_pkg::*;
    import program_pkg::*;
(
    input logic clk,
    input logic scheduler_busy,
    input logic [31:0] from_cpu,    // GPIO 0

    output logic [9:0] instr_write_addr,
    output logic [31:0] instr_write_data,
    output logic instr_write_en,
    output logic program_ready,
    output logic reset_frame,
    output program_info new_program,
    output logic [1:0] to_cpu       // GPIO 1
);

typedef enum logic [1:0] {
    WAIT_FOR_START = 2'b00,
    WAIT_FOR_EVEN = 2'b10,
    WAIT_FOR_ODD = 2'b11
} cpu_io_state_t;

// func2 for communication instructions
typedef enum logic [1:0] {
    X_START = 2'b00,
    X_INSTR = 2'b01,
    X_END = 2'b10
} x_func2_t;

cpu_io_state_t current_state;
cpu_io_state_t next_state;

opc_t opc;
x_func2_t func2;
logic [2:0] program_number;
logic [17:0] num_blocks_cpu;

logic program_valid;

always_comb begin
    opc = opc_t'(from_cpu[31:29]);
    func2 = x_func2_t'(from_cpu[28:27]);
    program_number = from_cpu[26:24];

    new_program.valid = program_valid;
    new_program.start_addr = 10'h0;
    new_program.num_blocks = num_blocks_cpu;

    case (current_state)
    WAIT_FOR_START: begin
        next_state = (opc == X_TYPE && func2 == X_START) ? WAIT_FOR_ODD
                                                         : WAIT_FOR_START;
        to_cpu = scheduler_busy ? 2'b01 : 2'b00;
    end
    WAIT_FOR_ODD: begin
        next_state = (opc == C_TYPE)                ? WAIT_FOR_START :
                     from_cpu[4] && (opc != X_TYPE) ? WAIT_FOR_EVEN
                                                    : WAIT_FOR_ODD;
        to_cpu = 2'b11;
    end
    WAIT_FOR_EVEN: begin
        next_state = (opc == C_TYPE)                 ? WAIT_FOR_START :
                     ~from_cpu[4] && (opc != X_TYPE) ? WAIT_FOR_ODD
                                                     : WAIT_FOR_EVEN;
        to_cpu = 2'b10;
    end
    default: begin
        next_state = WAIT_FOR_START;
        to_cpu = 2'b11;
    end
    endcase
end

always_ff @(posedge clk) begin
    program_ready <= (opc == X_TYPE && func2 == X_START);
    program_valid <= (opc == C_TYPE) && (current_state != WAIT_FOR_START);
    instr_write_data <= from_cpu;
    instr_write_en <= (current_state == WAIT_FOR_ODD
                       && next_state != WAIT_FOR_ODD)
                   || (current_state == WAIT_FOR_EVEN
                       && next_state != WAIT_FOR_EVEN);
    current_state <= next_state;
    instr_write_addr <= (opc == X_TYPE && func2 == X_START) ? 10'h0 :
                        instr_write_en ? (instr_write_addr + 10'h1) :
                                                  instr_write_addr ;
    num_blocks_cpu <= (opc == X_TYPE && func2 == X_START) ? from_cpu[17:0]
                                                      : num_blocks_cpu;
    // TODOTODO: MAKE THIS WORK WITH 100 as input
    reset_frame <= (opc == X_TYPE && func2 == X_START
                 && program_number == 3'b0);
end

endmodule
