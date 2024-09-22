module fp_normalise (
    input logic clk,
    input logic input_sign,
    input logic [6:0] input_exponent,
    input logic [17:0] input_mantissa,

    input logic        is_cvtfr,

    output logic output_sign,
    output logic [6:0] output_exponent,
    output logic [16:0] output_mantissa
);

typedef struct packed {
    logic sign;
    logic [6:0] exponent;
    logic [17:0] mantissa;
    logic [4:0] one_pos;
    logic found_one;
} normalise_pipeline_reg;

typedef struct packed {
    logic [6:0] cvtfr_norm_exponent;
    logic [17:0] cvtfr_norm_extended_mantissa;
    logic passed_in_sign;
    logic is_cvtfr;
} cvtfr_pipeline_reg;

normalise_pipeline_reg pipelineRegIn;
normalise_pipeline_reg pipelineRegOut;

cvtfr_pipeline_reg cvtfrRegIn;
cvtfr_pipeline_reg cvtfrRegOut;

/* signals for normalising */
logic [4:0] upper_one_pos;
logic [4:0] lower_one_pos;
logic found_upper_one;
logic found_lower_one;

/* signals for CVTFR */
logic [6:0]   cvtfr_shamt;
logic [6:0]   cvtfr_norm_exponent;
logic [17:0]  cvtfr_norm_extended_mantissa;
logic [17:0]  expanded_res_mantissa;
logic [14:0]  expanded_cvtfr_result;

/* res of cvtfr is in output mantissa,
-> two temp_output mantissas */
    logic [16:0] output_mantissa_normalisation;
    logic [16:0] output_mantissa_cvtfr;


always_comb begin
    // Stage 0: Find the position of the first 1 in input mantissa
    // FADD corrects for negative numbers in CVTIF
    pipelineRegIn.sign = input_sign;
    pipelineRegIn.exponent = input_exponent;
    pipelineRegIn.mantissa = input_mantissa;

    // Find first one in bits 17:9
    upper_one_pos = 5'd18;
    for (found_upper_one = 1'b0;
         upper_one_pos != 5'd9 && !found_upper_one; ) begin
        upper_one_pos = upper_one_pos - 5'd1;
        found_upper_one = input_mantissa[upper_one_pos];
    end

    // Find first one in bits 8:0
    lower_one_pos = 5'd9;
    for (found_lower_one = 1'b0;
         lower_one_pos != 5'd0 && !found_lower_one; ) begin
        lower_one_pos = lower_one_pos - 5'd1;
        found_lower_one = input_mantissa[lower_one_pos];
    end

    pipelineRegIn.one_pos = found_upper_one ? upper_one_pos
                                            : lower_one_pos;
    pipelineRegIn.found_one = found_upper_one | found_lower_one;

    //Stage 1: Shift the mantissa and add offset to the exponent;
    output_sign = pipelineRegOut.sign;
    // Special case for zero
    output_exponent = pipelineRegOut.found_one ?
                      ({2'b00, pipelineRegOut.one_pos} + pipelineRegOut.exponent)
                      : 7'b0;
    output_mantissa_normalisation = {pipelineRegOut.mantissa
                       << (5'd17 - pipelineRegOut.one_pos)}[16:0];

    /* cvtfr logic -------------------- */
     /* normalisation shortcut because we always divide by 2pi */
        /* expanded_res_mantissa is either 000.1xxx or 000.01xxx */

    expanded_res_mantissa = input_mantissa;

    if(expanded_res_mantissa[17:14] == 4'b0001) begin
        //shift mantissa left by 1
        cvtfr_norm_exponent = input_exponent - 7'd1;
        cvtfr_norm_extended_mantissa = expanded_res_mantissa << 1;
    end
    else if(expanded_res_mantissa[17:13] == 5'b00001) begin
        //shift mantissa left by 1
        cvtfr_norm_exponent = input_exponent - 7'd2;
        cvtfr_norm_extended_mantissa = expanded_res_mantissa << 2;
    end
    else begin
        cvtfr_norm_exponent = 7'h0;
        cvtfr_norm_extended_mantissa = 18'h0;
    end

    /* values now normalised and pass them into the next pipeline */
    cvtfrRegIn.cvtfr_norm_exponent = cvtfr_norm_exponent;
    cvtfrRegIn.cvtfr_norm_extended_mantissa = cvtfr_norm_extended_mantissa;
    cvtfrRegIn.passed_in_sign = input_sign;
    cvtfrRegIn.is_cvtfr = is_cvtfr;

    /* trivial case */
    if (cvtfrRegOut.cvtfr_norm_exponent == 7'd63) begin
        expanded_cvtfr_result = cvtfrRegOut.cvtfr_norm_extended_mantissa[14:0];
        cvtfr_shamt = 7'h0; //needed for verilator warning
    end
    /* need to left shift res mantissa */
    else if(cvtfrRegOut.cvtfr_norm_exponent > 7'd63) begin
        cvtfr_shamt = cvtfrRegOut.cvtfr_norm_exponent - 7'd63;
        if(cvtfr_shamt < 14) begin
            expanded_cvtfr_result =
                {cvtfrRegOut.cvtfr_norm_extended_mantissa << cvtfr_shamt}[14:0];
        end
        /* if shift too big we set result to 0 */
        else begin
            expanded_cvtfr_result = 15'h0;
        end
    end
    /* need to right shift res mantissa */
    else begin
        cvtfr_shamt = 7'd63 - cvtfrRegOut.cvtfr_norm_exponent;
        if(7'd63 - cvtfrRegOut.cvtfr_norm_exponent < 14) begin
            expanded_cvtfr_result =
                {cvtfrRegOut.cvtfr_norm_extended_mantissa >> cvtfr_shamt}[14:0];
        end
        /* if shift too big we set result to 0 */
        else begin
            expanded_cvtfr_result = 15'h0;
        end

    end
    output_mantissa_cvtfr = cvtfrRegOut.passed_in_sign ? 17'b0 - {expanded_cvtfr_result, 2'b0} :
                                    {expanded_cvtfr_result, 2'b0};

    output_mantissa = cvtfrRegOut.is_cvtfr ? output_mantissa_cvtfr : output_mantissa_normalisation;
    /* dontcares */
end

always_ff @(posedge clk) begin
    pipelineRegOut <= pipelineRegIn;
    cvtfrRegOut <= cvtfrRegIn;
end

endmodule
