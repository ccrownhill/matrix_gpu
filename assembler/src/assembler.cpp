#include <cassert>
#include <iostream>
#include <cstdint>
#include <ctype.h>
#include <fstream>
#include <iomanip>
#include <string>
#include <vector>
#include <algorithm>

#include "instruction.hpp"
#include "parameters.hpp"
#include "util.hpp"
#include "assembler.hpp"

// if this is broken remove the format parameter
void Assembler::assemble(std::istream &asm_file, std::ostream &bin_file, std::string format)

{
    if (format == "bin") {
        writebin(asm_file, bin_file);
    } else if(format == "hex") {
        writehex(asm_file, bin_file);
    } else {
        writebin(asm_file, bin_file);
    }
}

void Assembler::writebin(std::istream &asm_file, std::ostream &bin_file){
    std::string line;
    while (std::getline(asm_file, line)) {
        std::vector<std::string> tokens = tokenize(line);
        // Some lines may be empty
        if (!tokens.empty()) {

            std::vector<uint32_t> instr_bin = encode(tokens);
            // Requires unsigned long to be 32 bits
            for(uint32_t instr_bin1 : instr_bin){
                // Requires unsigned long to be 32 bits
                bin_file.write((char *)&instr_bin1, sizeof(uint32_t));
            }

        }
    }
}

void Assembler::writehex(std::istream &asm_file, std::ostream &hex_file){
    std::string line;
    while (std::getline(asm_file, line)) {
        std::vector<std::string> tokens = tokenize(line);
        // Some lines may be empty
        if (!tokens.empty()) {

            std::vector<uint32_t> instr_hex = encode(tokens);
            // Requires unsigned long to be 32 bits
            for(uint32_t instr_hex1 : instr_hex){
                hex_file << std::setw(8) << std::setfill('0')
                        << std::hex << instr_hex1 << "\n";
            }

        }
    }
}

std::vector<std::string> Assembler::tokenize(const std::string &line)
{
    std::vector<std::string> tokens;
    std::string token = "";
    // Keep track of whether token is empty to avoid adding empty
    // tokens to list
    bool empty_token = true;

    for (char character : line) {
        // Ignore comments
        if (character == '#') {
            if (!empty_token) {
                tokens.push_back(token);
            }
            return tokens;
        }

        // Each character must be valid
        if (!valid_char(character)) {
            std::cerr << "Error: invalid character " << character << std::endl;
            std::exit(1);
        }
        else if (isalnum(character) || character == '%' || character == '.') {
            token.push_back(tolower(character));
            empty_token = false;
        } else if (character == '>' || character == '<') {
            token.push_back(tolower(character));
            tokens.push_back(token);
            token.clear();
            empty_token = true;
        } else if (!empty_token) {
            tokens.push_back(token);
            token.clear();
            empty_token = true;
        }
        // Do nothing if non-alphanumeric character and empty token
    }

    // Add final token
    if (!empty_token) {
        tokens.push_back(token);
    }

    return tokens;
}


