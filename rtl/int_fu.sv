/*
 *  Handles the following instructions:
 *  ADD, ADDI, SUB, SUBI, FABS, AND, ANDI, SLL, SLLI, SRL, SRLI,
 *  SLT, SLTI, SEQ, SEQI, LUI
 */

`include "pkg/opcode_pkg.svh"

module int_fu
    import opcode_pkg::*;
(
    input logic [17:0] reg1,
    input logic [17:0] reg2,
    input logic [17:0] imm,
    input logic is_imm,
    input func5_t func5,

    output logic [17:0] result
);

logic [17:0] op2;

always_comb begin
    op2 = is_imm ? imm : reg2;

    case (func5)
        ADD: result = reg1 + op2;
        SUB: result = reg1 - op2;
        ABS: result = {1'b0, reg1[16:0]};   // This is an FABS instruction
        AND: result = reg1 & op2;           // Integer ABS is not supported
        SLL: result = reg1 << op2[4:0];
        SRL: result = reg1 >> op2[4:0];
        SLT: result = {17'b0, reg1 < op2};
        SEQ: result = {17'b0, reg1 == op2};
        LUI: result = op2;
        default: result = 18'hx;
    endcase
end

endmodule
