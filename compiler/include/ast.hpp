#ifndef AST_HPP
#define AST_HPP

#include <memory>
#include <string>
#include <cmath>
#include <cstdint>

#include "codegen.hpp"
#include "constants.hpp"
#include "ast_visitor.hpp"


class ASTNode
{
public:
    virtual void Accept(ASTVisitor *visitor) const = 0;
    virtual ~ASTNode() {}
};

class ASTNodeList : public ASTNode
{
public:
    ASTNodeList(std::shared_ptr<ASTNode> first) : nodeList_ {{first}} {}

    void Accept(ASTVisitor *visitor) const override;

    void Append(std::shared_ptr<ASTNode> node);
private:
    std::list<std::shared_ptr<ASTNode>> nodeList_;
};

class Assignment : public ASTNode
{
public:
    Assignment(std::string varName, std::shared_ptr<ASTNode> rhs)
        : varName_ {varName}, rhs_ {rhs}
    {}

    void Accept(ASTVisitor *visitor) const override;
private:
    std::string varName_;
    std::shared_ptr<ASTNode> rhs_;
};

class PlotStatement : public ASTNode
{
public:
    PlotStatement(std::string varName, double min, double max)
        : varName_ {varName}, min_ {min}, max_ {max}
    {}

    void Accept(ASTVisitor *visitor) const override;
private:
    std::string varName_;
    double min_, max_;
};

class PlotXYStatement : public ASTNode
{
public:
    PlotXYStatement(double angleX, double angleY, double angleZ,
            std::shared_ptr<ASTNode> xyExpr)
        : angleX_ {angleX}, angleY_ {angleY}, angleZ_ {angleZ}, xyExpr_ {xyExpr}
    {}

    void Accept(ASTVisitor *visitor) const override;
private:
    double angleX_, angleY_, angleZ_;
    std::shared_ptr<ASTNode> xyExpr_;
};

class PlotXYSimpleStatement : public ASTNode
{
public:
    PlotXYSimpleStatement(double min, double max, std::shared_ptr<ASTNode> xyExpr)
        : min_ {min}, max_ {max}, xyExpr_ {xyExpr}
    {}

    void Accept(ASTVisitor *visitor) const override;
private:
    double min_, max_;
    std::shared_ptr<ASTNode> xyExpr_;
};


class PlotXStatement : public ASTNode
{
public:
    PlotXStatement(std::shared_ptr<ASTNode> xExpr)
        : xExpr_ {xExpr}
    {}

    void Accept(ASTVisitor *visitor) const override;
private:
    std::shared_ptr<ASTNode> xExpr_;
};

class BinaryExprNode : public ASTNode
{
public:
    BinaryExprNode(CodeGen::BinaryOp opType,
            std::shared_ptr<ASTNode> op1,
            std::shared_ptr<ASTNode> op2)
        : opType_ {opType}, op1_ {op1}, op2_ {op2}
    {}

    void Accept(ASTVisitor *visitor) const override;
private:
    CodeGen::BinaryOp opType_;
    
    std::shared_ptr<ASTNode> op1_;
    std::shared_ptr<ASTNode> op2_;
};

class UnaryExprNode : public ASTNode
{
public:
    UnaryExprNode(CodeGen::UnaryOp opType, std::shared_ptr<ASTNode> op)
        : opType_ {opType}, op_ {op}
    {}
    void Accept(ASTVisitor *visitor) const override;
private:
    CodeGen::UnaryOp opType_; 
    std::shared_ptr<ASTNode> op_;
};

class VarNode : public ASTNode
{
public:
    VarNode(std::string var) : var_ {var} {}
    void Accept(ASTVisitor *visitor) const override;
private:
    std::string var_;
};

class RealConstNode : public ASTNode
{
public:
    RealConstNode(double val) : val_ {val} {}
    void Accept(ASTVisitor *visitor) const override;
private:
    double val_;
};

class ArrayLiteralNode : public ASTNode
{
public:
    ArrayLiteralNode(std::vector<int> shape, std::vector<double> elements)
        : shape_ {std::move(shape)}, elements_ {std::move(elements)}
    {}

    void Accept(ASTVisitor *visitor) const override;
private:
    std::vector<int> shape_;
    std::vector<double> elements_;
};

#endif