std::vector<uint32_t> Assembler::encode(std::vector<std::string> &tokens)
{
    std::vector<uint32_t> instr_bin_vec;
    if (tokens[0] == "<") {
        instr_bin_vec.push_back(std::stoi(tokens[1]) | 0xa0000000);
        return instr_bin_vec;

    }

    // Binary encoding of instruction
    uint32_t instr_bin = 0x0000'0000;

    // check if it ends in .p
    // if it is D-/C-type throw error since they must not be predicated
    // else set bit 28
    if (tokens[0].ends_with(".p")) {
        instr_bin |= 1 << 28;
        tokens[0].erase(tokens[0].size()-2, 2);
    }

    // Instruction must be valid
    if (!instr_map_.contains(tokens[0])) {
        std::cerr << "Error: unrecognized instruction '"
                  << tokens[0] << "'" << std::endl;
        std::exit(1);
    }

    // const unordered_map doesn't support [] operator
    InstructionOp instr_op = instr_map_.at(tokens[0]);
    if (instr_op == InstructionOp::NOP) {
        return {0};

    }
    InstructionType instr_type = get_instr_type(instr_op);
    if (((instr_bin >> 28) & 1) &&
            (instr_type == InstructionType::D || instr_type == InstructionType::C)) {
        std::cerr << "Error: predicated version unsupported for D-/C-type" << std::endl;
        std::exit(1);
    }

    switch (instr_type) {
    case InstructionType::R: {
        uint32_t opc = 0b000;
        uint32_t func5 = get_func5(instr_op);
        uint32_t rs1, rs2;
        if (is_predicate_setter(instr_op)) {
            rs1 = get_reg(tokens[1]);
            rs2 = get_reg(tokens[2]);
        } else {
            uint32_t rd = get_reg(tokens[1]);
            instr_bin |= rd;
            rs1 = get_reg(tokens[2]);
            // abs has a unique encoding without rs2
            rs2 = (instr_op == InstructionOp::ABS) ? 0b00000
                                                            : get_reg(tokens[3]);
        }
        instr_bin |= opc << 29;
        instr_bin |= func5 << 10;
        instr_bin |= rs1 << 5;
        instr_bin |= rs2 << 15;
        break;
    }
    case InstructionType::I: {
        uint32_t opc = 0b001;
        uint32_t func5 = get_func5(instr_op);
        uint32_t rs1, imm;
        if (is_predicate_setter(instr_op)) {
            rs1 = get_reg(tokens[1]);
            imm = get_imm(tokens[2]);
        } else {
            uint32_t rd = get_reg(tokens[1]);
            instr_bin |= rd;
            rs1 = get_reg(tokens[2]);
            imm = get_imm(tokens[3]) & 0x1fff; // get only bottom 13 bits
        }
        // lui has a unique encoding without rs1 - use these 5 bits for immediate

        instr_bin |= opc << 29;
        instr_bin |= func5 << 10;
        instr_bin |= rs1 << 5;
        instr_bin |= imm << 15;
        break;
    }
    case InstructionType::U: {
        uint32_t opc = 0b010;
        uint32_t func5 = 0b11111;
        uint32_t rd = get_reg(tokens[1]);
        uint32_t imm_upper = get_imm(tokens[2]) & 0x3ffe0; // top 13 bits
        uint32_t imm_lower = get_imm(tokens[2]) & 0x0001f; // bottom 5 bits

        instr_bin |= opc << 29;
        instr_bin |= func5 << 10;
        instr_bin |= rd;
        instr_bin |= imm_lower << 5;
        instr_bin |= imm_upper << 10;   // Already left-shifted by 5
        break;
    }
    case InstructionType::F: {
        uint32_t opc = 0b011;
        uint32_t func5 = get_func5(instr_op);

        // TODO refactor because this is same as InstructionType::R
        // except for the is_single_operant_instr check
        uint32_t rs1, rs2;
        if (is_predicate_setter(instr_op)) {
            rs1 = get_reg(tokens[1]);
            rs2 = get_reg(tokens[2]);
        } else {
            uint32_t rd = get_reg(tokens[1]);
            instr_bin |= rd;
            rs1 = get_reg(tokens[2]);
            // abs has a unique encoding without rs2
            rs2 = (is_single_operand_instr(instr_op)) ? 0b00000
                                                            : get_reg(tokens[3]);
        }

        instr_bin |= opc << 29;
        instr_bin |= func5 << 10;
        instr_bin |= rs1 << 5;
        instr_bin |= rs2 << 15;
        break;
    }
    case InstructionType::M: {
        uint32_t opc = 0b100;

        instr_bin |= opc << 29;

        if (instr_op == InstructionOp::LW) {
            uint32_t rd = get_reg(tokens[1]);
            uint32_t rs1 = get_reg(tokens[2]);
            instr_bin |= rd;
            instr_bin |= rs1 << 5;
            instr_bin |= 0b00 << 10; // "func1" is 0

        } else if (instr_op == InstructionOp::SPIX || instr_op == InstructionOp::SW) {
            uint32_t rs2 = get_reg(tokens[1]);
            uint32_t rs1 = get_reg(tokens[2]);
            instr_bin |= rs1 << 5;
            instr_bin |= rs2 << 15;
            instr_bin |=
                (0b1
                 + (static_cast<int>(instr_op == InstructionOp::SPIX) << 1))
                 << 10; // "func1 is 0b11 for spix and 0b01 for sw"
        } else {
            assertm(0, "Unrecognised memory instruction");
        }
        break;
    }
    case InstructionType::D: {
        uint32_t opc = 0b110;
        uint32_t rs1 = get_reg(tokens[1]);
        instr_bin |= opc << 29;
        instr_bin |= rs1 << 5;
        break;
    }
    case InstructionType::C: {
        uint32_t opc = 0b111;
        instr_bin |= opc << 29;
        uint32_t func3;

        // Nested switch for instruction op
        switch (instr_op) {
        case InstructionOp::JUMP: {
            func3 = 0b000;
            uint32_t imm = get_imm(tokens[1]);
            instr_bin |= imm & 0xffc;
            instr_bin |= (imm & 0xffff000) << 13;
            break;
        }
        case InstructionOp::BRANCH: {
            func3 = 0b001;
            uint32_t imm = get_imm(tokens[1]);
            instr_bin |= imm & 0xffc;
            instr_bin |= (imm & 0xffff000) << 13;
            break;
        }
        case InstructionOp::CALL: {
            func3 = 0b010;
            uint32_t rd = get_reg(tokens[1]);
            uint32_t rs1 = get_reg(tokens[3]);
            uint32_t imm = get_imm(tokens[2]);
            instr_bin |= rd;
            instr_bin |= rs1 << 5;
            instr_bin |= (imm & 0x3fffc) << 13;
            break;
        }
        case InstructionOp::RET: {
            func3 = 0b011;
            uint32_t rs1 = get_reg(tokens[1]);
            instr_bin |= rs1 << 5;
            break;
        }
        case InstructionOp::SYNC:
            func3 = 0b110;
            break;
        case InstructionOp::EXIT:
            func3 = 0b111;
            break;
        default:
            assertm(0, "Unrecognised control instruction");
        }

        instr_bin |= func3 << 10;
        break;
    }
    case InstructionType::P: {
        if (instr_op == InstructionOp::LI) { // pseudo instruction for LUI and then ADDI
            std::vector<std::string> fake_tokens = tokens;
            uint32_t imm = get_imm(tokens[2]);
            fake_tokens[0] = "lui";
            if (imm & (1 << 13)) {
                fake_tokens[2] = std::to_string((imm >> 14) + 1); // possible + 1 needed but is not in testing
            }else{
                fake_tokens[2] = std::to_string(imm >> 14);
            }
            instr_bin_vec.push_back(encode(fake_tokens)[0]);
            fake_tokens[0] = "addi";
            fake_tokens[2] = tokens[1];
            fake_tokens.push_back(std::to_string(imm & 0x3fff));
            instr_bin_vec.push_back(encode(fake_tokens)[0]);
            return instr_bin_vec;

        }
        break;
    }
    default:
        assertm(0, "Unrecognised instruction type");
    }
    instr_bin_vec.push_back(instr_bin);
    return instr_bin_vec;

}

