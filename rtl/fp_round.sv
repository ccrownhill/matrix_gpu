module fp_round (
    input logic sign_in,
    input logic [6:0] exponent_in,
    input logic [16:0] mantissa_in, // with implicit one

    output logic [17:0] val_out
);

logic [6:0] precision_bits;
logic reduced_precision;

always_comb begin
    precision_bits = mantissa_in[6:0];

    reduced_precision = |precision_bits[5:0];

    if (precision_bits[6] && !reduced_precision) begin
        val_out = (mantissa_in[7]) ?
            {sign_in,exponent_in,mantissa_in[16:7]} + 18'b1 :
            {sign_in,exponent_in,mantissa_in[16:7]};
    end
    else if (precision_bits[6] && reduced_precision) begin
        val_out = {sign_in,exponent_in,mantissa_in[16:7]} + 18'b1;
    end
    else begin
        val_out = {sign_in,exponent_in,mantissa_in[16:7]};
    end
 // 01000000111111111111111111

end

endmodule
