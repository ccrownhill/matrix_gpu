#include <cstdint>
#include <cmath>
#include <list>
#include <functional>
#include <array>
#include <algorithm> // for std::find, std::copy
#include <vector>
#include <numeric> // for accumulate

#include "constants.hpp"
#include "codegen.hpp"

/// returns a new register from the list of available ones
/// if none are available anymore it will look at the list of registers
/// associated to variables and spill them (currently into nothingness because
/// only x and y are used which are reloaded not from memory but through
/// a certain computation contained in XIntoReg/YIntoReg)
int CodeGen::AllocReg()
{
    int reg;
    if (freeRegs_.empty()) {
        if (!varMap.empty()) {
            reg = -1;
            for (auto [k, v] : varMap) {
                if (k != "x" && k != "y") {
                    reg = v;
                    std::erase(usedRegs_, reg);
                    //std::cout << "from varmap: " << reg << std::endl;
                    varMap.erase(k);
                    break;
                }
            }
            if (reg == -1) {
                std::cerr << "register allocation: from varmap failed" << std::endl;
            }
        } else {
            std::cerr << "register allocation error" << std::endl;
            return -1;
        }
    } else {
        reg = freeRegs_.back();
        //std::cout << "normal: " << reg << std::endl;
        freeRegs_.pop_back();
    }
    usedRegs_.push_back(reg);
    std::cerr << "register alloc: " << reg << std::endl;
    return reg;
}

/// check if given map contains the given value at any of its keys
template <typename K, typename V>
static bool doesMapContainVal(std::unordered_map<K, V> map, V val)
{
    for (auto [k, v] : map) {
        if (v == val) {
            return true;
        }
    }
    return false;
}

/// free last used register but not if it is now in the variable map
void CodeGen::FreeReg()
{
    int reg = usedRegs_.back();
    if (doesMapContainVal(varMap, reg)) {
        return;
    }
    std::cerr << "register free: " << reg << std::endl;
    usedRegs_.pop_back();
    freeRegs_.push_back(reg);
}

/// free given register but not if it is now in the variable map
void CodeGen::FreeReg(int reg)
{
    if (doesMapContainVal(varMap, reg)) {
        return;
    }
    if (std::find(usedRegs_.begin(), usedRegs_.end(), reg) != usedRegs_.end()) {
        std::cerr << "register free: " << reg << std::endl;
        std::erase(usedRegs_, reg);
        freeRegs_.push_back(reg);
    }
}

/// free given registers but not if they are now in the variable map
void CodeGen::FreeReg(std::initializer_list<int> regs)
{
    for (int reg : regs) {
        if (doesMapContainVal(varMap, reg)) {
            continue;
        }
        if (std::find(usedRegs_.begin(), usedRegs_.end(), reg) != usedRegs_.end()) {
            std::cerr << "register free: " << reg << std::endl;
            std::erase(usedRegs_, reg);
            freeRegs_.push_back(reg);
        }
    }
}

std::string CodeGen::FormatOp(std::string op)
{
    if (predMode) {
        return op + ".p";
    } else {
        return op;
    }
}

void CodeGen::PredicateBackup(std::ostream &stream)
{
    predBackupReg = AllocReg();
    predModeBackup = predMode;
    predMode = false;
    ASMImmOp("addi", predBackupReg, "zero", 2, stream);
    ASMImmOp("addi.p", predBackupReg, "zero", 0, stream);
    predMode = predModeBackup;
}

void CodeGen::PredicateRestore(std::ostream &stream)
{
    FreeReg(predBackupReg);
    predMode = predModeBackup;
    predMode = false;
    ASMImmOp("slti", predBackupReg, 1, stream);
    predMode = predModeBackup;
}

void CodeGen::Reset()
{
    freeRegs_.sort();
    usedRegs_.sort();
    freeRegs_.merge(usedRegs_); // will leave usedRegs_ empty
    freeRegs_.reverse(); // want highest register first
    varMap.clear();
    predMode = false;
}

void CodeGen::ProgHeader(int noBlocks, std::ostream &stream)
{
    stream << "<" << noBlocks << "," << BLOCK_DIM << ">" << std::endl;
}

