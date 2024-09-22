/*
 *  Handles the DISP instruction
 *  Takes colour indexes calculated in cvtfc_fu.sv and uses them
 *  to index a colour spectrum map
 */

module disp_fu (
    input logic [8:0] colour_index,

    output logic [23:0] rgb
);

logic [23:0] bgr;

const logic [23:0] colour_lookup [0:63] = {
    24'h8000ff,
    24'h770dff,
    24'h6f19ff,
    24'h6726fe,
    24'h5f33fe,
    24'h573ffd,
    24'h4f4bfc,
    24'h4757fb,
    24'h3f63fa,
    24'h376ff9,
    24'h2f7af7,
    24'h2685f5,
    24'h1e90f4,
    24'h169af2,
    24'h0ea4f0,
    24'h06aded,
    24'h02b7eb,
    24'h0abfe8,
    24'h12c7e6,
    24'h1acfe3,
    24'h22d6e0,
    24'h2adddd,
    24'h33e3da,
    24'h3be8d6,
    24'h43edd3,
    24'h4bf2cf,
    24'h53f5cb,
    24'h5bf9c7,
    24'h63fbc3,
    24'h6bfdbf,
    24'h73febb,
    24'h7bffb7,
    24'h84ffb2,
    24'h8cfead,
    24'h94fda9,
    24'h9cfba4,
    24'ha4f99f,
    24'hacf59a,
    24'hb4f295,
    24'hbced90,
    24'hc4e88a,
    24'hcce385,
    24'hd4dd80,
    24'hddd67a,
    24'he5cf74,
    24'hedc76f,
    24'hf5bf69,
    24'hfdb763,
    24'hffad5d,
    24'hffa457,
    24'hff9a51,
    24'hff904b,
    24'hff8545,
    24'hff7a3f,
    24'hff6f39,
    24'hff6333,
    24'hff572c,
    24'hff4b26,
    24'hff3f20,
    24'hff3319,
    24'hff2613,
    24'hff190d,
    24'hff0d06,
    24'hff0000
};

logic [5:0] map_index;

always_comb begin
    if (colour_index == 9'd511 || colour_index == 9'd0) begin
        map_index = 6'hx;
        bgr = 24'hffffff;
    end else if (colour_index == 9'd0) begin
        map_index = 6'hx;
        bgr = 24'h8000ff;
    end else begin
        map_index = {colour_index - 9'd1}[8:3];
        bgr = colour_lookup[map_index];
    end
	
	rgb = {bgr[7:0], bgr[15:8], bgr[23:16]};
end

endmodule
