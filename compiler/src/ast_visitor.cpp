#include <string>
#include <memory>
#include <numeric> // for accumulate
#include <algorithm> // for equal
#include <variant>

#include "ast.hpp"
#include "ast_visitor.hpp"
#include "codegen.hpp"

void PrintVisitor::VisitAssignment(std::string varName,
        std::shared_ptr<ASTNode> rhs)
{
    stream_ << "$" << varName << " = ";
    rhs->Accept(this);
    stream_ << std::endl;
}

void PrintVisitor::VisitPlot(std::string varName, double min, double max)
{
    stream_ << ".plot $" << varName << " " << min << " " << max << "\n";
}

void PrintVisitor::VisitPlotXY(double angleX, double angleY, double angleZ,
    std::shared_ptr<ASTNode> xyExpr)
{
    stream_ << ".plotxy " << angleX << " " << angleY << " " << angleZ << " ";
    xyExpr->Accept(this);
    stream_ << std::endl;
}

void PrintVisitor::VisitPlotXYSimple(double min, double max, std::shared_ptr<ASTNode> xyExpr)
{
    stream_ << ".plotxy_simple " << min << " " << max << " ";
    xyExpr->Accept(this);
    stream_ << std::endl;
}


void PrintVisitor::VisitPlotX(std::shared_ptr<ASTNode> xExpr)
{
    stream_ << ".plotx ";
    xExpr->Accept(this);
    stream_ << std::endl;
}

void PrintVisitor::VisitBinExpr
(
		CodeGen::BinaryOp opType,
		std::shared_ptr<ASTNode> op1,
		std::shared_ptr<ASTNode> op2
)
{
    stream_ << "(";
    op1->Accept(this);
    stream_ << " " << CodeGen::BinaryOpToStr(opType) << " ";
    op2->Accept(this);
    stream_ << ")";
}
    
void PrintVisitor::VisitUnaryExpr
(
    CodeGen::UnaryOp opType,
    std::shared_ptr<ASTNode> op
)
{
    stream_ << "(" << CodeGen::UnaryOpToStr(opType);
    stream_ << "(";
    op->Accept(this);
    stream_ << ")";
    stream_ << ")";
}

void PrintVisitor::VisitVar(std::string var)
{
    stream_ << var;
}

void PrintVisitor::VisitConst(double val)
{
    stream_ << val;
}


void PrintVisitor::VisitArrayLiteral(std::vector<int> shape,
        std::vector<double> elements)
{
    stream_ << "|";
    for (int s : shape) {
        stream_ << s << ",";
    }
    stream_ << "|[";
    for (double e : elements) {
        stream_ << e << ",";
    }
    stream_ << "]";
}

void ASMGenVisitor::VisitAssignment(std::string varName,
        std::shared_ptr<ASTNode> rhs)
{
    rhs->Accept(this);

    if (ctx_->varMemMap.find(varName) != ctx_->varMemMap.end()) {

        if (ctx_->exprOut.t == CodeGen::OutType::mem &&
                ctx_->varMemMap[varName].t == CodeGen::OutType::mem &&
                std::get<CodeGen::Arr>(ctx_->exprOut.v).addr !=
                std::get<CodeGen::Arr>(ctx_->varMemMap[varName].v).addr) {
            ctx_->FreeMem(std::get<CodeGen::Arr>(ctx_->varMemMap[varName].v).addr);
        }
    }
    ctx_->varMemMap[varName] = ctx_->exprOut;
}

void ASMGenVisitor::VisitPlot(std::string varName, double min, double max)
{
    if (ctx_->varMemMap[varName].t != CodeGen::OutType::mem) {
        std::cerr << ".plot works only for 2d arrays but not integers" << std::endl;
        std::exit(1);
    }

    CodeGen::Arr var = std::get<CodeGen::Arr>(ctx_->varMemMap[varName].v);
    if (var.shape.size() != 2) {
        std::cerr << ".plot works only for 2d arrays but got array of dim '"
                  << var.shape.size() << "'" << std::endl;
        std::exit(1);
    }

    std::cerr << ".plot shape: " + CodeGen::ShapeToStr(var.shape) << std::endl;
    auto [paddedDims, paddedSize] = CodeGen::PaddedArrSize(var.shape);
    // program for 1 row of the array
    for (int i = 0; i < var.shape[0]; i++) {
        CodeGen::ProgHeader(paddedDims[1]/BLOCK_DIM, stream_);
        int addrReg = ctx_->IndexIntoReg(stream_);
        int valReg = ctx_->AllocReg();
        ctx_->ASMImmOp("addi", addrReg, addrReg,
                var.addr + i * 2 * paddedDims[1], stream_);
        ctx_->LoadReg(valReg, addrReg, stream_);
        
        ctx_->ChangeRegScale(valReg, min, max, 0.0, 1.0, stream_);
        ctx_->ASMOp("cvtfc", valReg, valReg, stream_);
        stream_ << "disp r" << valReg << "\n";
        ctx_->FreeReg({addrReg, valReg});
        ctx_->Reset();
        stream_ << "exit\n";

        CodeGen::ProgHeader((SCREEN_WIDTH - paddedDims[1])/BLOCK_DIM, stream_);
        stream_ << "disp zero\n";
        stream_ << "exit\n";
    }
}

