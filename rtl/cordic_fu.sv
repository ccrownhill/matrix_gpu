`include "pkg/cordic_pkg.svh"
`include "pkg/opcode_pkg.svh"

module cordic_fu
    import cordic_pkg::*;
    import opcode_pkg::*;
(
    input logic clk,
    input func5_t func_in,
    input logic valid_in,
    input logic [17:0] op1_in,
    input logic [17:0] op2_in,

    output func5_t func_out,
    output logic valid_out,
    output logic [25:0] result, //todo need to be changed
    output logic       override_out,
    output logic [17:0] override_val_out
);

/*
 * Registers 0...4 have input of stages 0...4a
 * Register 5 has input of stage 4b
 * Registers 6...11 have inputs of stages 5...10
 * Register 12 has inputs of postprocessor
 */

cordic_reg [6:0] pipelineRegsIn;
cordic_reg [6:0] pipelineRegsOut;
cordic_reg [5:0] intermWiresIn;
cordic_reg [5:0] intermWiresOut;

cordic_preproc CordicPreproc (
    .func(func_in),
    .op1(op1_in),
    .op2(op2_in),

    .mode(pipelineRegsIn[0].mode),
    .coord(pipelineRegsIn[0].coord),
    .x_0(pipelineRegsIn[0].x),
    .y_0(pipelineRegsIn[0].y),
    .z_0(pipelineRegsIn[0].z),
    .output_sign(pipelineRegsIn[0].fp_sign),
    .output_exponent(pipelineRegsIn[0].fp_exponent),
    .override(pipelineRegsIn[0].override),
    .override_val(pipelineRegsIn[0].override_val)
);

genvar k;   // Used for both generate constructs
generate
    /* stages 0 to 3 */
    for (k = 0; k < 2; k++) begin: Stages0_3
        cordic_stage #(2*k) Stage_evens_0 (
            .mode(pipelineRegsOut[k].mode),
            .coord(pipelineRegsOut[k].coord),
            .x_i(pipelineRegsOut[k].x),
            .y_i(pipelineRegsOut[k].y),
            .z_i(pipelineRegsOut[k].z),

            .x_i_1(intermWiresIn[k].x),
            .y_i_1(intermWiresIn[k].y),
            .z_i_1(intermWiresIn[k].z)
        );
        cordic_stage #(2*k+1) Stage_odds_0 (
            .mode(intermWiresOut[k].mode),
            .coord(intermWiresOut[k].coord),
            .x_i(intermWiresOut[k].x),
            .y_i(intermWiresOut[k].y),
            .z_i(intermWiresOut[k].z),

            .x_i_1(pipelineRegsIn[k+1].x),
            .y_i_1(pipelineRegsIn[k+1].y),
            .z_i_1(pipelineRegsIn[k+1].z)
        );
    end

    /* stages 4a */
    cordic_stage #(4) Stage4a (
            .mode(pipelineRegsOut[2].mode),
            .coord(pipelineRegsOut[2].coord),
            .x_i(pipelineRegsOut[2].x),
            .y_i(pipelineRegsOut[2].y),
            .z_i(pipelineRegsOut[2].z),

            .x_i_1(intermWiresIn[2].x),
            .y_i_1(intermWiresIn[2].y),
            .z_i_1(intermWiresIn[2].z)
    );
    /* stages 4b */
    cordic_stage #(4) Stage4b (
        .mode(intermWiresOut[2].mode),
        .coord(intermWiresOut[2].coord),
        .x_i(intermWiresOut[2].x),
        .y_i(intermWiresOut[2].y),
        .z_i(intermWiresOut[2].z),

        .x_i_1(pipelineRegsIn[3].x),
        .y_i_1(pipelineRegsIn[3].y),
        .z_i_1(pipelineRegsIn[3].z)
    );

    for (k = 3; k < 6; k++) begin: Stages5_10
        cordic_stage #(2*k-1) Stage_odds_1 (
            .mode(pipelineRegsOut[k].mode),
            .coord(pipelineRegsOut[k].coord),
            .x_i(pipelineRegsOut[k].x),
            .y_i(pipelineRegsOut[k].y),
            .z_i(pipelineRegsOut[k].z),

            .x_i_1(intermWiresIn[k].x),
            .y_i_1(intermWiresIn[k].y),
            .z_i_1(intermWiresIn[k].z)
        );
        cordic_stage #(2*k) Stage_evens_1 (
            .mode(intermWiresOut[k].mode),
            .coord(intermWiresOut[k].coord),
            .x_i(intermWiresOut[k].x),
            .y_i(intermWiresOut[k].y),
            .z_i(intermWiresOut[k].z),

            .x_i_1(pipelineRegsIn[k+1].x),
            .y_i_1(pipelineRegsIn[k+1].y),
            .z_i_1(pipelineRegsIn[k+1].z)
        );
    end
