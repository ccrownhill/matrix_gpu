/*
 *  Number format: 2 MSB guard bit, 1 implied mantissa bit,
 *                 10 true mantissa bits, 4 precision bits
 *               - MSB guard bit doubles as sign bit
 */

`include "pkg/cordic_pkg.svh"

module cordic_stage
    import cordic_pkg::*;
#(
    parameter i
)(
    input mode_t mode,
    input coord_t coord,
    input logic [12:-4] x_i,
    input logic [12:-4] y_i,
    input logic [12:-4] z_i,

    output logic [12:-4] x_i_1,
    output logic [12:-4] y_i_1,
    output logic [12:-4] z_i_1
);

// Values of arctan(2^-i)
// Uses signed fixed-point 30-bit values (first two bits before binary point)
// TODO: Update this with FPGA angle units
const logic [12:-4] arctan_table [0:10] = {
    17'h04000,  // i = 1
    17'h025c8,
    17'h013f6,
    17'h00a22,
    17'h00516,
    17'h0028c,
    17'h00146,
    17'h000a3,
    17'h00051,
    17'h00029,
    17'h00014
};

// Values of artanh(2^-i)
// Uses signed fixed-point 16-bit values (first two bits before binary point)
const logic [12:-4] artanh_table [0:10] = {
    17'hx,      // This value should never be used
    17'h02328,   // i = 1
    17'h01059,
    17'h0080b,
    17'h00401,
    17'h00200,
    17'h00100,
    17'h00080,
    17'h00040,
    17'h00020,
    17'h00010
};

logic sign;
logic [12:-4] x_shifted;
logic [12:-4] y_shifted;

always_comb begin
    // Either in rotation or vectoring mode
    // 1 if <0, 0 otherwise
    sign = (mode == ROTATION) ? z_i[12] : y_i[12] ~^ x_i[12];

    // Arithmetic shift right
    x_shifted = $signed(x_i) >>> i; // TODO: try using signed'()
    y_shifted = $signed(y_i) >>> i;

    case (coord)
    CIRCULAR: begin
        x_i_1 = sign ? x_i + y_shifted : x_i - y_shifted;
        y_i_1 = sign ? y_i - x_shifted : y_i + x_shifted;
        z_i_1 = sign ? z_i + arctan_table[i] : z_i - arctan_table[i];
    end
    LINEAR: begin
        x_i_1 = x_i;
        y_i_1 = sign ? y_i - x_shifted : y_i + x_shifted;
        z_i_1 = sign ? z_i + (17'h04000 >> i) : z_i - (17'h04000 >> i);
    end
    HYPERBOLIC: begin
        x_i_1 = sign ? x_i - y_shifted : x_i + y_shifted;
        y_i_1 = sign ? y_i - x_shifted : y_i + x_shifted;
        z_i_1 = sign ? z_i + artanh_table[i] : z_i - artanh_table[i];
    end
    default: begin
        // This case is invalid
        x_i_1 = 17'hx;
        y_i_1 = 17'hx;
        z_i_1 = 17'hx;
    end
    endcase
end

endmodule