uint32_t CodeGen::DoubleToTF18Int(double x)
{
    //std::cerr << "double is " << x << std::endl;
	float f = static_cast<float>(x);
    uint32_t* bits = reinterpret_cast<uint32_t*>(&f);
    uint32_t rawBits = *bits;
    uint32_t input_sign = (rawBits >> 31) & 0x1;
    uint32_t input_exp = (rawBits >> 23) & 0xFF;
    uint32_t input_mantissa = rawBits & 0x7FFFFF;
    uint32_t output_sign = input_sign;
    uint32_t output_exp;
    uint32_t output_mantissa;
    uint32_t output_rawBits;
    output_exp = input_exp; // converts bias 127 to bias 63
    if (output_exp > 127) {
        output_exp -= 64;
        if (output_exp > 127)
            output_exp = 127;
        output_exp <<= 10;
    }
    // else if (output_exp > 127){
    //     output_exp = (output_exp - 64);
    //     output_exp <<= 9;
    // }
    else if (output_exp > 64){
        output_exp = (output_exp - 64) & 0x7F;
        output_exp <<= 10;
    }

    if (input_mantissa & 0x1000){
        output_mantissa = (input_mantissa >> 13) + 1;
    }
    else{
        output_mantissa = input_mantissa >> 13;
    }
    output_rawBits = (output_sign << 17) | output_exp | output_mantissa;
    return output_rawBits;
}

void CodeGen::ConstIntoReg(int reg, uint32_t val, std::ostream &stream)
{
    stream << FormatOp("lui") + " r" << reg << ", " << val << std::endl;
}

void CodeGen::DoubleIntoReg(int reg, double val, std::ostream &stream)
{
    stream << FormatOp("lui") + " r" << reg << ", 0x"
           << std::hex << CodeGen::DoubleToTF18Int(val) << std::dec << std::endl;
}

/// for operations with one register operand
void CodeGen::ASMOp(std::string op, int target, int src, std::ostream &stream)
{
    stream << FormatOp(op) << " r" << target << ", r" << src << std::endl;
}

void CodeGen::ASMOp
(
	std::string op, int valReg, std::string addrReg, std::ostream &stream
)
{
    stream << FormatOp(op) << " r" << valReg << ", " << addrReg << std::endl;
}

/// for operations with two register operands
void CodeGen::ASMOp
(
	std::string op, int target, int srcA, int srcB, std::ostream &stream
)
{
    stream << FormatOp(op) << " r" << target << ", r" << srcA << ", r" << srcB << std::endl;
}

/// for operations with two register operands
void CodeGen::ASMOp
(
	std::string op, int target, std::string srcA, std::string srcB, std::ostream &stream
)
{
    stream << FormatOp(op) << " r" << target << ", " << srcA << ", " << srcB << std::endl;
}

/// for operations with two register operands
void CodeGen::ASMOp
(
	std::string op, int target, int srcA, std::string srcB, std::ostream &stream
)
{
    stream << FormatOp(op) << " r" << target << ", r" << srcA << ", " << srcB << std::endl;
}

/// for operations with immediate integer values
void CodeGen::ASMImmOp
(
    std::string op, int src, int x, std::ostream &stream
)
{
    stream << FormatOp(op) << " r" << src << ", " << x << std::endl;
}

/// for operations with immediate integer values
void CodeGen::ASMImmOp
(
    std::string op, int target, int src, int x, std::ostream &stream
)
{
    stream << FormatOp(op) << " r" << target << ", r" << src << ", " << x << std::endl;
}

/// for operations with immediate integer values
void CodeGen::ASMImmOp
(
    std::string op, int target, std::string src, int x, std::ostream &stream
)
{
    stream << FormatOp(op) << " r" << target << ", " << src << ", " << x << std::endl;
}


/// for operations with immediate real values
void CodeGen::ASMImmOp
(
    std::string op, int target, int src, double x, std::ostream &stream
)
{
    int immReg = AllocReg();
    FreeReg(immReg);
    DoubleIntoReg(immReg, x, stream);
    stream << FormatOp(op) << " r" << target << ", r" << src << ", r" << immReg << std::endl;
}

void CodeGen::ChangeRegScale
(
    int reg, double oldMin, double oldMax, double newMin, double newMax,
	std::ostream &stream
)
{
    if (oldMin != 0.0) {
        ASMImmOp("fsub", reg, reg, oldMin, stream);
    }

    if ((newMax - newMin)/(oldMax - oldMin) != 0.0) {
        ASMImmOp("fmul", reg, reg, (newMax - newMin)/(oldMax - oldMin), stream);
    }
    // add the min because it will be negative
    if (newMin != 0.0) {
        ASMImmOp("fadd", reg, reg, newMin, stream);
    }
}

