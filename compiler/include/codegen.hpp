#ifndef CODEGEN_HPP
#define CODEGEN_HPP

#include <variant>
#include <tuple>
#include <list>
#include <cstdint>
#include <string>
#include <functional>
#include <iostream>
#include <cmath>
#include <unordered_map>
#include <vector>


#include "constants.hpp"
#include "context.hpp"

#define IDX_VAR_NAME "idx_var"


class CodeGen {
public:
    CodeGen()
        : exprOut {.t = OutType::reg, .v = -1},
          predMode {false},
          usedMem {{}},
          freeMem {{{0, MEM_SIZE}}},
          freeRegs_ {{11, 10, 9, 8, 7, 6, 5, 4}},
          usedRegs_ {}
    {}
    ~CodeGen() {}

	enum class BinaryOp {
		PLUS,
		MINUS,
		MULT,
		DIV,
        DOT,
	};

	enum class UnaryOp {
		SQRT,
		EXP,
		SIN,
		COS,
		MINUS,
        TRANSPOSE,
        RELU,
	};

    struct Arr {
        int size;
        int addr;
        std::vector<int> shape;
    };

    enum class OutType {
        reg,
        mem,
        real,
        integer,
    };


    struct ExprOut {
        OutType t;
        std::variant<Arr, int, double> v;
    };

    int AllocReg();

    /// frees most recently used register
    void FreeReg();
    /// frees specified register
    void FreeReg(int reg);
    /// frees all given registers
    void FreeReg(std::initializer_list<int> regs);

    void ASMOp(std::string op, int target, int src, std::ostream &stream);

    void ASMOp
    (
        std::string op, int valReg, std::string addrReg, std::ostream &stream
    );

    void ASMOp
    (
        std::string op, int target, int srcA, int srcB, std::ostream &stream
    );
    void ASMOp
    (
        std::string op, int target, std::string srcA, std::string srcB, std::ostream &stream
    );
    void ASMOp
    (
        std::string op, int target, int srcA, std::string srcB, std::ostream &stream
    );


	void ASMImmOp
	(
		std::string op, int src, int x, std::ostream &stream
	);

    void ASMImmOp
    (
        std::string op, int target, int src, int x, std::ostream &stream
    );
    void ASMImmOp
    (
        std::string op, int target, std::string src, int x, std::ostream &stream
    );
    void ASMImmOp
    (
        std::string op, int target, int src, double x, std::ostream &stream
    );


    void ChangeRegScale
    (
        int reg, double oldMin, double oldMax, double newMin, double newMax,
        std::ostream &stream
    );


    int IndexIntoReg(std::ostream &stream, int multiplier = 1);
	int XIntoReg(
			std::ostream &stream,
			double min =
				std::ceil(0.5 * (1.0 - 1.0/std::sqrt(2)) * static_cast<double>(PLOT_WIDTH)),
			double max =
				PLOT_WIDTH - std::ceil(0.5 * (1.0 - 1.0/std::sqrt(2)) * static_cast<double>(PLOT_WIDTH))
	);
	int YIntoReg(
			std::ostream &stream,
			double min =
				std::ceil(0.5 * (1.0 - 1.0/std::sqrt(2)) * static_cast<double>(PLOT_HEIGHT)),
			double max =
				PLOT_WIDTH - std::ceil(0.5 * (1.0 - 1.0/std::sqrt(2)) * static_cast<double>(PLOT_HEIGHT))
	);


    std::tuple<int, int, int> RotateRegs(
            int xReg, int yReg, int zReg,
            double angleX, double angleY, double angleZ,
            std::ostream &stream
    );

    void SetAxes
    (
            int xReg, int yReg, int zReg,
            double angleX, double angleY, double angleZ,
            std::ostream &stream
    );


    void Interpolate(int valReg, int addrReg, std::ostream &stream);

    void Interpolate_new(int valReg, int addrReg, std::ostream &stream);

    void FloatCoordsToAddrReg(int addrReg, int xReg, int yReg, std::ostream &stream,
            double min = std::ceil(0.5 * (1.0 - 1.0/std::sqrt(2)) * static_cast<double>(PLOT_WIDTH)),
            double max = PLOT_WIDTH - std::ceil(0.5 * (1.0 - 1.0/std::sqrt(2)) * static_cast<double>(PLOT_WIDTH)));

    std::string FormatOp(std::string op);

    void PredicateBackup(std::ostream &stream);
    void PredicateRestore(std::ostream &stream);

    void Reset();

    static void ProgHeader(int noBlocks, std::ostream &stream);

    void ConstIntoReg(int reg, uint32_t val, std::ostream &stream);
    void DoubleIntoReg(int reg, double val, std::ostream &stream);

    void StoreColour(int valReg, int addrReg, std::ostream &stream);

    void ResetMem(std::ostream &stream);

    void DisplayMem(std::ostream &stream);

    void TopBottomWhiteMargin(std::ostream &stream);

    void StoreReg(int valReg, int addrReg, std::ostream &stream);
    void LoadReg(int valReg, int addrReg, std::ostream &stream);

    int AllocMem(int size);
    void FreeMem(int addr);

    void EmitBinExpr(CodeGen::BinaryOp opType, int targetReg,
            int val1Reg, int val2Reg, std::ostream &stream);

    void EmitUnaryExpr(CodeGen::UnaryOp opType, int targetReg,
            int srcReg, std::ostream &stream);

    int ToRegCast(ExprOut out, std::ostream &stream);
    CodeGen::Arr ToArrCast(ExprOut out, std::ostream &stream);

    bool IsArrAVariable(Arr a);
    static uint32_t DoubleToTF18Int(double x);
    static std::tuple<std::vector<int>, int> PaddedArrSize(std::vector<int> &shape);

    static std::string ShapeToStr(std::vector<int> &shape);
    static std::string UnaryOpToStr(UnaryOp op);

    static std::function<int(int,int)> BinaryOpToIntFn(BinaryOp op);
    static std::function<double(double, double)> BinaryOpToDoubleFn(BinaryOp op);
    static std::string BinaryOpToStr(BinaryOp op);

    /// public member variables
    // TODO get rid of varMap
    std::unordered_map<std::string, int> varMap;
    std::unordered_map<std::string, ExprOut> varMemMap;

    ExprOut exprOut;

    bool predMode;
    bool predModeBackup;
    int predBackupReg;
    bool singleOut;
private:
    std::unordered_map<int, int> usedMem;
    std::vector<std::pair<int, int>> freeMem;
    std::list<int> freeRegs_;
    std::list<int> usedRegs_;
};

#endif
