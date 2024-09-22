#include <iostream>
#include <memory>
#include <string>
#include <cmath>

#include "lex.hpp"
#include "context.hpp"
#include "ast.hpp"
#include "codegen.hpp"

void ASTNodeList::Accept(ASTVisitor *visitor) const
{
    for (auto node : nodeList_) {
        node->Accept(visitor);
    }
}

void ASTNodeList::Append(std::shared_ptr<ASTNode> node)
{
    nodeList_.push_back(node);
}

void Assignment::Accept(ASTVisitor *visitor) const
{
    visitor->VisitAssignment(varName_, rhs_);
}

void PlotStatement::Accept(ASTVisitor *visitor) const
{
    visitor->VisitPlot(varName_, min_, max_);
}

void PlotXYStatement::Accept(ASTVisitor *visitor) const
{
    visitor->VisitPlotXY(angleX_, angleY_, angleZ_, xyExpr_);
}

void PlotXYSimpleStatement::Accept(ASTVisitor *visitor) const
{
    visitor->VisitPlotXYSimple(min_, max_, xyExpr_);
}

void PlotXStatement::Accept(ASTVisitor *visitor) const
{
    visitor->VisitPlotX(xExpr_);
}

void BinaryExprNode::Accept(ASTVisitor *visitor) const
{
    visitor->VisitBinExpr(opType_, op1_, op2_);
}

void UnaryExprNode::Accept(ASTVisitor *visitor) const
{
    visitor->VisitUnaryExpr(opType_, op_);
}

void VarNode::Accept(ASTVisitor *visitor) const
{
    visitor->VisitVar(var_);
}

void RealConstNode::Accept(ASTVisitor *visitor) const
{
    visitor->VisitConst(val_);
}

void ArrayLiteralNode::Accept(ASTVisitor *visitor) const
{
    visitor->VisitArrayLiteral(shape_, elements_);
}