void ASMGenVisitor::VisitPlotXY(double angleX, double angleY, double angleZ,
	std::shared_ptr<ASTNode> xyExpr)
{
	// PROGRAM to fill with rotated values
	CodeGen::ProgHeader((PLOT_WIDTH * PLOT_HEIGHT)/BLOCK_DIM, stream_);

    // compute result of expression for all pixel values
	xyExpr->Accept(this);

    int oldZReg = ctx_->ToRegCast(ctx_->exprOut, stream_);
	
	int oldXReg = ctx_->XIntoReg(stream_);
	int oldYReg = ctx_->YIntoReg(stream_);


	int maxReg = ctx_->AllocReg();
	int minReg = ctx_->AllocReg();

	ctx_->DoubleIntoReg(maxReg, X_MAX, stream_);
	ctx_->DoubleIntoReg(minReg, X_MIN, stream_);
	ctx_->ASMOp("fslt", oldXReg, maxReg, stream_);
	ctx_->predMode = true; // execute rest only if predicate bit is set
	ctx_->ASMOp("fslt", minReg, oldXReg, stream_);

	ctx_->ASMOp("fslt", oldYReg, maxReg, stream_);
	ctx_->ASMOp("fslt", minReg, oldYReg, stream_);

	ctx_->ASMOp("fslt", oldZReg, maxReg, stream_);
	ctx_->ASMOp("fslt", minReg, oldZReg, stream_);

	ctx_->FreeReg({maxReg, minReg});
    auto [newXReg, newYReg, newZReg] = ctx_->RotateRegs(oldXReg, oldYReg, oldZReg,
            angleX, angleY, angleZ, stream_);

    ctx_->FreeReg({oldXReg, oldYReg, oldZReg});
// 	ctx_->SetAxes(oldXReg, oldYReg, oldZReg,
// 			angleX, angleY, angleZ, stream_);

	ctx_->ChangeRegScale(newZReg, Z_MIN, Z_MAX, 0.0, 1.0, stream_);
    int addrReg = ctx_->AllocReg();
	// this will also bring newXReg and newYReg in the range
	// of 0 to PLOT_WIDTH/PLOT_HEIGHT
	ctx_->FloatCoordsToAddrReg(addrReg, newXReg, newYReg, stream_);

	ctx_->StoreColour(newZReg, addrReg, stream_);

	ctx_->FreeReg({addrReg, newXReg, newYReg, newZReg});
	stream_ << "exit" << std::endl;
	ctx_->Reset();

    if (ctx_->singleOut) {
        return;
    }

	// PROGRAM for white margin at top
    ctx_->TopBottomWhiteMargin(stream_);

	// PROGRAM for white margin on side with function content in middle
    ctx_->DisplayMem(stream_);

	// PROGRAM for white margin at bottom
    ctx_->TopBottomWhiteMargin(stream_);

	//int addrReg = ctx_->AllocReg();
	// this will also bring newXReg and newYReg in the range
	// of 0 to PLOT_WIDTH/PLOT_HEIGHT
	//ctx_->FloatCoordsToAddrReg(addrReg, newXReg, newYReg, stream_);

	//ctx_->StoreColour(newZReg, addrReg, stream_);
	//ctx_->StoreColour(oldZReg, addrReg, stream_);

// 	ctx_->FreeReg({addrReg, newXReg, newYReg, newZReg});
}

