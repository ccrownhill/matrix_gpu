`include "pkg/opcode_pkg.svh"

module imm_ext
    import opcode_pkg::*;
(
    /* verilator lint_off UNUSEDSIGNAL */
    // Some instr bits are not used
    input logic [31:0] instr,
    /* verilator lint_on UNUSEDSIGNAL */

    output logic [17:0] imm_out
);

opc_t opc;

always_comb begin
    opc = opc_t'(instr[31:29]);
    case (opc)
        I_TYPE: imm_out = {{6{instr[27]}}, instr[26:15]};
        U_TYPE: imm_out = {instr[27:15], instr[9:5]};
        default: imm_out = 18'bx; // Other instructions don't use immediate
    endcase
end

endmodule