int CodeGen::IndexIntoReg(std::ostream &stream, int multiplier)
{
    int reg;
    if (varMap.find(IDX_VAR_NAME) == varMap.end()) {
        reg = AllocReg();
        
        ASMImmOp("slli", reg, "%blockIdx",
                static_cast<int>(std::log2(static_cast<double>(BLOCK_DIM * multiplier))),
                stream);
        ASMOp("add", reg, reg, "%threadIdx", stream);

        varMap[IDX_VAR_NAME] = reg;
    } else {
        reg = varMap[IDX_VAR_NAME];
    }
    return reg;
}

int CodeGen::XIntoReg(std::ostream &stream, double min, double max)
{
    int reg;
	if (varMap.find("x") != varMap.end()) {
		reg = varMap["x"];
	} else {
        reg = AllocReg();
        int idxReg = IndexIntoReg(stream);
        // remainder of divide by PLOT_WIDTH to get column
        ASMImmOp("andi", reg, idxReg, PLOT_WIDTH - 1, stream);
        ASMOp("cvtif", reg, reg, stream);
        ChangeRegScale(reg, min, max, X_MIN, X_MAX, stream);
        //ASMImmOp("fadd", reg, reg, 0.5 * (X_MAX - X_MIN)/PLOT_WIDTH, stream);
        //varMap["x"] = reg; 
        FreeReg(idxReg);
	}
    return reg;
}

int CodeGen::YIntoReg(std::ostream &stream, double min, double max)
{
    int reg;
	if (varMap.find("y") != varMap.end()) {
		reg = varMap["y"];
	} else {
        reg = AllocReg();
        int idxReg = IndexIntoReg(stream);
        // divide by PLOT_WIDTH without remainder to get row
        ASMImmOp("srli", reg, idxReg,
                static_cast<int>(std::log2(static_cast<double>(PLOT_WIDTH))),
                stream);

        // flip y to make bottom be the lowest value instead of top
        ASMImmOp("subi", reg, reg, PLOT_HEIGHT, stream);
        ASMOp("sub", reg, 0, reg, stream);

        ASMOp("cvtif", reg, reg, stream);
        ChangeRegScale(reg, min, max, Y_MIN, Y_MAX, stream);
        //ASMImmOp("fadd", reg, reg, 0.5 * (Y_MAX - Y_MIN)/PLOT_HEIGHT, stream);
        //varMap["y"] = reg; 
        FreeReg(idxReg);
	}
    return reg;
}

// TODO it might also be possible to write to oldXReg, oldYReg, oldZReg directly
/// Output registers that correspond to oldXReg, oldYReg, oldZreg after applying
/// the 3D rotation specified by angleX, angleY, angleZ.
///
/// Does not change the value of oldXReg, oldYReg, oldZReg
std::tuple<int, int, int> CodeGen::RotateRegs
(
            int oldXReg, int oldYReg, int oldZReg,
            double angleX, double angleY, double angleZ,
            std::ostream &stream
)
{
    double xSin = std::sin(angleX), ySin = std::sin(angleY), zSin = std::sin(angleZ);
    double xCos = std::cos(angleX), yCos = std::cos(angleY), zCos = std::cos(angleZ);

    int tmpReg = AllocReg();
    int newXReg = AllocReg();

    // OPTIM check for 0 angles
    ASMImmOp("fmul", newXReg, oldXReg, yCos * zCos, stream);

    if (xSin * ySin * zCos - xCos * zSin != 0.0) {
        ASMImmOp("fmul", tmpReg, oldYReg, xSin * ySin * zCos - xCos * zSin, stream);
        ASMOp("fadd", newXReg, newXReg, tmpReg, stream);
    }

    if (xCos * ySin * zCos + xSin * zSin != 0.0) {
        ASMImmOp("fmul", tmpReg, oldZReg, xCos * ySin * zCos + xSin * zSin, stream);
        ASMOp("fadd", newXReg, newXReg, tmpReg, stream);
    }

    int newYReg = AllocReg();

    ASMImmOp("fmul", newYReg, oldXReg, yCos * zSin, stream);

    if (xSin * ySin * zSin + xCos * zCos != 0.0) {
        ASMImmOp("fmul", tmpReg, oldYReg, xSin * ySin * zSin + xCos * zCos, stream);
        ASMOp("fadd", newYReg, newYReg, tmpReg, stream);
    }

    if (xCos * ySin * zSin - xSin * zCos != 0.0) {
        ASMImmOp("fmul", tmpReg, oldZReg, xCos * ySin * zSin - xSin * zCos, stream);
        ASMOp("fadd", newYReg, newYReg, tmpReg, stream);
    }


    int newZReg = AllocReg();

    ASMImmOp("fmul", newZReg, oldXReg, -ySin, stream);

    if (xSin * yCos != 0.0) {
        ASMImmOp("fmul", tmpReg, oldYReg, xSin * yCos, stream);
        ASMOp("fadd", newZReg, newZReg, tmpReg, stream);
    }

    if (xCos * yCos != 0.0) {
        ASMImmOp("fmul", tmpReg, oldZReg, xCos * yCos, stream);
        ASMOp("fadd", newZReg, newZReg, tmpReg, stream);
    }

    FreeReg(tmpReg);
    return {newXReg, newYReg, newZReg};
}

