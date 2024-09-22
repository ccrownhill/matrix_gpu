`ifndef PROGRAM_PKG_SVH
`define PROGRAM_PKG_SVH

package program_pkg;

typedef struct packed {
    logic [9:0] start_addr;
    logic [17:0] num_blocks;
    logic valid;
} program_info;

endpackage

`endif
