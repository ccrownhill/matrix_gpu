`ifndef MEMORY_PKG_SVH
`define MEMORY_PKG_SVH

package memory_pkg;

typedef struct packed {
    logic en;
    logic forcewrite;
    logic [8:0] data;
    logic [17:0] addr;
} write_req_pkt;

endpackage

`endif
