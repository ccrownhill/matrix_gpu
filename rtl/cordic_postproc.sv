`include "pkg/cordic_pkg.svh"
`include "pkg/opcode_pkg.svh"

module cordic_postproc
    import cordic_pkg::*;
    import opcode_pkg::*;
(
    input func5_t func,
    input logic [12:-4] x_n,
    input logic [12:-4] y_n,
    input logic [12:-4] z_n,
    input logic passed_in_sign,
    input logic [6:0] input_exponent,
    input logic override,
    input logic [17:0] override_val,

    /* for non-cvrtfr -> result{1 bit sign, 7 bit exp, 18 bit mantissa}
       for cvrtfr     -> result{1 bit sign, 7 bit exp, 17 bit mantissa, 1'b0} */ 
    output logic [25:0] result, //1 bit sign, 7 bit exp, 18 bit mantissa
    output logic [17:0] override_output, //is equal to override_val if override high, or cvtfr
    output logic        override_out
);

// Longer input mantissa to improve CORDIC precision
// Ignore sign bit from CORDIC for now
// TODO: Investigate this
logic [12:-4] input_mantissa;
logic         result_sign;

logic [16:0]  magnitude;

always_comb begin
    // Select correct input mantissa
    case (func)
    MUL: begin
        input_mantissa = y_n[12:-4];
    end
    DIV: begin
        input_mantissa = z_n[12:-4];
    end
    SQRT: begin
        input_mantissa = x_n[12:-4];
    end
    CVTFR: begin
        input_mantissa = z_n[12:-4];
    end
    SLL: begin
        input_mantissa = y_n[12:-4];
    end
    SRL: begin
        input_mantissa = x_n[12:-4];
    end
    default: begin
        // Not relevant - copy another case
        input_mantissa = y_n[12:-4];
    end
    endcase

    if (func != CVTFR) begin
        result_sign = input_mantissa[12];
        magnitude = result_sign ? 17'b0 - input_mantissa : input_mantissa;
        result = {passed_in_sign, input_exponent - 7'd14, 1'b0, magnitude};

        //todo improve this
        override_out = override;
        override_output = {override_val};
    end

    else begin

        result = {passed_in_sign, input_exponent, input_mantissa, 1'b0}; 
        //todo improve this
        override_out = override;
        override_output = {override_val};

        /* for verilator latch set these to 0 */
        result_sign = 1'b0;
        magnitude = 17'h0;
    end

end

endmodule
