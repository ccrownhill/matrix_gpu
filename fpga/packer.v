module packer(

input           aclk,
input           aresetn,

input [191:0]   rgb_in,
input           eol,
output          in_stream_ready,
input           valid,
input           sof, 

output [255:0]  out_stream_tdata,
output [31:0]   out_stream_tkeep,
output          out_stream_tlast,
input           out_stream_tready,
output          out_stream_tvalid,
output [0:0]    out_stream_tuser );


reg [1:0]       state_reg = 2'b0;

//Act instantly in state 0 if input pixel is start of frame
wire [1:0]      state = sof ? 2'b00 : state_reg;
wire            state0 = (state == 2'b0);

reg             sof_reg;
reg [191:0]     prev_rgb;

//Combinational
reg [255:0]     tdata;
reg             tvalid;
reg             ready;


always @(posedge aclk)begin
    if(aresetn) begin
        //Advance state if valid and...
        if (valid) begin
            //...if in state 0 or destination is ready
            if (state0 | out_stream_tready) begin
                //Always return to state zero at end of line
                if (eol) begin
                    state_reg <= 2'b0;
                end
                else begin
                    state_reg <= state + 2'b1;
                end
            end

            // Store the sof flag when it is set (it can't be read in this cycle because output data isn't ready)
            if (sof) begin
                sof_reg <= 1'b1;
            end
            // Reset it after it has been read
            else if (valid & out_stream_tready) begin
                sof_reg <= 1'b0;
            end

            //Latch colour inputs whenever they are valid
            prev_rgb <= rgb_in;
        end
    end
    else begin
        state_reg <= 2'b0;
        sof_reg <= 1'b0;
    end
end

integer i;

always @* begin
    case ({state})
        2'b00 : 
            begin 
                //Output is not complete (valid) in this state, that means we are always ready for the next pixel.
                //tdata: don't care since valid is false
                tdata = 256'bx;
                tvalid = 1'b0;
                ready = 1'b1;
            end
        2'b01 :
            begin 
                // Couldn't find a more concise way of doing the wire assignments
                // Send gr from current pixel 2, all of currents pixels 1:0, and previous pixels 7:0
                tdata = {
                    rgb_in[63:56], rgb_in[71:64],
                    rgb_in[31:24], rgb_in[39:32], rgb_in[47:40],
                    rgb_in[ 7: 0], rgb_in[15: 8], rgb_in[23:16],
                    prev_rgb[175:168], prev_rgb[183:176], prev_rgb[191:184],
                    prev_rgb[151:144], prev_rgb[159:152], prev_rgb[167:160],
                    prev_rgb[127:120], prev_rgb[135:128], prev_rgb[143:136],
                    prev_rgb[103: 96], prev_rgb[111:104], prev_rgb[119:112],
                    prev_rgb[ 79: 72], prev_rgb[ 87: 80], prev_rgb[ 95: 88],
                    prev_rgb[ 55: 48], prev_rgb[ 63: 56], prev_rgb[ 71: 64],
                    prev_rgb[ 31: 24], prev_rgb[ 39: 32], prev_rgb[ 47: 40],
                    prev_rgb[  7:  0], prev_rgb[ 15:  8], prev_rgb[ 23: 16]
                };
                
                tvalid = valid;
                ready = out_stream_tready;
            end
        2'b10 : 
            begin 
                // Send r from current pixel 5, all of currents pixels 4:0,
                // previous pixels 7:3, and b from previous pixel 2
                tdata = {
                    rgb_in[143:136],
                    rgb_in[103: 96], rgb_in[111:104], rgb_in[119:112],
                    rgb_in[ 79: 72], rgb_in[ 87: 80], rgb_in[ 95: 88],
                    rgb_in[ 55: 48], rgb_in[ 63: 56], rgb_in[ 71: 64],
                    rgb_in[ 31: 24], rgb_in[ 39: 32], rgb_in[ 47: 40],
                    rgb_in[  7:  0], rgb_in[ 15:  8], rgb_in[ 23: 16],          
                    prev_rgb[175:168], prev_rgb[183:176], prev_rgb[191:184],
                    prev_rgb[151:144], prev_rgb[159:152], prev_rgb[167:160],
                    prev_rgb[127:120], prev_rgb[135:128], prev_rgb[143:136],
                    prev_rgb[103: 96], prev_rgb[111:104], prev_rgb[119:112],
                    prev_rgb[ 79: 72], prev_rgb[ 87: 80], prev_rgb[ 95: 88],                           
                    prev_rgb[55:48]
                };

                tvalid = valid;
                ready = out_stream_tready;
            end
        2'b11 : 
            begin 
                // Send current pixels 7:0, previous pixels 7:6 and bg from previous pixel 5 
                tdata = {
                    rgb_in[175:168], rgb_in[183:176], rgb_in[191:184],
                    rgb_in[151:144], rgb_in[159:152], rgb_in[167:160],
                    rgb_in[127:120], rgb_in[135:128], rgb_in[143:136],
                    rgb_in[103: 96], rgb_in[111:104], rgb_in[119:112],
                    rgb_in[ 79: 72], rgb_in[ 87: 80], rgb_in[ 95: 88],
                    rgb_in[ 55: 48], rgb_in[ 63: 56], rgb_in[ 71: 64],
                    rgb_in[ 31: 24], rgb_in[ 39: 32], rgb_in[ 47: 40],
                    rgb_in[  7:  0], rgb_in[ 15:  8], rgb_in[ 23: 16],          
                    prev_rgb[175:168], prev_rgb[183:176], prev_rgb[191:184],                
                    prev_rgb[151:144], prev_rgb[159:152], prev_rgb[167:160],
                    prev_rgb[127:120], prev_rgb[135:128]      
                };

                tvalid = valid;
                ready = out_stream_tready;
            end
        default : 
            begin 
                //Output is not complete (valid) in this state, that means we are always ready for the next pixel.
                //tdata: don't care since valid is false
                tdata = 256'bx;
                tvalid = 1'b0;
                ready = 1'b1;
            end
    endcase
end

assign in_stream_ready = ready;
assign out_stream_tlast = eol; //Assuming that end of line is never in state zero
assign out_stream_tuser = sof_reg;
assign out_stream_tkeep = 8'hffffffff; //Assuming that line contains a multiple of 8 bytes.
assign out_stream_tdata = tdata;
assign out_stream_tvalid = tvalid;

endmodule