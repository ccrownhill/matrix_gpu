`include "pkg/opcode_pkg.svh"
`include "pkg/pipeline_reg_pkg.svh"

module simd_lane
    import opcode_pkg::*;
    import pipeline_reg_pkg::*;
#(
    parameter BLOCK_DIM = 1,
              THREAD_IDX = 0
)(
    input logic clk,
    input logic stall,
    input logic [3:0] thread_num,
    input logic [17:0] block_idx,
    input func5_t func5,
    input logic [17:0] imm,
    input logic [3:0] rs1,
    input logic [3:0] rs2,
    input logic [3:0] rd,
    input logic is_int,
    input logic is_imm,
    input logic is_fadd,
    input logic is_cordic,
    input logic is_mem,
    input logic is_memwrite,
    input logic is_memforce,
    input logic is_cvtfc,
    input logic is_disp,
    input logic is_predicate_setter,
    input logic is_predicate_getter,
    input logic [8:0] mmu_read_data,

    output logic mmu_write_en,
    output logic mmu_write_force,
    output logic [8:0] mmu_write_data,
    output logic [17:0] mmu_addr,
    output logic [23:0] rgb_val,
    output logic disp_valid
);

// Read regfile -> FU pipeline register
read_fu_reg readFURegIn;
read_fu_reg readFURegOut;

// Int pipeline register
int_result_reg intResultReg1In;
int_result_reg intResultReg1Out;

// Result registers for integer pipeline
int_write_reg resultRegsIn[9];
int_write_reg resultRegsOut[9];

// Register file write input
int_write_reg regFileWriteIn;

// Different pipeline lengths mean results appear in different cycles
logic [17:0] int_fu_out;    // Result appears in same cycle
logic cvtfi_valid;          // Result appears after 1 cycle
logic [17:0] cvtfi_result;  // Result appears after 1 cycle
logic [8:0] cvtfc_result;   // Result appears in same cycle

// FP_ADD FU / Int FU -> Write regfile pipeline register
int_write_reg addSubWriteRegIn;
int_write_reg addSubWriteRegOut;

// CORDIC FU -> Write regfile pipeline register
fp_write_reg cordicWriteRegIn;
fp_write_reg cordicWriteRegOut;

// CORDIC -> fp_standardise unit interconnections
func5_t      func5_cordic;
logic        valid_out_cordic;
logic [25:0] result_cordic;
logic        override_cordic;
logic [17:0] override_cordic_val;

// fp_standardise -> Write regfile pipeline register
logic reg_file_predicate;

logic fadd_valid_conn;
logic fadd_sign_conn;
logic [6:0] fadd_exponent_conn;
logic [17:0] fadd_mantissa_conn;

reg_file #(BLOCK_DIM, THREAD_IDX) RegFile (
    .clk(clk),
    .read_thread(thread_num),
    .block_idx(block_idx),
    .rs1(rs1),
    .rs2(rs2),
    .write_thread(regFileWriteIn.thread_num),
    .write_rd(regFileWriteIn.rd),
    .write_data(regFileWriteIn.write_data),
    .write_en(regFileWriteIn.regwrite),
    .predicate_write_en(regFileWriteIn.set_pred),
    .predicate_in(regFileWriteIn.new_pred_val),

    .reg1_out(readFURegIn.reg1),
    .reg2_out(readFURegIn.reg2),
    .predicate_out(reg_file_predicate)
);

int_fu IntFU (
    .reg1(readFURegOut.reg1),
    .reg2(readFURegOut.reg2),
    .imm(readFURegOut.imm),
    .is_imm(readFURegOut.is_imm),
    .func5(readFURegOut.func5),

    .result(int_fu_out)
);

fadd_fu FAddFU (
    .clk(clk),
    .op1_in(readFURegOut.reg1),
    .op2_in(readFURegOut.reg2),
    .func5(readFURegOut.func5),
    .fadd_valid(readFURegOut.is_fadd & readFURegOut.predicate),

    .cvtfi_valid(cvtfi_valid),
    .cvtfi_result(cvtfi_result),

    .sign_out(fadd_sign_conn),
    .exponent_out(fadd_exponent_conn),
    .mantissa_out(fadd_mantissa_conn),
    .std_valid_out(fadd_valid_conn)
);

cordic_fu CordicFU (
    .clk(clk),
    .func_in(readFURegOut.func5),
    .valid_in(readFURegOut.is_cordic & readFURegOut.predicate),
    .op1_in(readFURegOut.reg1),
    .op2_in(readFURegOut.reg2),

    .func_out(func5_cordic),
    .valid_out(valid_out_cordic),
    .result(result_cordic),
    .override_out(override_cordic),
    .override_val_out(override_cordic_val)
);

