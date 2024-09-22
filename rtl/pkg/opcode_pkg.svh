`ifndef OPCODE_PKG_SVH
`define OPCODE_PKG_SVH

package opcode_pkg;

typedef enum logic [2:0] {
    R_TYPE = 3'b000,
    I_TYPE = 3'b001,
    U_TYPE = 3'b010,
    F_TYPE = 3'b011,
    M_TYPE = 3'b100,
    X_TYPE = 3'b101,
    D_TYPE = 3'b110,
    C_TYPE = 3'b111
} opc_t;

typedef enum logic [4:0] {
    ADD   = 5'b00000,
    SUB   = 5'b00001,
    MUL   = 5'b00010,
    DIV   = 5'b00100,
    ABS   = 5'b00110,
    AND   = 5'b00111,
    SQRT  = 5'b01000,
    SLL   = 5'b01010, //this is a sin as well
    SRL   = 5'b01011, //this is a cos as well
    CVTIF = 5'b10000,
    CVTFI = 5'b10001,
    CVTFR = 5'b10010,
    CVTFC = 5'b10011,
    SLT   = 5'b11100,
    SEQ   = 5'b11101,
    LUI   = 5'b11111
} func5_t;

endpackage

`endif
