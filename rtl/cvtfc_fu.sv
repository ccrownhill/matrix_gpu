/*
 *  Handles the CVTFC instruction
 *  Maps a floating point value to an index into a colour lookup table
 *  This colour lookup table is implemented in disp_fu.sv
 */

module cvtfc_fu (
    input logic [17:0] tf18_value,

    output logic [8:0] colour_index
);

logic [6:0] shift_amount;

always_comb begin
    if (tf18_value[17]) begin
        // Negative numbers map to white (always overwritten)
        colour_index = 9'd0;
        shift_amount = 7'hx;
    end else if (tf18_value >= 18'h0fc00) begin
        // Numbers greater than one map to white (never overwritten)
        colour_index = 9'd511;
        shift_amount = 7'hx;
    end else if (tf18_value == 18'h800ff) begin
        // Zero maps to purple (least mapped value)
        colour_index = 9'd1;
        shift_amount = 7'hx;
    end else begin
        // Numbers between zero and one map to colours on a spectrum
        shift_amount = 7'd63 - tf18_value[16:10];
        // Denormalize floating point mantissa to get colour index
        colour_index = {{1'b1, tf18_value[9:0]} >> shift_amount}[9:1] + 9'b1;
    end
end

endmodule
