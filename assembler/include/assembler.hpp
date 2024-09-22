#ifndef ASSEMBLER_HPP
#define ASSEMBLER_HPP

#include <cstdint>
#include <string>
#include <unordered_map>
#include <vector>

#include "instruction.hpp"

class Assembler
{
public:
    Assembler() {}
    ~Assembler() {}

    // Converts asm input to binary output
    void assemble(std::istream &asm_file, std::ostream &bin_file, std::string format);

private:

    void writebin(std::istream &asm_file, std::ostream &bin_file);

    void writehex(std::istream &asm_file, std::ostream &hex_file);
    // Splits line into tokens and adds them to current tokens
    std::vector<std::string> tokenize(const std::string &line);

    // Encodes assembly instruction tokens into binary instruction(s)
    std::vector<uint32_t> encode(std::vector<std::string> &tokens);


    // Returns func5 for instruction op
    // Used for R, I, and F-type instructions
    uint32_t get_func5(InstructionOp op);

    // Converts a string immediate token to a 32-bit immediate
    uint32_t get_imm(const std::string &imm_token);

    // Converts register token into register number
    uint32_t get_reg(std::string token);

    // Checks whether character is valid for assembly language
    bool valid_char(char character);

    // Mapping from instruction strings to enums
    const std::unordered_map<std::string, InstructionOp> instr_map_ {
        {"add", InstructionOp::ADD},
        {"sub", InstructionOp::SUB},
        {"mul", InstructionOp::MUL},
        {"div", InstructionOp::DIV},
        {"rem", InstructionOp::REM},
        {"and", InstructionOp::AND},
        {"or", InstructionOp::OR},
        {"xor", InstructionOp::XOR},
        {"sll", InstructionOp::SLL},
        {"srl", InstructionOp::SRL},
        {"sra", InstructionOp::SRA},
        {"slt", InstructionOp::SLT},
        {"seq", InstructionOp::SEQ},
        {"addi", InstructionOp::ADDI},
        {"subi", InstructionOp::SUBI},
        {"muli", InstructionOp::MULI},
        {"divi", InstructionOp::DIVI},
        {"remi", InstructionOp::REMI},
        {"andi", InstructionOp::ANDI},
        {"ori", InstructionOp::ORI},
        {"xori", InstructionOp::XORI},
        {"slli", InstructionOp::SLLI},
        {"srli", InstructionOp::SRLI},
        {"srai", InstructionOp::SRAI},
        {"slti", InstructionOp::SLTI},
        {"seqi", InstructionOp::SEQI},
        {"lui", InstructionOp::LUI},
        {"fadd", InstructionOp::FADD},
        {"fsub", InstructionOp::FSUB},
        {"fmul", InstructionOp::FMUL},
        {"fdiv", InstructionOp::FDIV},
        {"fabs", InstructionOp::FABS},
        {"frcp", InstructionOp::FRCP},
        {"fsqrt", InstructionOp::FSQRT},
        {"frsqrt", InstructionOp::FRSQRT},
        {"fsin", InstructionOp::FSIN},
        {"fcos", InstructionOp::FCOS},
        {"flog", InstructionOp::FLOG},
        {"fexp", InstructionOp::FEXP},
        {"fslt", InstructionOp::FSLT},
        {"fseq", InstructionOp::FSEQ},
        {"cvtif", InstructionOp::CVTIF},
        {"cvtfi", InstructionOp::CVTFI},
        {"cvtfr", InstructionOp::CVTFR},
        {"cvtfc", InstructionOp::CVTFC},
        {"lw", InstructionOp::LW},
        {"sw", InstructionOp::SW},
        {"spix", InstructionOp::SPIX},
        {"jump", InstructionOp::JUMP},
        {"branch", InstructionOp::BRANCH},
        {"call", InstructionOp::CALL},
        {"ret", InstructionOp::RET},
        {"disp", InstructionOp::DISP},
        {"sync", InstructionOp::SYNC},
        {"exit", InstructionOp::EXIT},
        {"nop", InstructionOp::NOP},
        {"li", InstructionOp::LI}
    };

    const std::vector<char> non_alnum_chars_ {
        ' ', '\t', ',', '(', ')', '%', '<', '>', '.'
    };
};

#endif