endgenerate

cordic_postproc CordicPostproc (
    .func(pipelineRegsOut[6].func),
    .x_n(pipelineRegsOut[6].x),
    .y_n(pipelineRegsOut[6].y),
    .z_n(pipelineRegsOut[6].z),
    .passed_in_sign(pipelineRegsOut[6].fp_sign),
    .input_exponent(pipelineRegsOut[6].fp_exponent),
    .override(pipelineRegsOut[6].override),
    .override_val(pipelineRegsOut[6].override_val),

    .result(result),
    .override_output(override_val_out),
    .override_out(override_out)
);

always_comb begin
    pipelineRegsIn[0].func = func_in;
    pipelineRegsIn[0].valid = valid_in;

    for (int i = 1; i <= 6; i++) begin
        pipelineRegsIn[i].func = intermWiresOut[i-1].func;
        pipelineRegsIn[i].mode = intermWiresOut[i-1].mode;
        pipelineRegsIn[i].coord = intermWiresOut[i-1].coord;
        pipelineRegsIn[i].fp_sign = intermWiresOut[i-1].fp_sign;
        pipelineRegsIn[i].fp_exponent = intermWiresOut[i-1].fp_exponent;
        pipelineRegsIn[i].valid = intermWiresOut[i-1].valid;
        pipelineRegsIn[i].override = intermWiresOut[i-1].override;
        pipelineRegsIn[i].override_val = intermWiresOut[i-1].override_val;
    end

    for (int i = 0; i <= 5; i++) begin
        intermWiresIn[i].func = pipelineRegsOut[i].func;
        intermWiresIn[i].mode = pipelineRegsOut[i].mode;
        intermWiresIn[i].coord = pipelineRegsOut[i].coord;
        intermWiresIn[i].fp_sign = pipelineRegsOut[i].fp_sign;
        intermWiresIn[i].fp_exponent = pipelineRegsOut[i].fp_exponent;
        intermWiresIn[i].valid = pipelineRegsOut[i].valid;
        intermWiresIn[i].override = pipelineRegsOut[i].override;
        intermWiresIn[i].override_val = pipelineRegsOut[i].override_val;
    end

    valid_out = pipelineRegsOut[6].valid;
    func_out = pipelineRegsOut[6].func;
end

always_comb begin

    // Only use stage 0 if using circular cords
    if (pipelineRegsOut[0].coord == CIRCULAR) begin
        intermWiresOut[0] = intermWiresIn[0];
    end else begin
        intermWiresOut[0] = pipelineRegsOut[0];
    end

    for (int i = 1; i <= 5; i++) begin
        intermWiresOut[i] = intermWiresIn[i];
    end


end

always_ff @(posedge clk) begin
    pipelineRegsOut[0] <= pipelineRegsIn[0];

    for (int i = 1; i <= 2; i++) begin
        pipelineRegsOut[i] <= pipelineRegsIn[i];
    end

    if (intermWiresOut[2].coord == HYPERBOLIC) begin
        pipelineRegsOut[3] <= pipelineRegsIn[3];
    end else begin
        pipelineRegsOut[3] <= intermWiresOut[2];
    end

    for (int i = 4; i <= 6; i++) begin
        pipelineRegsOut[i] <= pipelineRegsIn[i];
    end
end

endmodule