/// Change zReg to mininum of z-range to draw axis black if point is below any
/// of the 3 axes.
/// Parametric equation of the axes (for \f$\forall \lambda \in \R\f$)
/// x-axis:
/// - \f$x = \lambda cos(angleY) cos(angleZ)\f$
/// - \f$y = \lambda cos(angleY) sin(angleZ)\f$
/// - \f$z = \lambda (-sin(angleY))
/// for the other axis it will be the same but using the other columns from
/// the standard 3D rotation matrix: https://en.wikipedia.org/wiki/Rotation_matrix#General_3D_rotations
/// 
/// To check if a point is below the z-axis we use the x and y equation to get
/// \f$\lambda\f$ and then compute the z-value of the axis to check if its above
/// the value in zReg in which case it will be replaced with the mininum value
/// in its range
void CodeGen::SetAxes
(
        int xReg, int yReg, int zReg, double angleX, double angleY, double angleZ,
        std::ostream &stream
)
{
    // store matrix by columns

    std::array<std::array<double, 3>, 3> rotMatTranspose {{
        {
            std::cos(angleY) * std::cos(angleZ),
            std::cos(angleY) * std::sin(angleZ),
            -std::sin(angleY),
        },
        {
            std::sin(angleX) * std::sin(angleY) * std::cos(angleZ) -
                std::cos(angleX) * std::sin(angleZ),
            std::sin(angleX) * std::sin(angleY) * std::sin(angleZ) +
                std::cos(angleX) * std::cos(angleZ),
            std::sin(angleX) * std::cos(angleY),
        },
        {
            std::cos(angleX) * std::sin(angleY) * std::cos(angleZ) +
                std::sin(angleX) * std::sin(angleZ),
            std::cos(angleX) * std::sin(angleY) * std::sin(angleZ) -
                std::sin(angleX) * std::cos(angleZ),
            std::cos(angleX) * std::cos(angleY),
        },
    }};


    PredicateBackup(stream);

    predMode = false;

    int tmpReg = AllocReg();
    int xLambdaReg = AllocReg();
    int yLambdaReg = AllocReg();
    int newZReg = AllocReg();
    ASMImmOp("addi", newZReg, zReg, 0, stream);


    for (auto col : rotMatTranspose) {
        predMode = false;
        bool xZero = false;
        bool yZero = false;
        double errMargin = EQUALITY_ERROR_MARGIN;
        if (col[0] != 0.0) {
            if (EQUALITY_ERROR_MARGIN / std::fabs(col[0]) > errMargin) {
                errMargin = EQUALITY_ERROR_MARGIN / std::fabs(col[0]);
            }
            DoubleIntoReg(xLambdaReg, col[0], stream);
            ASMOp("fdiv", xLambdaReg, xReg, xLambdaReg, stream);
        } else {
            xZero = true;
        }
        if (col[1] != 0.0) {
            if (EQUALITY_ERROR_MARGIN / std::fabs(col[1]) > errMargin) {
                errMargin = EQUALITY_ERROR_MARGIN / std::fabs(col[1]);
            }
            DoubleIntoReg(yLambdaReg, col[1], stream);
            ASMOp("fdiv", yLambdaReg, yReg, yLambdaReg, stream);
        } else {
            yZero = true;
        }
        
        if (xZero) {
            // check if xReg = 0
            ASMOp("fabs", tmpReg, xReg, stream); 
            DoubleIntoReg(xLambdaReg, errMargin, stream);
            ASMOp("fslt", tmpReg, xLambdaReg, stream);
            predMode = true;
            // lambda to use is yLambdaReg
            int backupReg = xLambdaReg;
            xLambdaReg = yLambdaReg;
            yLambdaReg = backupReg; // so that every register will be freeated
                                    // in the end
        }

        if (yZero) {
            // check if yReg = 0
            ASMOp("fabs", tmpReg, yReg, stream); 
            // use EQUALITY_ERROR_MARGIN because it is not multiplied by anything
            DoubleIntoReg(yLambdaReg, EQUALITY_ERROR_MARGIN, stream);
            ASMOp("fslt", tmpReg, yLambdaReg, stream);
            predMode = true;
        }


        if (!xZero && !yZero) {
            // check if their lambda coefficients match
            ASMOp("fsub", tmpReg, xLambdaReg, yLambdaReg, stream); 
            ASMOp("fabs", tmpReg, tmpReg, stream); 
            DoubleIntoReg(yLambdaReg, errMargin, stream);
            ASMOp("fslt", tmpReg, yLambdaReg, stream);
            predMode = true;
        }

        FreeReg(yLambdaReg);

        if (xZero && yZero) {
            // this will only happen if it also passed the checks that x=y=0
            // where passed because this operation is performed predicatedly
            DoubleIntoReg(newZReg, Z_MIN, stream);
        } else {
            // load z value of axis into xLambdaReg
            DoubleIntoReg(tmpReg, col[2], stream);
            ASMOp("fmul", xLambdaReg, xLambdaReg, tmpReg, stream);
            
            DoubleIntoReg(tmpReg, EQUALITY_ERROR_MARGIN, stream);
            ASMOp("fadd", xLambdaReg, xLambdaReg, tmpReg, stream);
            ASMOp("fslt", zReg, xLambdaReg, stream);

            predMode = true;
            DoubleIntoReg(newZReg, Z_MIN, stream);
        }
    }

    predMode = false;
    ASMImmOp("addi", zReg, newZReg, 0, stream);
    PredicateRestore(stream);
    FreeReg({tmpReg, xLambdaReg, newZReg});
}

