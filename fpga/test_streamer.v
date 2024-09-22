module test_streamer (
    input           aclk,
    input           aresetn,
    
    output [255:0]  out_stream_tdata,
    output [31:0]    out_stream_tkeep,
    output          out_stream_tlast,
    input           out_stream_tready,
    output          out_stream_tvalid,
    output [0:0]    out_stream_tuser,
    
    input  [31:0]   from_cpu,   // GPIO 0
    output [1:0]    to_cpu      // GPIO 1
);

reg [31:0] from_cpu_reg;

localparam X_SIZE = 1024;
localparam Y_SIZE = 576;

reg [10:0] x;
reg [9:0] y;

wire ready;
wire disp_valid;
wire gpu_reset_frame; // Active-high frame reset signal from GPU

wire reset_frame;   // Active-low frame reset signal sent to pixel packer
assign reset_frame = aresetn & ~gpu_reset_frame;

wire first = (x == 0) & (y==0);
wire lastx = (x == X_SIZE - 8);
wire lasty = (y == Y_SIZE - 1);

always @(posedge aclk) begin
    if (reset_frame) begin
        if (ready & valid) begin
            if (lastx) begin
                x <= 11'd0;
                if (lasty) begin
                    y <= 10'd0;
                end
                else begin
                    y <= y + 10'd1;
                end
            end
            else x <= x + 11'd8;
        end
    end
    else begin
        x <= 0;
        y <= 0;
    end
    from_cpu_reg <= from_cpu;
end

wire valid;
wire [191:0] rgb_vals;

// disp_valid is array of valid bits from all disp units
assign valid = |disp_valid;

simd_processor SIMDProcessor (
    .clk(aclk),
    .vdma_ready(ready),
    .from_cpu(from_cpu_reg),

    .disp_valid(disp_valid),
    .rgb_out(rgb_vals),
    .reset_frame(gpu_reset_frame),
    .to_cpu(to_cpu)
);

packer PixelPacker (
    .aclk(aclk),
    .aresetn(reset_frame),
    .rgb_in(rgb_vals),
    .eol(lastx), .in_stream_ready(ready), .valid(valid), .sof(first),
    .out_stream_tdata(out_stream_tdata), .out_stream_tkeep(out_stream_tkeep),
    .out_stream_tlast(out_stream_tlast), .out_stream_tready(out_stream_tready),
    .out_stream_tvalid(out_stream_tvalid), .out_stream_tuser(out_stream_tuser)
);

endmodule