void ASMGenVisitor::VisitPlotXYSimple(double min, double max, std::shared_ptr<ASTNode> xyExpr)
{
	// PROGRAM to fill with rotated values
	CodeGen::ProgHeader(SCREEN_HEIGHT * NUM_THREADS, stream_);
	for (int i = 0;
		i < ((SCREEN_WIDTH - 1024)/2) / (NUM_THREADS * BLOCK_DIM);
		i++) {
		stream_ << "disp zero\n";
	}

	int colReg = ctx_->AllocReg();
	int rowReg = ctx_->AllocReg();

	// col = (blockIdx % NUM_THREADS) * BLOCK_DIM + threadIdx + i * NUM_THREADS * BLOCK_DIM
	// note that NUM_THREADS has to be a power of 2
	ctx_->ASMImmOp("andi", colReg, "%blockIdx", NUM_THREADS-1, stream_);
	ctx_->ASMImmOp("slli", colReg, colReg,
		static_cast<int>(std::log2(static_cast<double>(BLOCK_DIM))),
		stream_);
	ctx_->ASMOp("add", colReg, colReg, "%threadIdx", stream_);

	// row = blockIdx / NUM_THREADS (integer division)
	// addr = row * PLOT_WIDTH + col
	ctx_->ASMImmOp("srli", rowReg, "%blockIdx",
		static_cast<int>(std::log2(static_cast<double>(NUM_THREADS))),
		stream_);
    ctx_->ASMImmOp("subi", rowReg, rowReg, 720, stream_);
    ctx_->ASMOp("sub", rowReg, 0, rowReg, stream_);
    ctx_->ASMOp("cvtif", rowReg, rowReg, stream_);
    ctx_->ChangeRegScale(rowReg, 0.0, 720.0, Y_MIN*720.0/1024.0, Y_MAX*720.0/1024.0, stream_);

    for (int i = 0; i < 1024 / (NUM_THREADS * BLOCK_DIM); i++) {
        int tmpReg = ctx_->AllocReg();
        ctx_->ASMImmOp("addi", tmpReg, colReg,
                i * NUM_THREADS * BLOCK_DIM, stream_);
        ctx_->ASMOp("cvtif", tmpReg, tmpReg, stream_);
        ctx_->ChangeRegScale(tmpReg, 0.0, 1024.0, X_MIN, X_MAX, stream_);
        ctx_->varMap["y"] = rowReg;
        ctx_->varMap["x"] = tmpReg;
        // compute result of expression for all pixel values
        xyExpr->Accept(this);
        int zReg = ctx_->ToRegCast(ctx_->exprOut, stream_);
        ctx_->ChangeRegScale(zReg, min, max, 0.0, 1.0, stream_);
        stream_ << "cvtfc r" << zReg << ", r" << zReg << std::endl;
        stream_ << "disp r" << zReg << std::endl;
        ctx_->varMap.erase("x");
        ctx_->FreeReg({zReg, tmpReg});
    }
    ctx_->FreeReg({rowReg, colReg});

	for (int i = 0;
		i < ((SCREEN_WIDTH - 1024)/2) / (NUM_THREADS * BLOCK_DIM);
		i++) {
		stream_ << "disp zero\n";
	}

    stream_ << "exit\n";
	ctx_->Reset();
}

void ASMGenVisitor::VisitPlotX(std::shared_ptr<ASTNode> xExpr)
{
    return;
}