/// Interpolate point with 3 points on left, top left, and top of it.
///
/// This is done by checking if valReg contains invalid value, i.e.
/// minus infinity in TF18 representation, and if that is the case it takes
/// first non-minus-infinity value of the three points to its left/top
void CodeGen::Interpolate(int valReg, int addrReg, std::ostream &stream)
{
    PredicateBackup(stream);
    //int illegalReg = AllocReg();
    // NOTE if tmpAddrReg introduces too much register pressure then just use
    // addrReg and add 512 at the end of this function to it to restore it
    // to its previous value
    int tmpAddrReg = AllocReg();
    ASMOp("seq", valReg, "zero", stream);
    ASMImmOp("subi", tmpAddrReg, addrReg, 1, stream);
    predMode = true;
    ASMOp("lw", valReg, tmpAddrReg, stream);

    ASMOp("seq", valReg, "zero", stream);
    predMode = false;
    ASMImmOp("subi", tmpAddrReg, addrReg, PLOT_WIDTH, stream);
    predMode = true;
    ASMOp("lw", valReg, tmpAddrReg, stream);

    ASMOp("seq", valReg, "zero", stream);
    predMode = false;
    ASMImmOp("subi", tmpAddrReg, addrReg, PLOT_WIDTH + 1, stream);
    predMode = true;
    ASMOp("lw", valReg, tmpAddrReg, stream);

    PredicateRestore(stream);
    FreeReg({tmpAddrReg});
}


/// converts values in xReg and yReg to integer address into 1D frame buffer
/// array
/// will modify xReg and yReg contents
void CodeGen::FloatCoordsToAddrReg(int addrReg, int xReg, int yReg, std::ostream &stream,
        double min, double max)
{
	ChangeRegScale(xReg, X_MIN, X_MAX, min, max, stream);
	ChangeRegScale(yReg, Y_MIN, Y_MAX, min, max, stream);

    ASMOp("cvtfi", xReg, xReg, stream);
    ASMOp("cvtfi", yReg, yReg, stream);
    ASMImmOp("subi", yReg, yReg, PLOT_HEIGHT, stream);
    ASMOp("sub", yReg, 0, yReg, stream);
    ASMImmOp("slli", addrReg, yReg,
            static_cast<int>(std::log2(static_cast<double>(PLOT_WIDTH))),
            stream);
    ASMOp("add", addrReg, addrReg, xReg, stream);
}

