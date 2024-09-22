#include <variant>

#include <vector>
#include <tuple>
#include <istream>

#include "lex.hpp"
#include "parser.hpp"

namespace parse {

static void parsingError(int lineNo, std::string msg)
{
    std::cerr << "Parsing error on line " << lineNo << ": " << msg << std::endl;
    std::exit(1);
}

std::shared_ptr<ASTNode> ParseFac(std::istream &inStream)
{
    auto [opType, lineNo, val] = lex::Lex(inStream); // int
    if (opType == lex::Token::X || opType == lex::Token::Y
            || opType == lex::Token::VARNAME) {
        if (opType == lex::Token::VARNAME &&
                (std::get<std::string>(val) == "x"
                || std::get<std::string>(val) == "y")) {
            parsingError(lineNo, "x and y are reserved names and can't be used for "
                    "variable names");
        }
        std::string name = std::get<std::string>(val);
        return std::make_shared<VarNode>(std::get<std::string>(val));
    } else if (opType == lex::Token::REAL) { // real
        return std::make_shared<RealConstNode>(std::get<double>(val));
    } else if (opType == lex::Token::INT) { // int
        return std::make_shared<RealConstNode>(static_cast<double>(std::get<int>(val)));
    } else if (opType == lex::Token::LROUND_BRACK) { // ( expr )
        std::shared_ptr<ASTNode> expr = ParseExpr(inStream);
        auto [t, ln, v] = lex::Lex(inStream);
        if (t != lex::Token::RROUND_BRACK) {
            parsingError(ln, "expected ')'");
        }
        return expr;
    } else if (opType == lex::Token::MINUS) {
        std::shared_ptr<ASTNode> expr = ParseTerm(inStream);
        return std::make_shared<UnaryExprNode>(CodeGen::UnaryOp::MINUS, expr);
    } else if (opType == lex::Token::VERT_LINE) { // |shape_arr|[val_arr]
        std::vector<int> shape;

        lex::Token t;
        int ln;
        lex::LexType v;
        for (std::tie(t, ln, v) = lex::Lex(inStream);
                t != lex::Token::VERT_LINE;
                std::tie(t, ln, v) = lex::Lex(inStream)) {
            if (t != lex::Token::INT) {
                parsingError(ln, "expected integer for shape list");
            }
            shape.push_back(std::get<int>(v));
            std::tie(t, ln, v) = lex::Lex(inStream);
            if (t == lex::Token::VERT_LINE) {
                std::tie(t, ln, v) = lex::Lex(inStream);
                break;
            } else if (t != lex::Token::COMMA) {
                parsingError(ln, "expected comma to separate shape list values");
            }
        }

        std::vector<double> vals;
        if (t != lex::Token::LSQUARE_BRACK) {
            parsingError(ln, "expected [ to initialise array literal got '"
                    + std::get<std::string>(v) + "'");
        }
        for (std::tie(t, ln, v) = lex::Lex(inStream);
                t != lex::Token::RSQUARE_BRACK;
                std::tie(t, ln, v) = lex::Lex(inStream)) {
            bool neg = false;
            if (t == lex::Token::MINUS) {
                neg = true;
                std::tie(t, ln, v) = lex::Lex(inStream);
            }
            if (t != lex::Token::REAL) {
                parsingError(ln, "expected double for array literal value");
            }
            if (neg) {
                vals.push_back(-std::get<double>(v));
            } else {
                vals.push_back(std::get<double>(v));
            }
            std::tie(t, ln, v) = lex::Lex(inStream);
            if (t == lex::Token::RSQUARE_BRACK) {
                break;
            } else if (t != lex::Token::COMMA) {
                parsingError(ln, "expected comma to separate array literal values");
            }
        }
        return std::make_shared<ArrayLiteralNode>(std::move(shape), std::move(vals));
    // TODO make this an else if with a function that checks if something is
    // a unary function
    } else { // fn ( expr )
        auto [t1, ln1, v1] = lex::Lex(inStream);
        if (t1 != lex::Token::LROUND_BRACK) {
            parsingError(ln1, "expected '(' for unary function");
        }
        std::shared_ptr<ASTNode> expr = ParseExpr(inStream);
        auto [t2, ln2, v2] = lex::Lex(inStream);
        if (t2 != lex::Token::RROUND_BRACK) {
            parsingError(ln2, "expected ')' for unary function but got '" + std::get<std::string>(v2) + "'");
        }
        return std::make_shared<UnaryExprNode>(std::get<CodeGen::UnaryOp>(val), expr);
    }
}

std::shared_ptr<ASTNode> ParsePowRHS(std::istream &inStream,
        std::shared_ptr<ASTNode> lhsOp)
{
    int oldPos = inStream.tellg();
    auto [opType, lineNo, val] = lex::Lex(inStream);
    if (opType == lex::Token::POW) {
        auto [t, ln, v] = lex::Lex(inStream);
        if (t != lex::Token::INT) {
            parsingError(ln, "expected integer power");
        }
        int pow = std::get<int>(v);
        std::shared_ptr<ASTNode> topExpr = lhsOp;
        for (int i = 1; i < pow; i++) {
            topExpr = std::make_shared<BinaryExprNode>(CodeGen::BinaryOp::MULT,
                    topExpr, lhsOp);
        }
        return topExpr;
    } else if (opType == lex::Token::TRANSPOSE) {
        return std::make_shared<UnaryExprNode>(CodeGen::UnaryOp::TRANSPOSE, lhsOp);
    } else {
        if (inStream.eof()) {
            inStream.clear();
        }
        inStream.seekg(oldPos);
        return lhsOp;
    }
}


std::shared_ptr<ASTNode> ParsePow(std::istream &inStream)
{
    std::shared_ptr<ASTNode> op1 = ParseFac(inStream);
    return ParsePowRHS(inStream, op1);
}


std::shared_ptr<ASTNode> ParseTermRHS(std::istream &inStream,
        std::shared_ptr<ASTNode> lhsOp)
{
    int oldPos = inStream.tellg();
    auto [opType, lineNo, val] = lex::Lex(inStream);
    if (opType == lex::Token::MULT || opType == lex::Token::DIV
        || opType == lex::Token::DOT) {
        std::shared_ptr<ASTNode> rhsOp = ParsePow(inStream); 
        std::shared_ptr<ASTNode> binExpr =
            std::make_shared<BinaryExprNode>(std::get<CodeGen::BinaryOp>(val),
                lhsOp, rhsOp);
        return ParseTermRHS(inStream, binExpr);
    } else {
        if (inStream.eof()) {
            inStream.clear();
        }
        inStream.seekg(oldPos);
        return lhsOp;
    }
}


std::shared_ptr<ASTNode> ParseTerm(std::istream &inStream)
{
    std::shared_ptr<ASTNode> op1 = ParsePow(inStream);
    return ParseTermRHS(inStream, op1);
}

std::shared_ptr<ASTNode> ParseExprRHS(std::istream &inStream,
        std::shared_ptr<ASTNode> lhsOp)
{
    int oldPos = inStream.tellg();
    auto [opType, lineNo, val] = lex::Lex(inStream);
    if (opType == lex::Token::PLUS || opType == lex::Token::MINUS) {
        std::shared_ptr<ASTNode> rhsOp = ParseTerm(inStream); 
        std::shared_ptr<ASTNode> binExpr =
            std::make_shared<BinaryExprNode>(std::get<CodeGen::BinaryOp>(val),
                lhsOp, rhsOp);
        return ParseExprRHS(inStream, binExpr);
    } else {
        if (inStream.eof()) {
            inStream.clear();
        }
        inStream.seekg(oldPos);
        return lhsOp;
    }
}

std::shared_ptr<ASTNode> ParseExpr(std::istream &inStream)
{
    std::shared_ptr<ASTNode> op1 = ParseTerm(inStream);
    return ParseExprRHS(inStream, op1);
}

std::shared_ptr<ASTNode> ParseStatement(std::istream &inStream)
{
    auto [opType, lineNo, val] = lex::Lex(inStream);
    if (opType == lex::Token::PLOT) {
        auto [t, ln, v] = lex::Lex(inStream);
        if (t != lex::Token::VARNAME) {
            parsingError(ln, "expected variable name for .plot first argument");
        }
        std::string varName = std::get<std::string>(v);

        std::tie(t, ln, v) = lex::Lex(inStream);
        if (t != lex::Token::REAL) {
            parsingError(ln, "expected real for minimum of .plot as second argument");
        }
        double min = std::get<double>(v);

        std::tie(t, ln, v) = lex::Lex(inStream);
        if (t != lex::Token::REAL) {
            parsingError(ln, "expected real for maximum of .plot as third argument");
        }
        double max = std::get<double>(v);

        return std::make_shared<PlotStatement>(varName, min, max);
    } else if (opType == lex::Token::PLOTXY) {
        // need 3 angles
        std::vector<double> angles {};
        for (int i = 0; i < 3; i++) {
            auto [t, ln, v] = lex::Lex(inStream);
            bool neg = false;
            if (t == lex::Token::MINUS) {
                neg = true;
                std::tie(t, ln, v) = lex::Lex(inStream);
            }
            if (t != lex::Token::REAL && t != lex::Token::INT) {
                parsingError(lineNo, "expected 'plotxy angleX angleY angleZ xyExpr'");
            }
            if (t == lex::Token::INT) {
                angles.push_back(static_cast<double>(std::get<int>(v)));
            } else {
                angles.push_back(std::get<double>(v));
            }
            if (neg) {
                angles[angles.size()-1] *= -1;
            }
        }
        std::shared_ptr<ASTNode> xyExpr = ParseExpr(inStream);
        return std::make_shared<PlotXYStatement>(angles[0], angles[1], angles[2],
                xyExpr);
    } else if (opType == lex::Token::PLOTXY_SIMPLE) {
        double min, max;
        bool neg = false;
        auto [t, ln, v] = lex::Lex(inStream);
        if (t == lex::Token::MINUS) {
            neg = true;
            std::tie(t, ln, v) = lex::Lex(inStream);
        }
        if (t != lex::Token::REAL && t != lex::Token::INT) {
            parsingError(lineNo, "expected 'plotxy angleX angleY angleZ xyExpr'");
        }
        if (t == lex::Token::INT) {
            min = static_cast<double>(std::get<int>(v));
        } else {
            min = std::get<double>(v);
        }
        if (neg) {
            min *= -1;
        }

        std::tie(t, ln, v) = lex::Lex(inStream);
        neg = false;
        if (t == lex::Token::MINUS) {
            neg = true;
            std::tie(t, ln, v) = lex::Lex(inStream);
        }
        if (t != lex::Token::REAL && t != lex::Token::INT) {
            parsingError(lineNo, "expected '.simple_plotxy min max xyExpr'");
        }
        if (t == lex::Token::INT) {
            max = static_cast<double>(std::get<int>(v));
        } else {
            max = std::get<double>(v);
        }
        if (neg) {
            max *= -1;
        }

        std::shared_ptr<ASTNode> xyExpr = ParseExpr(inStream);
        return std::make_shared<PlotXYSimpleStatement>(min, max, xyExpr);
    } else if (opType == lex::Token::PLOTX) {
        std::shared_ptr<ASTNode> xExpr = ParseExpr(inStream);
        return std::make_shared<PlotXStatement>(xExpr);
    } else if (opType == lex::Token::VARNAME) {
        if (std::get<std::string>(val) == "x" || std::get<std::string>(val) == "y") {
            parsingError(lineNo, "x and y are reserved names and can't be used for "
                    "variable names");
        }
        auto [t, ln, v] = lex::Lex(inStream);
        if (t != lex::Token::EQUAL) {
            parsingError(ln, "expected equality sign for assignment expression");
        }
        std::shared_ptr<ASTNode> rhs = ParseExpr(inStream);
        return std::make_shared<Assignment>(
                std::get<std::string>(val), rhs);

    } else if (opType == lex::Token::ILLEGAL) {
        std::cerr << "Parsing error on line " << lineNo
                  << ": expected plotxy call" << std::endl;
        std::exit(1);
    } else {
        // should never happen
        std::cerr << "Parsing: unexpected error, token: "
                  << std::get<std::string>(val) << std::endl;
        std::exit(1);
    }
}

std::shared_ptr<ASTNode> ParseStatementList(std::istream &inStream)
{
    std::shared_ptr<ASTNodeList> statementList =
        std::make_shared<ASTNodeList>(ParseStatement(inStream));
    while (inStream.peek() != EOF) {
        statementList->Append(ParseStatement(inStream));
    }
    return statementList;
}

std::shared_ptr<ASTNode> Parse(std::istream &inStream)
{
    return ParseStatementList(inStream);
}

} // namespace parse
