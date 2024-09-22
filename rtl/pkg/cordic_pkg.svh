`ifndef CORDIC_PKG_SVH
`define CORDIC_PKG_SVH

`include "pkg/opcode_pkg.svh"

package cordic_pkg;

import opcode_pkg::*;

// CORDIC operation modese
typedef enum logic {
    ROTATION = 1'd0,
    VECTORING = 1'd1
} mode_t;

// CORDIC coordinate systems
typedef enum logic [1:0] {
    CIRCULAR = 2'd0,    // m = 1
    LINEAR = 2'd1,      // m = 0
    HYPERBOLIC =2'd2    // m = -1
} coord_t;

// Contents of CORDIC pipeline registers
typedef struct packed {
    func5_t func;
    mode_t mode;
    coord_t coord;
    logic [12:-4] x;
    logic [12:-4] y;
    logic [12:-4] z;
    logic fp_sign;
    logic [6:0] fp_exponent;
    logic override;
    logic [17:0] override_val;
    logic valid;
} cordic_reg;

endpackage

`endif