uint32_t Assembler::get_func5(InstructionOp op)
{
    uint32_t func5;

    switch (op) {
    case InstructionOp::ADD:
    case InstructionOp::ADDI:
    case InstructionOp::FADD:
        func5 = 0b00000;
        break;
    case InstructionOp::SUB:
    case InstructionOp::SUBI:
    case InstructionOp::FSUB:
        func5 = 0b00001;
        break;
    case InstructionOp::MUL:
    case InstructionOp::MULI:
    case InstructionOp::FMUL:
        func5 = 0b00010;
        break;
    case InstructionOp::DIV:
    case InstructionOp::DIVI:
    case InstructionOp::FDIV:
        func5 = 0b00100;
        break;
    case InstructionOp::REM:
    case InstructionOp::REMI:
        func5 = 0b00101;
        break;
    case InstructionOp::ABS:
    case InstructionOp::FABS:
        func5 = 0b00110;
        break;
    case InstructionOp::AND:
    case InstructionOp::ANDI:
    case InstructionOp::FRCP:
        func5 = 0b00111;
        break;
    case InstructionOp::OR:
    case InstructionOp::ORI:
    case InstructionOp::FSQRT:
        func5 = 0b01000;
        break;
    case InstructionOp::XOR:
    case InstructionOp::XORI:
    case InstructionOp::FRSQRT:
        func5 = 0b01001;
        break;
    case InstructionOp::SLL:
    case InstructionOp::SLLI:
    case InstructionOp::FSIN:
        func5 = 0b01010;
        break;
    case InstructionOp::SRL:
    case InstructionOp::SRLI:
    case InstructionOp::FCOS:
        func5 = 0b01011;
        break;
    case InstructionOp::SRA:
    case InstructionOp::SRAI:
    case InstructionOp::FLOG:
        func5 = 0b01100;
        break;
    case InstructionOp::FEXP:
        func5 = 0b01101;
        break;
    case InstructionOp::SLT:
    case InstructionOp::SLTI:
    case InstructionOp::FSLT:
        func5 = 0b11100;
        break;
    case InstructionOp::SEQ:
    case InstructionOp::SEQI:
    case InstructionOp::FSEQ:
        func5 = 0b11101;
        break;
    case InstructionOp::CVTIF:
        func5 = 0b10000;
        break;
    case InstructionOp::CVTFI:
        func5 = 0b10001;
        break;
    case InstructionOp::CVTFR:
        func5 = 0b10010;
        break;
    case InstructionOp::CVTFC:
        func5 = 0b10011;
        break;
    case InstructionOp::LUI:
        func5 = 0b11111;
        break;
    default:
        assertm(0, "Unrecognised instruction op");
    }

    return func5;
}

uint32_t Assembler::get_imm(const std::string &imm_token)
{
    return static_cast<uint32_t>(stoul(imm_token, nullptr, 0));
}

uint32_t Assembler::get_reg(std::string token) {
    // Special registers
    if (token == "zero") {
        return ZERO_REG;
    } else if (token == "%blockidx") {
        return BLOCK_IDX_REG;
    } else if (token == "%blockdim") {
        return BLOCK_DIM_REG;
    } else if (token == "%threadidx") {
        return THREAD_IDX_REG;
    }

    // Otherwise must be standard register syntax
    if (token[0] != 'r') {
        std::cerr << "Error: register name not starting with r: '" << token << "'" << std::endl;
        std::exit(1);
    }
    token.erase(0, 1);
    uint32_t reg_num = static_cast<uint32_t>(stoul(token));
    assert(reg_num < REG_FILE_SIZE);
    return reg_num;
}

bool Assembler::valid_char(char character)
{
    if (isalnum(character)) {
        return true;
    }

    return (std::find(non_alnum_chars_.begin(), non_alnum_chars_.end(), character)
            != non_alnum_chars_.end());
}
