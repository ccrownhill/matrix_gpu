`ifndef PIPELINE_REG_PKG_SVH
`define PIPELINE_REG_PKG_SVH

package pipeline_reg_pkg;

typedef struct packed {
    logic [3:0] thread_num;
    logic [17:0] block_idx;
    logic [31:0] instr;
} fetch_decode_reg;

// Output register of decode shared by all lanes
typedef struct packed {
    logic [3:0] thread_num;
    logic [17:0] block_idx;
    logic [4:0] func5;
    logic [17:0] imm;
    logic [3:0] rs1;
    logic [3:0] rs2;
    logic [3:0] rd;
    logic is_int;
    logic is_imm;
    logic is_fadd;
    logic is_cordic;
    logic is_mem;
    logic is_memwrite;
    logic is_memforce;
    logic is_cvtfc;
    logic is_disp;
    logic is_predicate_setter;
    logic is_predicate_getter;
    logic exit;
} decode_lanes_reg;

typedef struct packed {
    logic [3:0] thread_num;
    logic [4:0] func5;
    logic [17:0] imm;
    logic [17:0] reg1;
    logic [17:0] reg2;
    logic [3:0] rd;
    logic is_int;
    logic is_imm;
    logic is_fadd;
    logic is_cordic;
    logic is_mem;
    logic is_memwrite;
    logic is_memforce;
    logic is_cvtfc;
    logic is_disp;
    logic predicate;
    logic is_predicate_setter;
} read_fu_reg;

// Extra registers to increase int pipeline length
typedef struct packed {
    logic [3:0] thread_num;
    logic [17:0] result;
    logic [3:0] rd;
    logic is_mem;
    logic use_result;
    logic predicate;
    logic is_predicate_setter;
} int_result_reg;

typedef struct packed {
    logic [3:0] thread_num;
    logic [17:0] write_data;
    logic regwrite;
    logic [3:0] rd;
    logic set_pred;
    logic new_pred_val;
} int_write_reg;

typedef struct packed {
    logic [17:0] write_data;
    logic regwrite;
    logic set_pred;
    logic new_pred_val;
} fp_write_reg;

endpackage

`endif