// void ASMGenVisitor::VisitPlotX(std::shared_ptr<ASTNode> xExpr)
// {
// 	// PROGRAM to fill with rotated values
// 	CodeGen::ProgHeader((PLOT_WIDTH * PLOT_HEIGHT)/BLOCK_DIM, stream_);
// 
//     // compute result of expression for all pixel values
// 	xExpr->Accept(this);
// 
// 	int yValReg = ctx_->exprOutReg; // target output reg
// 	
// 
//     int addrReg = ctx_->IndexIntoReg(stream_);
//     int tmpReg = ctx_->AllocReg();	
//     int errMarginReg = ctx_->AllocReg();
// 
//     // y axis: x = 0
// 	int xReg = ctx_->XIntoReg(stream_);
// 	ctx_->DoubleIntoReg(errMarginReg, EQUALITY_ERROR_MARGIN, stream_);
// 	ctx_->ASMOp("fabs", tmpReg, xReg, stream_); 
// 	// use EQUALITY_ERROR_MARGIN because it is not multiplied by anything
// 	ctx_->ASMOp("fslt", tmpReg, errMarginReg, stream_); 
// 	ctx_->predMode = true;
// 
//     ctx_->ASMImmOp("addi", tmpReg, "zero", 0, stream_);
// 	ctx_->ASMOp("spix", tmpReg, addrReg, stream_);
// 
//     ctx_->predMode = false;
//     ctx_->FreeReg(xReg);
// 
//     // x axis: y = 0
//     int yReg = ctx_->YIntoReg(stream_);
// 	ctx_->ASMOp("fabs", tmpReg, yReg, stream_); 
// 	// use EQUALITY_ERROR_MARGIN because it is not multiplied by anything
// 	ctx_->ASMOp("fslt", tmpReg, errMarginReg, stream_); 
// 	ctx_->predMode = true;
// 
//     ctx_->ASMImmOp("addi", tmpReg, "zero", 0, stream_);
// 	ctx_->ASMOp("spix", tmpReg, addrReg, stream_);
// 
//     ctx_->predMode = false;
// 
// 
//     // set output color if yReg == yValReg
//     ctx_->ASMOp("fsub", tmpReg, yReg, yValReg, stream_);
// 	ctx_->ASMOp("fabs", tmpReg, tmpReg, stream_); 
// 	// use EQUALITY_ERROR_MARGIN because it is not multiplied by anything
// 	ctx_->ASMOp("fslt", tmpReg, errMarginReg, stream_); 
// 	ctx_->predMode = true;
// 
//     // choose 200 as arbitrary colour index in middle
//     ctx_->ASMImmOp("addi", tmpReg, "zero", 200, stream_);
// 	ctx_->ASMOp("spix", tmpReg, addrReg, stream_);
// 
//     ctx_->predMode = false;
// 
//     
// 	ctx_->FreeReg({addrReg, tmpReg, yReg, yValReg});
// 	ctx_->Reset();
// 
// 
// 	// PROGRAM for white margin at top
//     ctx_->TopBottomWhiteMargin(stream_);
// 
// 	// PROGRAM for white margin on side with function content in middle
//     ctx_->DisplayMem(stream_);
// 
// 	// PROGRAM for white margin at bottom
//     ctx_->TopBottomWhiteMargin(stream_);
// }

