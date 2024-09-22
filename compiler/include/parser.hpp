#ifndef PARSER_HPP
#define PARSER_HPP

#include <istream>
#include <memory>

#include "ast.hpp"

namespace parse {



std::shared_ptr<ASTNode> ParseFac(std::istream &inStream);

std::shared_ptr<ASTNode> ParsePowRHS(std::istream &inStream,
                std::shared_ptr<ASTNode> lhs_op);

std::shared_ptr<ASTNode> ParsePow(std::istream &inStream);

std::shared_ptr<ASTNode> ParseTermRHS(std::istream &inStream,
                std::shared_ptr<ASTNode> lhs_op);

std::shared_ptr<ASTNode> ParseTerm(std::istream &inStream);

std::shared_ptr<ASTNode> ParseExprRHS(std::istream &inStream,
                std::shared_ptr<ASTNode> lhs_op);

std::shared_ptr<ASTNode> ParseExpr(std::istream &inStream);

std::shared_ptr<ASTNode> ParseStatement(std::istream &inStream);

std::shared_ptr<ASTNode> ParseStatementList(std::istream &inStream);

std::shared_ptr<ASTNode> Parse(std::istream &inStream);

} // namespace parse


#endif
