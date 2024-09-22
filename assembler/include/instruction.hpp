#ifndef INSTRUCTION_HPP
#define INSTRUCTION_HPP

enum class InstructionOp
{
    ADD,
    SUB,
    MUL,
    DIV,
    REM,
    ABS,
    AND,
    OR,
    XOR,
    SLL,
    SRL,
    SRA,
    SLT,
    SEQ,
    ADDI,
    SUBI,
    MULI,
    DIVI,
    REMI,
    ANDI,
    ORI,
    XORI,
    SLLI,
    SRLI,
    SRAI,
    SLTI,
    SEQI,
    LUI,
    LI,
    FADD,
    FSUB,
    FMUL,
    FDIV,
    FABS,
    FRCP,
    FSQRT,
    FRSQRT,
    FSIN,
    FCOS,
    FLOG,
    FEXP,
    FSLT,
    FSEQ,
    CVTIF,
    CVTFI,
    CVTFR,
    CVTFC,
    LW,
    SW,
    SPIX,
    JUMP,
    BRANCH,
    CALL,
    RET,
    DISP,
    SYNC,
    EXIT,
    NOP
};

enum class InstructionType {
    R,  // Register
    I,  // Immediate
    U,  // Upper immediate
    F,  // Floating point
    M,  // Memory
    D,  // Display
    C,  // Control
    P   // Pseudo
};

bool is_single_operand_instr(InstructionOp op);
bool is_predicate_setter(InstructionOp op);
// Returns instruction type for given instruction op
InstructionType get_instr_type(InstructionOp op);

#endif
