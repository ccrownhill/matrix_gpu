`include "pkg/opcode_pkg.svh"

module fp_standardise
    import opcode_pkg::*;
(
    input logic clk,

    input func5_t      func5,
    input logic        valid_out_cordic,
    input logic [25:0] result_cordic,
    input logic        override_cordic,
    input logic [17:0] override_cordic_val,

    input logic valid_fadd,
    input logic fadd_sign_in,
    input logic [6:0] fadd_exponent_in,
    input logic [17:0] fadd_mantissa_in,

    output logic valid_std,
    output logic [17:0] res_std,
    output logic set_pred,
    output logic new_pred_val
);

//pipeline reg between stages 0 and 1
typedef struct packed {
    func5_t      func5;
    logic        valid_out;
    logic        override;
    logic [17:0] override_val;
} write_info_reg;

//pipeline reg between between input
//to module and output to normaliser
typedef struct packed {
    func5_t func5;
    logic        valid_out_cordic;
    logic [25:0] result_cordic;
    logic        override_cordic;
    logic [17:0] override_cordic_val;

    logic valid_fadd;
    logic fadd_sign_in;
    logic [6:0] fadd_exponent_in;
    logic [17:0] fadd_mantissa_in;
} input_pipeline_reg;

typedef struct packed {
    logic               sign;
    logic [6:0]         exponent;
    logic [16:0]        mantissa;
} normalize_to_round_reg;

//pipeline for cordic utility signals
write_info_reg cordicUtilityRegs_1_In;
write_info_reg cordicUtilityRegs_1_Out;

write_info_reg cordicUtilityRegs_2_In;
write_info_reg cordicUtilityRegs_2_Out;
//todo add one more here

//input pipeline regs
input_pipeline_reg inputRegsIn;
input_pipeline_reg inputRegsOut;

//regs between standardasize and round
normalize_to_round_reg normalizeToRoundRegIn;
normalize_to_round_reg normalizeToRoundRegOut;

logic is_cvtfr;

logic [17:0] rounded_tf18;
logic [17:0] rounded_tf18_or_cvtfr;

logic [25:0] normal_input;

fp_normalise  FpNormalise (
    .clk(clk),
    .input_sign(normal_input[25]),
    .input_exponent(normal_input[24:18]),
    .input_mantissa(normal_input[17:0]),

    .is_cvtfr(is_cvtfr),

    .output_sign(normalizeToRoundRegIn.sign),
    .output_exponent(normalizeToRoundRegIn.exponent),
    .output_mantissa(normalizeToRoundRegIn.mantissa) //contains full res of cvtfr
);

fp_round FpRound (
    .sign_in(normalizeToRoundRegOut.sign),
    .exponent_in(normalizeToRoundRegOut.exponent),
    .mantissa_in(normalizeToRoundRegOut.mantissa), // with implicit one

    .val_out(rounded_tf18)
);

always_comb begin
    is_cvtfr = (inputRegsOut.func5 == CVTFR);
    //Input regs connections
    inputRegsIn.func5 = func5;
    inputRegsIn.valid_out_cordic = valid_out_cordic;
    inputRegsIn.result_cordic = result_cordic;
    inputRegsIn.override_cordic = override_cordic;
    inputRegsIn.override_cordic_val = override_cordic_val;

    inputRegsIn.valid_fadd = valid_fadd;
    inputRegsIn.fadd_sign_in = fadd_sign_in;
    inputRegsIn.fadd_exponent_in = fadd_exponent_in;
    inputRegsIn.fadd_mantissa_in = fadd_mantissa_in;

    //Cordic regs connections
    cordicUtilityRegs_1_In.func5 = inputRegsOut.func5;
    cordicUtilityRegs_1_In.valid_out = inputRegsOut.valid_out_cordic | inputRegsOut.valid_fadd;
    cordicUtilityRegs_1_In.override = inputRegsOut.override_cordic & inputRegsOut.valid_out_cordic;
    cordicUtilityRegs_1_In.override_val = inputRegsOut.override_cordic_val;

    //Regs between normalize and round modules
    cordicUtilityRegs_2_In.func5 = cordicUtilityRegs_1_Out.func5;
    cordicUtilityRegs_2_In.valid_out = cordicUtilityRegs_1_Out.valid_out;
    cordicUtilityRegs_2_In.override = cordicUtilityRegs_1_Out.override;
    cordicUtilityRegs_2_In.override_val = cordicUtilityRegs_1_Out.override_val;

    valid_std = cordicUtilityRegs_2_Out.valid_out;
    //output_mantissa contains the res of cvtfr
    rounded_tf18_or_cvtfr = cordicUtilityRegs_2_Out.func5 == CVTFR ?
                            {normalizeToRoundRegOut.mantissa, 1'b0} : rounded_tf18;
    res_std = cordicUtilityRegs_2_Out.override ?
                                cordicUtilityRegs_2_Out.override_val : rounded_tf18_or_cvtfr;

    if(inputRegsOut.valid_out_cordic) begin
        normal_input[25] = inputRegsOut.result_cordic[25];
        normal_input[24:18] = inputRegsOut.result_cordic[24:18];
        normal_input[17:0] = inputRegsOut.result_cordic[17:0];
    end
    else begin
        normal_input[25] = inputRegsOut.fadd_sign_in;
        normal_input[24:18] = inputRegsOut.fadd_exponent_in;
        normal_input[17:0] = inputRegsOut.fadd_mantissa_in;
    end

    // Determine predicate result after rounding
    set_pred = cordicUtilityRegs_2_Out.valid_out
             && (cordicUtilityRegs_2_Out.func5 == SEQ
                 || cordicUtilityRegs_2_Out.func5 == SLT);

    if (cordicUtilityRegs_2_Out.func5 == SEQ) begin
        new_pred_val = (rounded_tf18 == 18'h0 || rounded_tf18 == 18'h20000);
    end else if (cordicUtilityRegs_2_Out.func5 == SLT) begin
        new_pred_val = rounded_tf18[17];
    end else begin
        new_pred_val = 1'b0;
    end
end

always_ff @(posedge clk) begin
    inputRegsOut <= inputRegsIn;
    normalizeToRoundRegOut <= normalizeToRoundRegIn;
    cordicUtilityRegs_1_Out <= cordicUtilityRegs_1_In;
    cordicUtilityRegs_2_Out <= cordicUtilityRegs_2_In;
end



endmodule