void CodeGen::StoreColour(int valReg, int addrReg, std::ostream &stream)
{
    ASMOp("cvtfc", valReg, valReg, stream);
    ASMOp("spix", valReg, addrReg, stream);
}

void CodeGen::ResetMem(std::ostream &stream)
{
	CodeGen::ProgHeader((PLOT_WIDTH * PLOT_HEIGHT)/(BLOCK_DIM*(MAX_INSTR/4)), stream);
	int addrReg = IndexIntoReg(stream);
    for (int i = 0; i < 64; i++) {
	    ASMOp("sw", 0, addrReg, stream);
        ASMImmOp("addi", addrReg, addrReg, BLOCK_DIM, stream);

    }
	stream << "exit" << std::endl;
	Reset();
}

void CodeGen::DisplayMem(std::ostream &stream)
{
	// note that this only works with BLOCK_DIM = 8
	CodeGen::ProgHeader(PLOT_HEIGHT * NUM_THREADS, stream);
	for (int i = 0;
		i < ((SCREEN_WIDTH - PLOT_WIDTH)/2) / (NUM_THREADS * BLOCK_DIM);
		i++) {
		stream << "disp zero\n";
	}

	int addrReg = AllocReg();
	int colReg = AllocReg();
	int valReg = AllocReg();

	// col = (blockIdx % NUM_THREADS) * BLOCK_DIM + threadIdx + i * NUM_THREADS * BLOCK_DIM
	// note that NUM_THREADS has to be a power of 2
	ASMImmOp("andi", colReg, "%blockIdx", NUM_THREADS-1, stream);
	ASMImmOp("slli", colReg, colReg,
		static_cast<int>(std::log2(static_cast<double>(BLOCK_DIM))),
		stream);
	ASMOp("add", colReg, colReg, "%threadIdx", stream);

	// row = blockIdx / NUM_THREADS (integer division)
	// addr = row * PLOT_WIDTH + col
	ASMImmOp("srli", addrReg, "%blockIdx",
		static_cast<int>(std::log2(static_cast<double>(NUM_THREADS))),
		stream);
	ASMImmOp("slli", addrReg, addrReg,
		static_cast<int>(std::log2(static_cast<double>(PLOT_WIDTH))),
		stream);
	ASMOp("add", addrReg, addrReg, colReg, stream);

	ASMOp("lw", valReg, addrReg, stream);
	Interpolate(valReg, addrReg, stream);
	stream << "disp r" << valReg << std::endl;

	for (int i = 1; i < PLOT_WIDTH / (NUM_THREADS * BLOCK_DIM); i++) {
		ASMImmOp("addi", addrReg, addrReg,
				NUM_THREADS * BLOCK_DIM, stream);

		ASMOp("lw", valReg, addrReg, stream);
		Interpolate(valReg, addrReg, stream);
		stream << "disp r" << valReg << std::endl;
	}
	FreeReg({addrReg, colReg, valReg});

	for (int i = 0;
		i < ((SCREEN_WIDTH - PLOT_WIDTH)/2) / (NUM_THREADS * BLOCK_DIM);
		i++) {
		stream << "disp zero\n";
	}

	stream << "exit" << std::endl;
	Reset();
}

void CodeGen::TopBottomWhiteMargin(std::ostream &stream)
{
	CodeGen::ProgHeader(80,
			stream);
	for (int i = 0; i < 208; i++){ // 640
	    stream << "disp r0" << std::endl;
    }
	stream << "exit" << std::endl;

	Reset();

}

void CodeGen::StoreReg(int valReg, int addrReg, std::ostream &stream)
{
    int tmpReg = AllocReg();
    // TODO remove when switching to 18 bits
    // possible optimisation: if more registers available store shifted
    // copy of valReg in another reg to avoid shifting it left again in the end
    //ASMImmOp("srli", valReg, valReg, 6, stream);
	ASMImmOp("andi", tmpReg, valReg, (1<<9) - 1, stream);
	ASMOp("sw", tmpReg, addrReg, stream);
	ASMImmOp("srli", tmpReg, valReg, 9, stream);
	ASMImmOp("addi", addrReg, addrReg, BLOCK_DIM, stream);
	ASMOp("sw", tmpReg, addrReg, stream);
    // restore addrReg and valReg
	ASMImmOp("subi", addrReg, addrReg, BLOCK_DIM, stream);
    //ASMImmOp("slli", valReg, valReg, 6, stream);
    FreeReg(tmpReg);
}

