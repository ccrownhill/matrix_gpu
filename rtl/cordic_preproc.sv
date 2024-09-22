`include "pkg/cordic_pkg.svh"
`include "pkg/opcode_pkg.svh"

module cordic_preproc
    import cordic_pkg::*;
    import opcode_pkg::*;
(
    input func5_t func,
    input logic [17:0] op1,
    input logic [17:0] op2,

    output mode_t mode,
    output coord_t coord,
    output logic [12:-4] x_0,
    output logic [12:-4] y_0,
    output logic [12:-4] z_0,
    output logic output_sign,
    output logic [6:0] output_exponent,
    output logic override,
    output logic [17:0] override_val
);

logic op1_sign;
logic [6:0] op1_exponent;
logic [9:0] op1_mantissa;

logic op2_sign;
logic [6:0] op2_exponent;
logic [9:0] op2_mantissa;

logic [16:0] fpga_radian;

/* precedence in override is the following 
NAN > function_override */
logic        function_override;
logic [17:0] function_override_val;

localparam sqrt_constant = 17'h01754;
localparam sincos_constant = 17'h026dd;
localparam negative_nan = 18'h3fe00;

always_comb begin
    // Extract floating point components
    op1_sign = op1[17];
    op1_exponent = op1[16:10];
    op1_mantissa = op1[9:0];

    op2_sign = op2[17];
    op2_exponent = op2[16:10];
    op2_mantissa = op2[9:0];

    fpga_radian = op1[17:1];

    case (func)
    MUL: begin
        mode = ROTATION;
        coord = LINEAR;

        output_sign = op1_sign ^ op2_sign;
        // Correct for the mantissa right-shift and exponent bias
        output_exponent = op1_exponent + op2_exponent - 7'd61;

        function_override = (op1[16:0] == 17'b0 || op2[16:0] == 17'b0);
        function_override_val = 18'b0;

        x_0 = {4'b0001, op1_mantissa, 3'b0};
        y_0 = 17'b0;
        z_0 = {4'b0001, op2_mantissa, 3'b0};
    end
    DIV: begin
        mode = VECTORING;
        coord = LINEAR;

        output_sign = op1_sign ^ op2_sign;
        // Correct for the mantissa right-shift and exponent bias
        output_exponent = op1_exponent - op2_exponent + 7'd64;

        //special case for inf and -inf
        if(op2[16:0] == 17'h00000) begin
            function_override = 1'b1;
            function_override_val = {output_sign, 17'h1fc00};
        end
        else begin
            function_override = 1'b0;
            function_override_val = 18'bx;
        end

        x_0 = {3'b001, op2_mantissa, 4'b0};
        y_0 = {4'b0001, op1_mantissa, 3'b0};
        z_0 = 17'b0;
    end
    SQRT : begin
        mode = VECTORING;
        coord = HYPERBOLIC;

        /* case when exponent is even */
        if(op1_exponent[0] == 0) begin
            x_0 = ({3'b001, op1_mantissa, 4'b0} >> 1) + sqrt_constant;
            y_0 = ({3'b001, op1_mantissa, 4'b0} >> 1) - sqrt_constant;
            output_exponent = {1'b0, op1_exponent[6:1]} + 7'd32;
        end
        /* case when exponent is odd */
        else begin
            x_0 = ({3'b001, op1_mantissa, 4'b0}) + sqrt_constant;
            y_0 = ({3'b001, op1_mantissa, 4'b0}) - sqrt_constant;
            output_exponent = {1'b0, op1_exponent[6:1]} + 7'd32;
        end

        output_sign = 1'b0;

        //special case for negatives
        if(op1_sign) begin
            function_override = 1'b1;
            function_override_val = negative_nan;
        end
        else begin
            function_override = 1'b0;
            function_override_val = 18'bx;
        end

        z_0 = 17'b0;

    end
    CVTFR : begin
        //hardcoded division by 2pi
        //note that: exp(2pi) -> 7'd65 and man(2pi) -> 10'h248
        mode = VECTORING;
        coord = LINEAR;

        output_sign = op1_sign;
        //accounts for bias and exp(2pi) and for the mantissa being right shifted
        output_exponent = op1_exponent - 7'd1;

        function_override = 1'b0;
        function_override_val = 18'bx;

        x_0 = {3'b001, 10'h248, 4'b0};
        y_0 = {4'b0001, op1_mantissa, 3'b0}; //right shifted by 1 to ensure cordic convergence
        z_0 = 17'b0;

    end
    SLL : begin //TODO this is sin
        mode = ROTATION;
        coord = CIRCULAR;

        output_exponent = 7'd63;

        function_override = 1'b0;
        function_override_val = 18'bx;

        x_0 = sincos_constant;
        y_0 = 17'h0;

        case (fpga_radian[16:15])
            /* quadrant 1 - trivial case */
            2'b00: begin
                output_sign = 1'b0;
                z_0 = fpga_radian;
            end
            /* quadrant 2 */
            2'b01: begin
                output_sign = 1'b0;
                z_0 = 17'h10000 - fpga_radian;
            end
            /* quadrant 3 */
            2'b10: begin
                output_sign = 1'b1;
                z_0 = 17'h10000 - fpga_radian;
            end
            /* quadrant 4 - tivial case r*/
            2'b11: begin
                output_sign = 1'b1;
                z_0 = fpga_radian;
            end
            default: begin
                output_sign = 1'b0;
                z_0 = fpga_radian;
            end
        endcase

    end
    SRL : begin //TODO this is cos
        
        mode = ROTATION;
        coord = CIRCULAR;

        output_exponent = 7'd63;

        function_override = 1'b0;
        function_override_val = 18'bx;

        x_0 = sincos_constant;
        y_0 = 17'h0;

        case (fpga_radian[16:15])
            /* quadrant 1 - trivial case */
            2'b00: begin
                output_sign = 1'b0;
                z_0 = fpga_radian;
            end
            /* quadrant 2 */
            2'b01: begin
                output_sign = 1'b1;
                z_0 = 17'h10000 - fpga_radian;
            end
            /* quadrant 3 */
            2'b10: begin
                output_sign = 1'b1;
                z_0 = 17'h10000 - fpga_radian;
            end
            /* quadrant 4 - trivial case */
            2'b11: begin
                output_sign = 1'b0;
                z_0 = fpga_radian;
            end
            default: begin
                output_sign = 1'b0;
                z_0 = fpga_radian;
            end
        endcase
    end

    default: begin
        output_sign = 1'hx;
        output_exponent = 7'hx;
        x_0 = 17'hx;
        y_0 = 17'hx;
        z_0 = 17'hx;
        function_override = 1'hx;
        function_override_val = 18'hx;
        // mode and coord are also don't cares
    end
    endcase

    //if either operand is nan then we override with nan
    if(op1[17:0] == negative_nan || op2[17:0] == negative_nan) begin
        override = 1'b1;
        override_val = negative_nan;
    end
    else begin
        override = function_override;
        override_val = function_override_val;
    end

end

endmodule