void ASMGenVisitor::VisitBinExpr
(
	CodeGen::BinaryOp opType,
	std::shared_ptr<ASTNode> op1,
	std::shared_ptr<ASTNode> op2
)
{
    op1->Accept(this);
    CodeGen::ExprOut out1 = ctx_->exprOut;
    
    op2->Accept(this);
    CodeGen::ExprOut out2 = ctx_->exprOut;


    if (out1.t == CodeGen::OutType::mem || out2.t == CodeGen::OutType::mem) {
        CodeGen::Arr arr1 = ctx_->ToArrCast(out1, stream_);
        CodeGen::Arr arr2 = ctx_->ToArrCast(out2, stream_);
            
        if (opType == CodeGen::BinaryOp::DOT) {
            if (arr1.shape.size() != 2 || arr2.shape.size() != 2) {
                std::cerr << "Codegen error: dot product only supported for 2D arrays"
						  << std::endl;
                std::exit(1);
            }

            if (arr1.shape[1] != arr2.shape[0]) {
                std::cerr << "Codegen error: mismatched shapes for dot product: "
                          << arr1.shape[0] << "x" << arr1.shape[1] << " and "
                          << arr2.shape[0] << "x" << arr2.shape[1]
						  << std::endl;
                std::exit(1);
            }


            auto [dimSizes1, totalSize1] = CodeGen::PaddedArrSize(arr1.shape);
            auto [dimSizes2, totalSize2] = CodeGen::PaddedArrSize(arr2.shape);

            // shapes: (m x n) dot (n x p) => (m x p)
            int newAddr = ctx_->AllocMem(dimSizes1[0] * dimSizes2[1] * 2);
            if (!ctx_->IsArrAVariable(arr1)) {
                ctx_->FreeMem(arr1.addr);
            }
            if (!ctx_->IsArrAVariable(arr2)) {
                ctx_->FreeMem(arr2.addr);
            }
            CodeGen::Arr arrOut = {
                .size = arr1.shape[0] * arr2.shape[1],
                .addr = newAddr,
                .shape = {arr1.shape[0], arr2.shape[1]},
            };
            std::cerr << "addr: " << newAddr << " " << arrOut.shape[0] << "x"
                      << arrOut.shape[1] << std::endl;

            // initialise output with 0s
            CodeGen::ProgHeader((dimSizes1[0] * dimSizes2[1] * 2) / BLOCK_DIM, stream_);

            int addrReg = ctx_->IndexIntoReg(stream_, 1);
            ctx_->ASMImmOp("addi", addrReg, addrReg, arrOut.addr, stream_);
            stream_ << "sw zero, r" << addrReg << "\n";
            stream_ << "exit\n";
            ctx_->Reset();

            CodeGen::ProgHeader(totalSize1 * NUM_THREADS, stream_);
            int addr1Reg = ctx_->AllocReg();
            int col1Reg = ctx_->AllocReg();
            int row1Reg = ctx_->AllocReg();
            // TODO make enum for registers to make this an ASMOp, too
            //
            // 8 elements of data are within 16 memory elements
            // hence every 8 rows add 16
            // addr = blockIdx + threadIdx + (blockIdx//BLOCK_DIM)*16
            // TODO OPTIM make different program for every column to avoid data
            // dependency
            int tmpBlockIdxReg = ctx_->AllocReg();
            ctx_->ASMImmOp("srli", tmpBlockIdxReg, "%blockIdx",
                static_cast<int>(std::log2(static_cast<double>(NUM_THREADS))),
                stream_);
            ctx_->ASMImmOp("srli", row1Reg, tmpBlockIdxReg,
                static_cast<int>(std::log2(static_cast<double>(dimSizes1[1]))),
                stream_);
            ctx_->ASMImmOp("slli", addr1Reg, row1Reg,
                static_cast<int>(std::log2(static_cast<double>(dimSizes1[1]*2))),
                stream_);
            
            // row2 = col1
            ctx_->ASMImmOp("andi", col1Reg, tmpBlockIdxReg, dimSizes1[1]-1, stream_); 
            ctx_->FreeReg(tmpBlockIdxReg);

            ctx_->ASMOp("add", addr1Reg, addr1Reg, col1Reg, stream_);
            ctx_->ASMOp("add", addr1Reg, addr1Reg, "%threadIdx", stream_);
            ctx_->ASMImmOp("addi", addr1Reg, addr1Reg, arr1.addr, stream_);
            

            int outValReg = ctx_->AllocReg();
            int val1Reg = ctx_->AllocReg();
            int val2Reg = ctx_->AllocReg();

            stream_ << "# row1Reg: " << row1Reg << "\n";
            stream_ << "# col1Reg: " << col1Reg << "\n";
            stream_ << "# outValReg: " << outValReg << "\n";
            stream_ << "# val1Reg: " << val1Reg << "\n";
            stream_ << "# val2Reg: " << val2Reg << "\n";

            ctx_->LoadReg(val1Reg, addr1Reg, stream_); // needs tmp register

            // output address = row1 * arr2.shape[1]*2 + col2
            ctx_->FreeReg(addr1Reg);
            // TODO check if padded dim or non-padded dim required for loop limit
            for (int i = 0; i < arr2.shape[1]; i++) {
                int outAddrReg = ctx_->AllocReg();
                stream_ << "# outAddrReg: " << outAddrReg << "\n";
                ctx_->ASMImmOp("slli", outAddrReg, row1Reg,
                    static_cast<int>(std::log2(static_cast<double>(dimSizes2[1]*2))),
                    stream_);
                // col2Reg = i
                ctx_->ASMImmOp("addi", outAddrReg, outAddrReg, i, stream_);
                ctx_->ASMOp("add", outAddrReg, outAddrReg, "%threadIdx", stream_);
                ctx_->ASMImmOp("addi", outAddrReg, outAddrReg, arrOut.addr, stream_);

                ctx_->LoadReg(outValReg, outAddrReg, stream_);
                
                int addr2Reg = ctx_->AllocReg();
                stream_ << "# addr2Reg: " << addr2Reg << "\n";
                // row2 = col1, col2 = i
                ctx_->ASMImmOp("slli", addr2Reg, col1Reg,
                    static_cast<int>(std::log2(static_cast<double>(dimSizes2[1]*2))),
                    stream_);
                ctx_->ASMImmOp("addi", addr2Reg, addr2Reg, i, stream_);
                ctx_->ASMOp("add", addr2Reg, addr2Reg, "%threadIdx", stream_);
                ctx_->ASMImmOp("addi", addr2Reg, addr2Reg, arr2.addr, stream_);
                ctx_->LoadReg(val2Reg, addr2Reg, stream_);
                ctx_->FreeReg(addr2Reg);
                
                ctx_->ASMOp("fmul", val1Reg, val1Reg, val2Reg, stream_);
                ctx_->ASMOp("fadd", outValReg, outValReg, val1Reg, stream_);


                // only perform this for 1 lane
                // threadIdx addition to addr is necessary, though, to make accesses
                // of lanes contiguous
                stream_ << "seqi %threadIdx, 0\n";

                ctx_->predMode = true;
                ctx_->StoreReg(outValReg, outAddrReg, stream_);
                ctx_->predMode = false;
                ctx_->FreeReg(outAddrReg);
            }
            ctx_->FreeReg(outValReg);
            ctx_->FreeReg(col1Reg);
            ctx_->FreeReg({val1Reg, val2Reg});
            ctx_->FreeReg(row1Reg);

            stream_ << "exit\n";
            ctx_->Reset();

            ctx_->exprOut = {
                .t = CodeGen::OutType::mem,
                .v = arrOut,
            };

        } else {

            auto [dimSizes1, totalSize1] = CodeGen::PaddedArrSize(arr1.shape);
            auto [dimSizes2, totalSize2] = CodeGen::PaddedArrSize(arr2.shape);

            CodeGen::Arr tmpArr;
            if (arr2.size != arr1.size) {
                std::cerr << "only supported for same size arrays" << std::endl;
            }
           
            // shapes: (m x n) dot (n x p) => (m x p)
            int newAddr = ctx_->AllocMem(totalSize1 * 2); // take size of bigger operand
            if (!ctx_->IsArrAVariable(arr1)) {
                ctx_->FreeMem(arr1.addr);
            }
            if (!ctx_->IsArrAVariable(arr2)) {
                ctx_->FreeMem(arr2.addr);
            }
            CodeGen::Arr arrOut = {
                .size = arr1.size,
                .addr = newAddr,
                .shape = arr1.shape,
            };

            CodeGen::ProgHeader(totalSize1/BLOCK_DIM, stream_);
            int addr2Reg = ctx_->IndexIntoReg(stream_, 2);
            int addr1Reg = ctx_->AllocReg();
            int outAddrReg = ctx_->AllocReg();
            ctx_->ASMImmOp("addi", outAddrReg, addr2Reg, arrOut.addr, stream_);
            ctx_->ASMImmOp("addi", addr1Reg, addr2Reg, arr1.addr, stream_);
            ctx_->ASMImmOp("addi", addr2Reg, addr2Reg, arr2.addr, stream_);
            
            int val1Reg = ctx_->AllocReg();
            ctx_->LoadReg(val1Reg, addr1Reg, stream_);
            int val2Reg = ctx_->AllocReg();
            ctx_->LoadReg(val2Reg, addr2Reg, stream_);

            ctx_->EmitBinExpr(opType, val2Reg, val1Reg, val2Reg, stream_);
            ctx_->StoreReg(val2Reg, outAddrReg, stream_);

            ctx_->Reset();
            stream_ << "exit\n";

            ctx_->exprOut = {
                .t = CodeGen::OutType::mem,
                .v = arrOut,
            };
        }
    } else if (out1.t == CodeGen::OutType::reg || out2.t == CodeGen::OutType::reg) {
        // none of them is an array
        int op1Reg, op2Reg;
        if (out1.t != CodeGen::OutType::reg) {
            op1Reg = ctx_->ToRegCast(out1, stream_);
        } else {
            op1Reg = std::get<int>(out1.v);
        }

        if (out2.t != CodeGen::OutType::reg) {
            op2Reg = ctx_->ToRegCast(out2, stream_);
        } else {
            op2Reg = std::get<int>(out2.v);
        }

        ctx_->FreeReg({op1Reg, op2Reg});


        int outReg = ctx_->AllocReg();
        ctx_->exprOut = {
            .t = CodeGen::OutType::reg,
            .v = outReg,
        };
        ctx_->EmitBinExpr(opType, outReg, op1Reg, op2Reg, stream_);
    } else if (out1.t == CodeGen::OutType::real || out1.t == CodeGen::OutType::real) {
        // no array or register operand but one is a real
        double op1, op2;
        if (out1.t != CodeGen::OutType::real) {
            op1 = static_cast<double>(std::get<int>(out1.v));
        } else {
            op1 = std::get<double>(out1.v);
        }
        if (out1.t != CodeGen::OutType::real) {
            op2 = static_cast<double>(std::get<int>(out2.v));
        } else {
            op2 = std::get<double>(out2.v);
        }

        ctx_->exprOut = {
            .t = CodeGen::OutType::real,
            .v = CodeGen::BinaryOpToDoubleFn(opType)(op1, op2),
        };
    } else { // integers
        int op1 = std::get<int>(out1.v);
        int op2 = std::get<int>(out2.v);
        ctx_->exprOut = {
            .t = CodeGen::OutType::integer,
            .v = CodeGen::BinaryOpToIntFn(opType)(op1, op2),
        };
    }
}