void CodeGen::LoadReg(int valReg, int addrReg, std::ostream &stream)
{
    int tmpReg = AllocReg();
    bool oldPredMode = predMode;
	ASMOp("lw", valReg, addrReg, stream);

    predMode = false; // to make all read the same address
	ASMImmOp("addi", tmpReg, addrReg, BLOCK_DIM, stream);
    predMode = oldPredMode;

	ASMOp("lw", tmpReg, tmpReg, stream);
	ASMImmOp("slli", tmpReg, tmpReg, 9, stream);
	ASMOp("add", valReg, valReg, tmpReg, stream);

    // TODO remove this as soon as everything is 18 bits
    //ASMImmOp("slli", valReg, valReg, 6, stream);

    FreeReg(tmpReg);
}

int CodeGen::AllocMem(int size)
{
    for (int i = 0; i < freeMem.size(); i++) {
        auto [addr, sz] = freeMem[i];
        if (sz >= size) {
            if (sz > size) {
                freeMem[i].second = sz - size;
                freeMem[i].first += size;
            } else {
                freeMem.erase(freeMem.begin() + i);
            }
            usedMem[addr] = size;
            std::cerr << "allocating " << size << " elements @ " << addr << "\n";
            return addr;
        }
    }
    // nothing free
    return -1;
}

void CodeGen::FreeMem(int addr)
{
    std::cerr << "freeing " << usedMem[addr] << " elements @ " << addr << std::endl;
    int size = usedMem[addr];
    usedMem.erase(addr);
    bool added = false;
    for (int i = 0; i < freeMem.size(); i++) {
        auto [a, sz] = freeMem[i];
        if (addr + size == a) {
            std::cerr << "merging with next entry" << std::endl;
            freeMem[i].second += size;
            freeMem[i].first = addr;
            added = true;
        } else if (addr + size < a) {
            std::cerr << "inserting before next entry" << std::endl;
            freeMem.insert(freeMem.begin() + i, {addr, size});
            added = true;
        }
        // check if it aligns with previous one
        if (i > 0 && freeMem[i-1].first + freeMem[i-1].second == addr) {
            std::cerr << "merging with previous entry" << std::endl;
            freeMem[i-1].second += freeMem[i].second;
            freeMem.erase(freeMem.begin() + i);
        }
        if (added) {
            std::cerr << "Number of items in freeMem: " << freeMem.size() << std::endl;
            break;
        }
    }

    for (auto [addr, size] : freeMem) {
        std::cerr << "@ " << addr << ": " << size << std::endl;
    }
}

void CodeGen::EmitBinExpr(CodeGen::BinaryOp opType, int targetReg,
        int val1Reg, int val2Reg, std::ostream &stream)
{
	std::string operation = "f" + CodeGen::BinaryOpToStr(opType);
	ASMOp(operation, targetReg, val1Reg, val2Reg, stream);
}

void CodeGen::EmitUnaryExpr(CodeGen::UnaryOp opType, int targetReg,
		int srcReg, std::ostream &stream)
{
    bool oldPredMode = predMode;
    switch (opType) {
    case UnaryOp::MINUS:
        ASMOp("fsub", targetReg, 0, srcReg, stream);
        break;
    case UnaryOp::RELU:
        ASMOp("fslt", targetReg, 0, stream);
        predMode = true;
        ASMImmOp("lui", targetReg, 0, stream);
        predMode = oldPredMode;
        break;
    case UnaryOp::SIN:
    case UnaryOp::COS:
        ASMOp("cvtfr", srcReg, srcReg, stream);
    default:
        std::string operation = "f" + CodeGen::UnaryOpToStr(opType);
        ASMOp(operation, targetReg, srcReg, stream);
    }
}

