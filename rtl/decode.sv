`include "pkg/opcode_pkg.svh"

module decode
    import opcode_pkg::*;
(
    input logic [31:0] instr,

    output func5_t func5,
    output logic [17:0] imm,
    output logic [3:0] rs1,
    output logic [3:0] rs2,
    output logic [3:0] rd,
    output logic is_int,
    output logic is_imm,
    output logic is_fadd,
    output logic is_cordic,
    output logic is_mem,
    output logic is_memwrite,
    output logic is_memforce,
    output logic is_cvtfc,
    output logic is_disp,
    output logic is_predicate_setter,
    output logic is_predicate_getter,
    output logic exit
);

imm_ext ImmExt (
    .instr(instr[31:0]),

    .imm_out(imm)
);

opc_t opc;

always_comb begin
    opc = opc_t'(instr[31:29]);

    func5 = func5_t'(instr[14:10]);
    rs1 = instr[8:5];
    rs2 = instr[18:15];
    rd = instr[3:0];

    is_int = (opc == R_TYPE) || (opc == I_TYPE) || (opc == U_TYPE)
              || (opc == F_TYPE && func5 == ABS);
    is_imm = (opc == I_TYPE) || (opc == U_TYPE);
    is_fadd = (opc == F_TYPE)
            && (func5 == ADD || func5 == SUB || func5 == CVTIF
                || func5 == CVTFI || func5 == SLT || func5 == SEQ);
    // SLL is SIN and SRL is COS
    is_cordic = (opc == F_TYPE)
              && (func5 == MUL || func5 == DIV || func5 == SQRT
               || func5 == SLL || func5 == SRL || func5 == CVTFR);
    is_mem = (opc == M_TYPE);
    is_memwrite = (opc == M_TYPE && instr[10]);
    is_memforce = (opc == M_TYPE && instr[11:10] == 2'b01);
    is_cvtfc = (opc == F_TYPE && func5 == CVTFC);
    is_disp = (opc == D_TYPE);
    is_predicate_setter = (func5 == SLT || func5 == SEQ);
    is_predicate_getter = instr[28]; // bit 28 is whether insruction is predicated
    exit = (opc == C_TYPE); // Only implemented control instruction is exit
end

endmodule
