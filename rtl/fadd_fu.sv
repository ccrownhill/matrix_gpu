/*
 *  Handles FADD, FSUB, CVTIF and CVTFI instructions
 *  TODO: Add right-shift precision bits and rounding
 */

`include "pkg/opcode_pkg.svh"

module fadd_fu
    import opcode_pkg::*;
(
    input logic clk,
    input logic [17:0] op1_in,
    input logic [17:0] op2_in,
    input func5_t func5,
    input logic fadd_valid,

    output logic cvtfi_valid,
    output logic [17:0] cvtfi_result,

    output logic sign_out,
    output logic [6:0] exponent_out,
    output logic [17:0] mantissa_out,
    output logic std_valid_out
);

// Pipeline register between stage 0 and stage 1
typedef struct packed {
    logic fadd_valid;
    logic op1_sign;
    logic [17:0] op1;
    logic op2_sign;
    logic result_sign;
    logic is_convert;
    logic [6:0] unnormalized_exponent;
    logic [11:0] greater_mantissa;
    logic [17:0] shift_mantissa;
    logic [6:0] shift_amount;
    func5_t func5;
} fadd_01_reg;

// Pipeline register between stage 1 and stage 2
typedef struct packed {
    logic fadd_valid;
    logic op1_sign;
    logic [17:0] op1;
    logic op2_sign;
    logic result_sign;
    logic [6:0] unnormalized_exponent;
    logic [11:0] greater_mantissa;
    logic [11:0] lesser_mantissa;
    func5_t func5;
} fadd_12_reg;

// Pipeline register between stage 2 and stage 3
typedef struct packed {
    logic fadd_valid;
    logic op1_sign;
    logic [17:0] op1;
    logic result_sign;
    logic [6:0] unnormalized_exponent;
    logic [11:0] unnormalized_mantissa;
    func5_t func5;
} fadd_23_reg;

logic op1_sign_s0;
logic [6:0] op1_exponent_s0;
logic op1_implicit_bit_s0;
logic [11:0] op1_mantissa_s0;

logic op2_sign_s0;
logic [6:0] op2_exponent_s0;
logic op2_implicit_bit_s0;
logic [11:0] op2_mantissa_s0;

fadd_01_reg reg01In;
fadd_01_reg reg01Out;

fadd_12_reg reg12In;
fadd_12_reg reg12Out;

fadd_23_reg reg23In;
fadd_23_reg reg23Out;

fadd_23_reg reg23_1In;
fadd_23_reg reg23_1Out;

fadd_23_reg reg23_2In;
fadd_23_reg reg23_2Out;

fadd_23_reg reg23_3In;
fadd_23_reg reg23_3Out;

fadd_23_reg reg23_4In;
fadd_23_reg reg23_4Out;

logic [17:0] shift_result;

// stage 1
logic round_bit;

always_comb begin

    if (func5 == CVTFI) begin
        reg01In.is_convert = 1'b1;
    end
    else begin
        reg01In.is_convert = 1'b0;
    end

    // Assigning bits
    op1_sign_s0 = op1_in[17];
    op1_exponent_s0 = op1_in[16:10];
    op1_implicit_bit_s0 = (op1_exponent_s0 != 7'b0);
    op1_mantissa_s0 = {1'b0, op1_implicit_bit_s0, op1_in[9:0]};

    // Invert op2 sign for sub operation
    op2_sign_s0 = (func5 == SUB || func5 == SLT
                || func5 == SEQ) ? ~op2_in[17] : op2_in[17];
                //TODO: FIX THE MAGIC NUMBER
    op2_exponent_s0 = reg01In.is_convert ? 7'd79 : op2_in[16:10]; // dont ask about the magic number 85 its a secret
    op2_implicit_bit_s0 = (op2_exponent_s0 != 7'b0);
    op2_mantissa_s0 = {1'b0, op2_implicit_bit_s0, op2_in[9:0]};

    reg01In.func5 = func5;
    reg01In.op1_sign = op1_sign_s0;
    reg01In.op1 = op1_in;
    reg01In.op2_sign = op2_sign_s0;
    reg01In.fadd_valid = fadd_valid;

    // Stage 0: Compare exponents and mantissas
    // after this stage cvtfi will have the correct output stored in
    if ((op1_exponent_s0 > op2_exponent_s0) && !reg01In.is_convert) begin
        reg01In.unnormalized_exponent = op1_exponent_s0;
        reg01In.result_sign = op1_sign_s0;
        reg01In.greater_mantissa = op1_mantissa_s0;
        reg01In.shift_amount = (op1_exponent_s0 - op2_exponent_s0);
        reg01In.shift_mantissa[17:6] = op2_mantissa_s0;

    end else if ((op1_exponent_s0 < op2_exponent_s0) || reg01In.is_convert) begin
        reg01In.unnormalized_exponent = op2_exponent_s0;
        reg01In.result_sign = op2_sign_s0;
        reg01In.greater_mantissa = op2_mantissa_s0;
        reg01In.shift_amount = (op2_exponent_s0 - op1_exponent_s0);
        reg01In.shift_mantissa[17:6] = op1_mantissa_s0;

    end else if (op1_mantissa_s0 > op2_mantissa_s0) begin
        reg01In.unnormalized_exponent = op1_exponent_s0;
        reg01In.result_sign = op1_sign_s0;
        reg01In.greater_mantissa = op1_mantissa_s0;
        reg01In.shift_amount = 7'd0;
        reg01In.shift_mantissa[17:6] = op2_mantissa_s0;

    end else begin
        reg01In.unnormalized_exponent = op2_exponent_s0;
        reg01In.result_sign = op2_sign_s0;
        reg01In.greater_mantissa = op2_mantissa_s0;
        reg01In.shift_amount = 7'd0;
        reg01In.shift_mantissa[17:6] = op1_mantissa_s0;
    end

    // Stage 1: Shift mantissa
    round_bit = reg01Out.shift_mantissa[{reg01Out.shift_amount - 7'b1}[4:0]];
    shift_result = reg01Out.shift_mantissa >> reg01Out.shift_amount;

    reg12In.op1_sign = reg01Out.op1_sign;
    reg12In.op1 = reg01Out.op1;
    reg12In.op2_sign = reg01Out.op2_sign;
    reg12In.lesser_mantissa = shift_result[17:6];
    reg12In.greater_mantissa = reg01Out.greater_mantissa;
    reg12In.result_sign = reg01Out.result_sign;
    reg12In.unnormalized_exponent = reg01Out.unnormalized_exponent;
    reg12In.func5 = reg01Out.func5;
    reg12In.fadd_valid = reg01Out.fadd_valid & ~reg01Out.is_convert;

    // set after barrel shifting
    if(reg01Out.op1_sign) begin // ideally find better way of doing this
        cvtfi_result = ~shift_result + 18'd1 + {17'b0, round_bit};
    end
    else begin
        cvtfi_result = shift_result + {17'b0, round_bit};
    end
    cvtfi_valid = reg01Out.is_convert;

    // Stage 2: Add/subtract mantissas
    reg23In.fadd_valid = reg12Out.fadd_valid;
    reg23In.func5 = reg12Out.func5;
    reg23In.op1_sign = reg12Out.op1_sign;
    reg23In.op1 = reg12Out.op1;
    if (reg12Out.op1_sign == reg12Out.op2_sign)
        reg23In.unnormalized_mantissa =
            reg12Out.greater_mantissa + reg12Out.lesser_mantissa;
    else
        reg23In.unnormalized_mantissa =
            reg12Out.greater_mantissa - reg12Out.lesser_mantissa;

    reg23In.result_sign = reg12Out.result_sign;
    reg23In.unnormalized_exponent = reg12Out.unnormalized_exponent;

    reg23_1In = reg23Out;
    reg23_2In = reg23_1Out;
    reg23_3In = reg23_2Out;
    reg23_4In = reg23_3Out;

    std_valid_out = reg23_4Out.fadd_valid;
    if(reg23_4Out.func5 == CVTIF) begin
        exponent_out = 7'b0111111;
        mantissa_out = reg23_4Out.op1_sign ? (18'b0 - reg23_4Out.op1[17:0])
                                           : reg23_4Out.op1[17:0];
        sign_out = reg23_4Out.op1_sign;
    end
    else begin
        exponent_out = reg23_4Out.unnormalized_exponent - 7'd10;
        mantissa_out = {6'b0, reg23_4Out.unnormalized_mantissa};
        sign_out = reg23_4Out.result_sign;
    end
end

always_ff @(posedge clk) begin
    reg01Out <= reg01In;
    reg12Out <= reg12In;
    reg23Out <= reg23In;
    reg23_1Out <= reg23_1In;
    reg23_2Out <= reg23_2In;
    reg23_3Out <= reg23_3In;
    reg23_4Out <= reg23_4In;
end

endmodule