fp_standardise FpStandardise(
    .clk(clk),

    .func5(func5_cordic),
    .valid_out_cordic(valid_out_cordic),
    .result_cordic(result_cordic),
    .override_cordic(override_cordic),
    .override_cordic_val(override_cordic_val),

    .valid_fadd(fadd_valid_conn),
    .fadd_sign_in(fadd_sign_conn),
    .fadd_exponent_in(fadd_exponent_conn),
    .fadd_mantissa_in(fadd_mantissa_conn),

    .valid_std(cordicWriteRegIn.regwrite),
    .res_std(cordicWriteRegIn.write_data),
    .set_pred(cordicWriteRegIn.set_pred),
    .new_pred_val(cordicWriteRegIn.new_pred_val)
);

cvtfc_fu CvtfcFU (
    .tf18_value(readFURegOut.reg1),

    .colour_index(cvtfc_result)
);

disp_fu DispFU (
    .colour_index(readFURegOut.reg1[8:0]),

    .rgb(rgb_val)
);

always_comb begin
    // Regfile read -> FU register inputs
    readFURegIn.func5 = func5;
    readFURegIn.thread_num = thread_num;
    readFURegIn.rd = rd;
    readFURegIn.imm = imm;
    readFURegIn.is_int = is_int;
    readFURegIn.is_imm = is_imm;
    readFURegIn.is_cordic = is_cordic;
    readFURegIn.is_fadd = is_fadd;
    readFURegIn.is_mem = is_mem;
    readFURegIn.is_memwrite = is_memwrite;
    readFURegIn.is_memforce = is_memforce;
    readFURegIn.is_cvtfc = is_cvtfc;
    readFURegIn.is_disp = is_disp;
    readFURegIn.is_predicate_setter = is_predicate_setter;
    readFURegIn.predicate = (~is_predicate_getter) | reg_file_predicate;

    // MMU inputs
    mmu_write_en = readFURegOut.is_memwrite & readFURegOut.predicate;
    mmu_write_force = readFURegOut.is_memforce;
    mmu_write_data = readFURegOut.reg2[8:0];
    mmu_addr = readFURegOut.reg1[17:0];

    // Display valid signal
    disp_valid = readFURegOut.is_disp;

    // Int pipeline register 1 inputs
    intResultReg1In.is_mem = readFURegOut.is_mem;
    intResultReg1In.result = readFURegOut.is_cvtfc ? {9'b0, cvtfc_result}
                                                   : int_fu_out;
    intResultReg1In.use_result = readFURegOut.is_int
                               | readFURegOut.is_cvtfc;
    intResultReg1In.thread_num = readFURegOut.thread_num;
    intResultReg1In.rd = readFURegOut.rd;
    // will always be high is not a predicate getter
    // otherwise it will be set to the value of readFURegOut.predicate
    intResultReg1In.predicate = readFURegOut.predicate;
    intResultReg1In.is_predicate_setter = readFURegOut.is_predicate_setter;

    // AddSub -> Write pipeline register inputs
    resultRegsIn[0].regwrite = (intResultReg1Out.use_result
                                 | intResultReg1Out.is_mem | cvtfi_valid)
                               & intResultReg1Out.predicate;
    resultRegsIn[0].thread_num = intResultReg1Out.thread_num;
    resultRegsIn[0].rd = intResultReg1Out.rd;
    // MUX to select what to write back to the register file from AddSub
    resultRegsIn[0].write_data =
        intResultReg1Out.use_result ? intResultReg1Out.result :
        intResultReg1Out.is_mem     ? {9'b0, mmu_read_data}
                                    : cvtfi_result;
    // Only int instructions set predicate in int pipeline
    resultRegsIn[0].set_pred = intResultReg1Out.use_result &
        intResultReg1Out.is_predicate_setter & intResultReg1Out.predicate;
    resultRegsIn[0].new_pred_val = intResultReg1Out.result[0];

    for (int i = 1; i < 9; i++) begin
        resultRegsIn[i] = resultRegsOut[i-1];
    end

    addSubWriteRegIn = resultRegsOut[8];

    regFileWriteIn.rd = addSubWriteRegOut.rd;
    regFileWriteIn.thread_num = addSubWriteRegOut.thread_num;
    if (addSubWriteRegOut.regwrite | addSubWriteRegOut.set_pred) begin
        regFileWriteIn.write_data = addSubWriteRegOut.write_data;
        regFileWriteIn.regwrite = addSubWriteRegOut.regwrite;
        regFileWriteIn.set_pred = addSubWriteRegOut.set_pred;
        regFileWriteIn.new_pred_val = addSubWriteRegOut.new_pred_val;
    end else begin
        regFileWriteIn.write_data = cordicWriteRegOut.write_data;
        regFileWriteIn.regwrite = cordicWriteRegOut.regwrite;
        regFileWriteIn.set_pred = cordicWriteRegOut.set_pred;
        regFileWriteIn.new_pred_val = cordicWriteRegOut.new_pred_val;

    end
end

always_ff @(posedge clk) begin
    readFURegOut <= ~stall ? readFURegIn : readFURegOut;
    intResultReg1Out <= intResultReg1In;
    for (int i = 0; i < 9; i++) begin
        resultRegsOut[i] <= resultRegsIn[i];
    end
    addSubWriteRegOut <= addSubWriteRegIn;
    cordicWriteRegOut <= cordicWriteRegIn;
end

endmodule