void ASMGenVisitor::VisitUnaryExpr
(
    CodeGen::UnaryOp opType,
    std::shared_ptr<ASTNode> op
)
{
    op->Accept(this);

    if (ctx_->exprOut.t == CodeGen::OutType::mem) {
		CodeGen::Arr arr = std::get<CodeGen::Arr>(ctx_->exprOut.v);
        if (opType == CodeGen::UnaryOp::TRANSPOSE) {
            if (arr.shape.size() != 2) {
                std::cerr << "Codegen error: transpose only supported for 2D arrays" << std::endl;
                std::exit(1);
            }

            auto [dimSizes, totalSize] = CodeGen::PaddedArrSize(arr.shape);
            int newAddr = ctx_->AllocMem(totalSize*2);
            if (!ctx_->IsArrAVariable(arr)) {
                ctx_->FreeMem(arr.addr);
            }
            arr.addr = newAddr;
            int tmpDim = arr.shape[0];
            arr.shape[0] = arr.shape[1];
            arr.shape[1] = tmpDim;


            CodeGen::ProgHeader(totalSize/BLOCK_DIM, stream_);
            int addrReg = ctx_->IndexIntoReg(stream_);
            int valReg = ctx_->AllocReg();
            int rowReg = ctx_->AllocReg();
            int colReg = ctx_->AllocReg();

            // row = addr / dimSizes[1]
            ctx_->ASMImmOp("srli", rowReg, addrReg,
                static_cast<int>(std::log2(static_cast<double>(dimSizes[1]))),
                stream_);
            // col = addr % dimSizes[1]
            ctx_->ASMImmOp("andi", colReg, addrReg, dimSizes[1]-1, stream_);

            ctx_->ASMImmOp("slli", addrReg, rowReg,
                static_cast<int>(std::log2(static_cast<double>(dimSizes[1]*2))),
                stream_);
            ctx_->ASMOp("add", addrReg, addrReg, colReg, stream_);
            ctx_->LoadReg(valReg, addrReg, stream_);

            // addr = col * dimSizes[1] + row
            ctx_->ASMImmOp("slli", addrReg, colReg,
                static_cast<int>(std::log2(static_cast<double>(dimSizes[1]*2))),
                stream_);
            ctx_->ASMOp("add", addrReg, addrReg, rowReg, stream_);

            ctx_->ASMImmOp("addi", addrReg, addrReg, arr.addr, stream_);
            ctx_->StoreReg(valReg, addrReg, stream_);

            stream_ << "exit\n";
            ctx_->FreeReg(valReg);
            ctx_->Reset();

            ctx_->exprOut = {
                .t = CodeGen::OutType::mem,
                .v = arr,
            };
        } else {

            auto [dimSizes, totalSize] = CodeGen::PaddedArrSize(arr.shape);

            // shapes: (m x n) dot (n x p) => (m x p)
            int newAddr = ctx_->AllocMem(totalSize * 2); // take size of bigger operand
            if (!ctx_->IsArrAVariable(arr)) {
                ctx_->FreeMem(arr.addr);
            }

            CodeGen::Arr arrOut = {
                .size = arr.size,
                .addr = newAddr,
                .shape = arr.shape,
            };

            CodeGen::ProgHeader(totalSize/BLOCK_DIM, stream_);
            int addrReg = ctx_->IndexIntoReg(stream_, 2);
            int outAddrReg = ctx_->AllocReg();
            ctx_->ASMImmOp("addi", outAddrReg, addrReg, arrOut.addr, stream_);
            ctx_->ASMImmOp("addi", addrReg, addrReg, arr.addr, stream_);
            
            int valReg = ctx_->AllocReg();
            ctx_->LoadReg(valReg, addrReg, stream_);

            ctx_->EmitUnaryExpr(opType, valReg, valReg, stream_);
            ctx_->StoreReg(valReg, outAddrReg, stream_);

            ctx_->Reset();
            stream_ << "exit\n";

            ctx_->exprOut = {
                .t = CodeGen::OutType::mem,
                .v = arrOut,
            };
        }
    } else {
        if (opType == CodeGen::UnaryOp::TRANSPOSE) {
            std::cerr << "codegen error: transpose not supported for scalars" << std::endl;
            std::exit(1);
        }
        // exprOut
        // for every other apply to register or every element of array
        int opReg = std::get<int>(ctx_->exprOut.v);

        ctx_->FreeReg(opReg);

        int outReg = ctx_->AllocReg();
        ctx_->exprOut = {
            .t = CodeGen::OutType::reg,
            .v = outReg,
        };
        ctx_->EmitUnaryExpr(opType, outReg, opReg, stream_);
    }
}

