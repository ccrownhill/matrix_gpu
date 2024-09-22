#include "util.hpp"
#include "instruction.hpp"

bool is_single_operand_instr(InstructionOp op)
{
    switch (op) {
    case InstructionOp::FABS:
    case InstructionOp::FRCP:
    case InstructionOp::FSQRT:
    case InstructionOp::FRSQRT:
    case InstructionOp::FSIN:
    case InstructionOp::FCOS:
    case InstructionOp::FLOG:
    case InstructionOp::FEXP:
    case InstructionOp::CVTIF:
    case InstructionOp::CVTFI:
    case InstructionOp::CVTFR:
    case InstructionOp::CVTFC:
        return true;
        break;
    default:
        return false;
    }
}


bool is_predicate_setter(InstructionOp op)
{
    switch (op) {
    case InstructionOp::FSLT:
    case InstructionOp::SLT:
    case InstructionOp::SLTI:
    case InstructionOp::SEQ:
    case InstructionOp::SEQI:
    case InstructionOp::FSEQ:
        return true;
        break;
    default:
        return false;
    }
}



InstructionType get_instr_type(InstructionOp op)
{
    InstructionType type;
    switch (op) {
    case InstructionOp::ADD:
    case InstructionOp::SUB:
    case InstructionOp::MUL:
    case InstructionOp::DIV:
    case InstructionOp::REM:
    case InstructionOp::ABS:
    case InstructionOp::AND:
    case InstructionOp::OR:
    case InstructionOp::XOR:
    case InstructionOp::SLL:
    case InstructionOp::SRL:
    case InstructionOp::SRA:
    case InstructionOp::SLT:
    case InstructionOp::SEQ:
        // Intentional fall-through
        type = InstructionType::R;
        break;
    case InstructionOp::ADDI:
    case InstructionOp::SUBI:
    case InstructionOp::MULI:
    case InstructionOp::DIVI:
    case InstructionOp::REMI:
    case InstructionOp::ANDI:
    case InstructionOp::ORI:
    case InstructionOp::XORI:
    case InstructionOp::SLLI:
    case InstructionOp::SRLI:
    case InstructionOp::SRAI:
    case InstructionOp::SLTI:
    case InstructionOp::SEQI:
        // Intentional fall-through
        type = InstructionType::I;
        break;
    case InstructionOp::LUI:
        type = InstructionType::U;
        break;
    case InstructionOp::LI:
        type = InstructionType::U;
        break;
    case InstructionOp::FADD:
    case InstructionOp::FSUB:
    case InstructionOp::FMUL:
    case InstructionOp::FDIV:
    case InstructionOp::FABS:
    case InstructionOp::FRCP:
    case InstructionOp::FSQRT:
    case InstructionOp::FRSQRT:
    case InstructionOp::FSIN:
    case InstructionOp::FCOS:
    case InstructionOp::FLOG:
    case InstructionOp::FEXP:
    case InstructionOp::FSLT:
    case InstructionOp::FSEQ:
    case InstructionOp::CVTIF:
    case InstructionOp::CVTFI:
    case InstructionOp::CVTFR:
    case InstructionOp::CVTFC:
        // Intentional fall-through
        type = InstructionType::F;
        break;
    case InstructionOp::LW:
    case InstructionOp::SW:
    case InstructionOp::SPIX:
        // Intentional fall-through
        type = InstructionType::M;
        break;
    case InstructionOp::DISP:
        type = InstructionType::D;
        break;
    case InstructionOp::JUMP:
    case InstructionOp::BRANCH:
    case InstructionOp::CALL:
    case InstructionOp::RET:
    case InstructionOp::SYNC:
    case InstructionOp::EXIT:
        // Intentional fall-through
        type = InstructionType::C;
        break;
    default:
        assertm(0, "Unrecognised instruction opc");
    }

    return type;
}
