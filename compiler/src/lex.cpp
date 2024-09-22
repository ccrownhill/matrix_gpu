#include <unordered_map>
#include <regex>
#include <tuple>
#include <cctype>
#include <functional>

#include "lex.hpp"

namespace lex {
std::tuple<Token, int, LexType> Lex(std::istream &inStream)
{
    static int lineNo = 1;
    std::unordered_map<std::string, std::pair<Token, std::function<LexType(std::string)>>>
        tok_map =
    {
        // variable name must start with a character but can't be single letter
        // x or y
        {"\\$[a-zA-Z][a-zA-Z0-9_]*",
            std::make_pair(Token::VARNAME, [](std::string t){ return t.substr(1,t.size()-1); })},
        {"x", {Token::X, [](std::string t){ return t; }}},
        {"y", {Token::Y, [](std::string t){ return t; }}},
        {"\\.plot",
            {Token::PLOT, [](std::string t){ return t; }}},
        {"\\.plotxy",
            {Token::PLOTXY, [](std::string t){ return t; }}},
        {"\\.simple_plotxy",
            {Token::PLOTXY_SIMPLE, [](std::string t){ return t; }}},
        {"\\.plotx",
            {Token::PLOTX, [](std::string t){ return t; }}},
        {"\\+",
            {Token::PLUS, [](std::string t){ return CodeGen::BinaryOp::PLUS; }}},
        {"-",
            {Token::MINUS, [](std::string t){ return CodeGen::BinaryOp::MINUS; }}},
        {"\\*",
            {Token::MULT, [](std::string t){ return CodeGen::BinaryOp::MULT; }}},
        {"/",
            {Token::DIV, [](std::string t){ return CodeGen::BinaryOp::DIV; }}},
        {"dot",
            {Token::DOT, [](std::string t){ return CodeGen::BinaryOp::DOT; }}},
        {"\\^",
            {Token::POW, [](std::string t){ return t; }}},
        {"\\.T",
            {Token::TRANSPOSE, [](std::string t){ return t; }}},
        {"exp",
            {Token::EXP, [](std::string t){ return CodeGen::UnaryOp::EXP; }}},
        {"sin",
            {Token::SIN, [](std::string t){ return CodeGen::UnaryOp::SIN; }}},
        {"cos",
            {Token::COS, [](std::string t){ return CodeGen::UnaryOp::COS; }}},
        {"sqrt",
            {Token::SQRT, [](std::string t){ return CodeGen::UnaryOp::SQRT; }}},
        {"relu",
            {Token::RELU, [](std::string t){ return CodeGen::UnaryOp::RELU; }}},
        {"\\(", 
            {Token::LROUND_BRACK, [](std::string t){ return t; }}},
        {"\\)",
            {Token::RROUND_BRACK, [](std::string t){ return t; }}},
        {"\\[",
            {Token::LSQUARE_BRACK, [](std::string t){ return t; }}},
        {"\\]",
            {Token::RSQUARE_BRACK, [](std::string t){ return t; }}},
        {"[0-9]+\\.([0-9]*)?",
            {Token::REAL, [](std::string t){ return std::stod(t); }}},
        {"[0-9]+",
            {Token::INT, [](std::string t){ return std::stoi(t); }}},
        {",",
            {Token::COMMA, [](std::string t){ return t; }}},
        {"=",
            {Token::EQUAL, [](std::string t){ return t; }}},
        {"\\|",
            {Token::VERT_LINE, [](std::string t){ return t; }}},
    };

    if (inStream.peek() == EOF) {
        return {Token::_EOF_, lineNo, ""};
    }

    // ignore any whitespace
    while(std::isspace(inStream.peek())) {
        if (inStream.get() == '\n') {
            lineNo++;
        }
    }


    std::string token = "";
    std::tuple<Token, int, LexType> res = {Token::ILLEGAL, lineNo, ""};
    for (char out = inStream.get(); out != EOF; out = inStream.get()) {
        token += out;
        bool matched = false;
        for (auto &[key, val] : tok_map) {
            std::regex rx {key};
            if (std::regex_match(token, rx)) {
                matched = true;
                res = {val.first, lineNo, val.second(token)};
            }
        }

        if (!matched && std::get<0>(res) != Token::ILLEGAL) {
            inStream.unget();
            return res;
        }
    }
    return res;
}
} // namespace lex