int CodeGen::ToRegCast(ExprOut out, std::ostream &stream)
{
	int outReg;
    if (out.t == CodeGen::OutType::mem) {
        CodeGen::Arr arr = std::get<CodeGen::Arr>(out.v);
        if (arr.size != 1) {
            std::cerr << "CodeGen error: compiler can only use size 1 array as scalar" << std::endl;
			std::exit(1);
        }
        outReg = AllocReg();

        LoadReg(outReg, arr.addr, stream);
    } else if (out.t == CodeGen::OutType::reg) {
        outReg = std::get<int>(out.v); // target output reg
    } else if (out.t == CodeGen::OutType::real) {
        outReg = AllocReg();
        DoubleIntoReg(outReg, std::get<double>(out.v), stream);
    } else if (out.t == CodeGen::OutType::integer) {
        outReg = AllocReg();
        ConstIntoReg(outReg, std::get<int>(out.v), stream);
    } else {
        std::cerr << "unrecognised output type: should never happen" << std::endl;
        std::exit(1);
    }
    return outReg;
}

// TODO fix this for all operand types
CodeGen::Arr CodeGen::ToArrCast(ExprOut out, std::ostream &stream)
{
    if (out.t == CodeGen::OutType::mem) {
        return std::get<CodeGen::Arr>(out.v);
    } else {
        int valReg = ToRegCast(out, stream);
        int addr = AllocMem(BLOCK_DIM*4);
        CodeGen::Arr arrOut = {
            .size = 1,
            .addr = addr,
            .shape = {1, 1},
        };
        int addrReg = AllocReg();
        ASMImmOp("addi", addrReg, "zero", addr, stream);
        // TODO maybe need "nop"s around this
        StoreReg(valReg, addrReg, stream);
        return arrOut;
    }
}

bool CodeGen::IsArrAVariable(Arr a)
{
	for (auto [k, v] : varMemMap) {
        if (v.t == CodeGen::OutType::mem &&
            std::get<CodeGen::Arr>(v.v).addr == a.addr) {
            return true;
        }
    }

    return false;
}


std::tuple<std::vector<int>, int> CodeGen::PaddedArrSize(std::vector<int> &shape)
{
    int out = 1;
    std::vector<int> dimSizes;
    for (int dim : shape) {
        int dimSize;
        if (dim % BLOCK_DIM == 0) {
            dimSize = dim;
        } else {
            dimSize = dim + BLOCK_DIM - (dim % BLOCK_DIM);
        }
        dimSizes.push_back(dimSize);
        out *= dimSize;
    }
    return {dimSizes, out};
}

std::string CodeGen::ShapeToStr(std::vector<int> &shape)
{
    std::string out = "";
    out += "(";
    for (int i : shape) {
        out += std::to_string(i) + ",";
    }
    out += ")";
    return out;
}

std::string CodeGen::UnaryOpToStr(CodeGen::UnaryOp op)
{
    switch (op) {
    case UnaryOp::MINUS:
        return "-";
    case UnaryOp::EXP:
        return "exp";
    case UnaryOp::SIN:
        return "sin";
    case UnaryOp::COS:
        return "cos";
    case UnaryOp::SQRT:
        return "sqrt";
    case UnaryOp::TRANSPOSE:
        return "transpose";
    case UnaryOp::RELU:
        return "relu";
    }
}

std::function<int(int,int)> CodeGen::BinaryOpToIntFn(BinaryOp op)
{
    switch (op) {
    case BinaryOp::PLUS:
        return [](int a, int b) { return a + b; };
    case BinaryOp::MINUS:
        return [](int a, int b) { return a - b; };
    case BinaryOp::MULT:
        return [](int a, int b) { return a * b; };
    case BinaryOp::DIV:
        return [](int a, int b) { return a / b; };
    case BinaryOp::DOT:
        std::cerr << "dot product not supported as scalar operation" << std::endl;
        std::exit(1);
    }

}

std::function<double(double,double)> CodeGen::BinaryOpToDoubleFn(BinaryOp op)
{
    switch (op) {
    case BinaryOp::PLUS:
        return [](double a, double b) { return a + b; };
    case BinaryOp::MINUS:
        return [](double a, double b) { return a - b; };
    case BinaryOp::MULT:
        return [](double a, double b) { return a * b; };
    case BinaryOp::DIV:
        return [](double a, double b) { return a / b; };
    case BinaryOp::DOT:
        std::cerr << "dot product not supported as scalar operation" << std::endl;
        std::exit(1);
    }

}


std::string CodeGen::BinaryOpToStr(CodeGen::BinaryOp op)
{
    switch (op) {
    case BinaryOp::PLUS:
        return "add";
    case BinaryOp::MINUS:
        return "sub";
    case BinaryOp::MULT:
        return "mul";
    case BinaryOp::DIV:
        return "div";
    case BinaryOp::DOT:
        return "dot";
    }
}