/// this may only be called inside a program
void ASMGenVisitor::VisitVar(std::string var)
{
    if (var == "x") {
        int xReg = ctx_->XIntoReg(stream_);
        ctx_->exprOut = {
            .t = CodeGen::OutType::reg,
            .v = xReg,
        };
    } else if (var == "y") {
        int yReg = ctx_->YIntoReg(stream_);
        ctx_->exprOut = {
            .t = CodeGen::OutType::reg,
            .v = yReg,
        };
    } else if (var == "xytup") {
        int xReg = ctx_->XIntoReg(stream_);
        int yReg = ctx_->YIntoReg(stream_);
        // create 2x1 array containing x and y
        int addr = ctx_->AllocMem(BLOCK_DIM * 4);
        CodeGen::Arr arr = {
            .size = 2,
            .addr = addr,
            .shape = {2,1},
        };
        int addrReg = ctx_->AllocReg();
        ctx_->ASMImmOp("lui", addrReg, "zero", addr, stream_);
        ctx_->StoreReg(xReg, addrReg, stream_);
        ctx_->ASMImmOp("addi", addrReg, addrReg, 1, stream_);
        ctx_->StoreReg(yReg, addrReg, stream_);
        ctx_->exprOut = {
            .t = CodeGen::OutType::mem,
            .v = arr,
        };
    } else {
        // load array into lhs var
        if (ctx_->varMemMap.find(var) == ctx_->varMemMap.end()) {
            std::cerr << "Codegen error: variable '" << var
                      << "used but never initialised" << std::endl;
            std::exit(1);
        }
        ctx_->exprOut = ctx_->varMemMap[var];
    }
}

