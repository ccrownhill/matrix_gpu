#ifndef AST_VISITOR_HPP
#define AST_VISITOR_HPP

#include <iostream>
#include <memory>
#include <string>
#include <vector>

#include "codegen.hpp"

class ASTNode;

class ASTVisitor
{
public:
    virtual void VisitAssignment(std::string varName,
            std::shared_ptr<ASTNode> rhs) = 0;

    virtual void VisitPlot(std::string varName, double min, double max) = 0;

    virtual void VisitPlotXY(double angleX, double angleY, double angleZ,
            std::shared_ptr<ASTNode> xyExpr) = 0;

    virtual void VisitPlotXYSimple(double min, double max, std::shared_ptr<ASTNode> xyExpr) = 0;

    virtual void VisitPlotX(std::shared_ptr<ASTNode> xExpr) = 0;

    virtual void VisitBinExpr(
            CodeGen::BinaryOp opType,
            std::shared_ptr<ASTNode> op1,
            std::shared_ptr<ASTNode> op2
        ) = 0;

    virtual void VisitUnaryExpr(CodeGen::UnaryOp opType,
            std::shared_ptr<ASTNode> op) = 0;
    virtual void VisitVar(std::string var) = 0;

    virtual void VisitConst(double val) = 0;

    virtual void VisitArrayLiteral(std::vector<int> shape, std::vector<double> elements) = 0;
    virtual ~ASTVisitor() {}
};

class PrintVisitor : public ASTVisitor
{
public:
    PrintVisitor(std::ostream &outStream = std::cout)
        : stream_ {outStream}
    {}

    void VisitAssignment(std::string varName,
        std::shared_ptr<ASTNode> rhs) override;

    void VisitPlot(std::string varName, double min, double max) override;

    void VisitPlotXY(double angleX, double angleY, double angleZ,
           std::shared_ptr<ASTNode> xyExpr) override;

    void VisitPlotXYSimple(double min, double max, std::shared_ptr<ASTNode> xyExpr) override;

    void VisitPlotX(std::shared_ptr<ASTNode> xExpr) override;
    

    void VisitBinExpr(CodeGen::BinaryOp opType, std::shared_ptr<ASTNode> op1,
            std::shared_ptr<ASTNode> op2) override;

    void VisitUnaryExpr(CodeGen::UnaryOp opType, std::shared_ptr<ASTNode> op) override;

    void VisitVar(std::string var) override;

    void VisitConst(double val) override;

    void VisitArrayLiteral(std::vector<int> shape, std::vector<double> elements) override;
private:
    std::ostream &stream_;
};

class ASMGenVisitor : public ASTVisitor
{
public:
    ASMGenVisitor(std::shared_ptr<CodeGen> ctx, std::ostream &outStream = std::cout)
        : ctx_ {ctx}, stream_ {outStream}
    {}

    void VisitAssignment(std::string varName,
        std::shared_ptr<ASTNode> rhs) override;

    void VisitPlot(std::string varName, double min, double max) override;

    void VisitPlotXY(double angleX, double angleY, double angleZ,
           std::shared_ptr<ASTNode> xyExpr) override;

    void VisitPlotXYSimple(double min, double max, std::shared_ptr<ASTNode> xyExpr) override;

    void VisitPlotX(std::shared_ptr<ASTNode> xExpr) override;

    void VisitBinExpr(CodeGen::BinaryOp opType, std::shared_ptr<ASTNode> op1,
            std::shared_ptr<ASTNode> op2) override;

    void VisitUnaryExpr(CodeGen::UnaryOp opType, std::shared_ptr<ASTNode> op) override;

    void VisitVar(std::string var) override;

    void VisitConst(double val) override;

    void VisitArrayLiteral(std::vector<int> shape, std::vector<double> elements) override;
private:
    std::shared_ptr<CodeGen> ctx_;
    std::ostream &stream_;
};

#endif
