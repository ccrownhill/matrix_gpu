#ifndef LEX_HPP
#define LEX_HPP

#include <iostream>
#include <utility>
#include <variant>
#include <string>
#include <functional>

#include "codegen.hpp"

namespace lex {
using LexType = std::variant<
    int,
    double,
    std::string,
    CodeGen::UnaryOp,
    CodeGen::BinaryOp
>;

enum class Token {
    VARNAME,
    X,
    Y,
    PLOT,
    PLOTXY,
    PLOTXY_SIMPLE,
    PLOTX,
    PLUS,
    MINUS,
    MULT,
    DIV,
    DOT,
    POW,
    TRANSPOSE,
    EXP,
    SIN,
    COS,
    SQRT,
    RELU,
    LROUND_BRACK,
    RROUND_BRACK,
    LSQUARE_BRACK,
    RSQUARE_BRACK,
    REAL,
    INT,
    COMMA,
    EQUAL,
    VERT_LINE,
    ILLEGAL,
    _EOF_,
};

std::tuple<Token, int, LexType> Lex(std::istream &in_stream);

} // namespace lex

#endif