// OPTIM constant times constant multiplication
void ASMGenVisitor::VisitConst(double val)
{
    ctx_->exprOut = {
        .t = CodeGen::OutType::real,
        .v = val,
    };
}

void ASMGenVisitor::VisitArrayLiteral(std::vector<int> shape, std::vector<double> elements)
{
    // TODO pad shape with 1s to make it 2d
    int addrReg = ctx_->AllocReg();
    std::vector<int> newShape;
    if (shape.size() < 2) {
        newShape = {1, shape[0]};
    } else {
        newShape = shape;
    }
    auto [paddedDims, paddedSize] = CodeGen::PaddedArrSize(newShape);
    CodeGen::ProgHeader(1, stream_);
    int addr = ctx_->AllocMem(paddedSize * 2);
    ctx_->ConstIntoReg(addrReg, addr, stream_);

    CodeGen::Arr arr = {
        .size = static_cast<int>(elements.size()),
        .addr = addr,
        .shape = newShape,
    };

    ctx_->exprOut = {
        .t = CodeGen::OutType::mem,
        .v = arr,
    };
 
    // only execute this on one thread
    ctx_->PredicateBackup(stream_);
    stream_ << "seqi %threadIdx, 0\n";
    ctx_->predMode = true;
    int valReg = ctx_->AllocReg();
    //int tmpReg = ctx_->AllocReg();
    int stores = 0;
    for (double el : elements) {
        ctx_->DoubleIntoReg(valReg, el, stream_); 
        ctx_->StoreReg(valReg, addrReg, stream_);
        // this means addrReg will be incremented by 1 compared to what it
        // was at the start of the loop
        ctx_->ASMImmOp("addi", addrReg, addrReg, 1, stream_);
        stores++;
        if (stores % BLOCK_DIM == 0) {
            ctx_->ASMImmOp("addi", addrReg, addrReg, BLOCK_DIM, stream_);
        }
        // pad every dimension with 0s to align it with BLOCK_DIM
        if (stores == newShape[newShape.size()-1]) {
//             if (stores % BLOCK_DIM != 0) {
//                 ctx_->ASMImmOp("addi", tmpReg, "zero", 0, stream_);
//                 for (int i = 0; i < BLOCK_DIM - stores % BLOCK_DIM; i++) {
//                     ctx_->StoreReg(tmpReg, addrReg, stream_);
//                     ctx_->ASMImmOp("addi", addrReg, addrReg, 1, stream_);
//                 }
//             }
            ctx_->ASMImmOp("addi", addrReg, addrReg, BLOCK_DIM - stores % BLOCK_DIM, stream_);

            ctx_->ASMImmOp("addi", addrReg, addrReg, BLOCK_DIM, stream_);
            stores = 0;
        }
    }
    ctx_->PredicateRestore(stream_);
    ctx_->Reset();
    stream_ << "exit\n";
}
