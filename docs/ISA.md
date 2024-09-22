# EpicGPU ISA

## Bits Encoding

### Predication

All instructions will be performed by checking the predicate if Instr[28] is set.

### R-Type

***Opcode*** [31:29] := [000]

***DONT_CARE*** [28:20] := [9(x)]

***RS2*** [19:15]

***Func5*** [14:10] ::

* 	add := [00000]
* 	sub := [00001]
* 	mul := [00010]
* 	(mac := [00011])
* 	div := [00100]
* 	rem := [00101]

***Unique encoding***

* 	abs := [000, 14(x), 00110, RS1, RD] // no
* 	and := [00111]
* 	or  := [01000] // no
* 	xor := [01001] // no
* 	sll := [01010]
* 	srl := [01011]
* 	sra := [01100] // no
* 	min := [11000] // no
* 	minu := [11001] // no
* 	max := [11010] // no
* 	maxu := [11011] // no
* 	slt := [11100]
* 	seq := [11101]

***RS1*** [9:5]

***RD*** [4:0]

### I-Type

***Opcode*** [31:29] := [001]

13 bit:

***Imm14*** [27:15] := [] All sign extended from MSB

***Func5*** [14:10] :=

* 	addi := [00000]
* 	subi := [00001]
* 	muli := [00010]
* 	(maci := [00011])
* 	divi := [00100]
* 	remi := [00101]
* 	andi := [00111]
* 	<!-- ori  := [01000]
* 	xori := [01001] -->
* 	slli := [01010]
* 	srli := [01011]
* 	<!-- srai := [01100] -->

* 	<!-- mini := [11000]
* 	miniu := [11001]
* 	maxi := [11010]
* 	maxiu := [11011] -->
* 	slti := [11100]
* 	seqi := [11101]


***RS1*** [9:5]
\
***RD*** [4:0]\


### U-Type
***Opcode*** [31:29] := [010]

* lui := [010, Imm(23:11), 11111, Imm(10:6), RD] <--- **new 18 bit encoding**
* lui := [010, Imm(23:10), 11111, Imm(9:5), RD] <--- **old 19 bit encoding**
* li  := [010, Imm(18:5),  11111, Imm(4:0), RD] **now deprecated**



### F-Type
***Opcode*** [31:29] := [011]

***Func5*** [14:10] ::

* fadd := [00000]
* fsub := [00001]
* fmul := [00010]
* (fmac := [00011])
* fdiv := [00100]
* fabs := [00110]
<!-- * frcp := [00111] -->
* fsqrt := [01000]
<!-- * frsqrt := [01001] -->
* fsin := [01010]
* fcos := [01011]
* flog := [01100]
* fexp := [01101]

<!-- * fmin := [11000]
* fmax := [11010] -->
* fslt := [11100]
* fseq := [11101]
* cvtif := [10000]
* cvtfi := [10001]
* cvtfr := [10010]
* cvtfc := [10011]

### M-Type

***Opcode*** [31:29] := [100]

***Instructions to access the frame buffer***

* lw := [100, 17(x), 00, RS1, RD]

    * `lw rd, (rs1)`

* sw := [100, 9(x), RS2, 3(x), 01, RS1, 00000]

    * `sw rs2, (rs1)`

* sz := [100, 9(x), RS2, 3(x), 11, RS1, 00000]

    * `sz rs2, (rs1)`

### X-Type

TODO: CHANGE THIS OPC

***Opcode*** [31:29] := [101]
* xstart := [101, 00, program_num(2:0), imm(23:0)]
* xinstr := [101, 01, x(27)]
* xend   := [101, 10, x(27)]

### D-Type
***Opcode*** [31:29] := [110]
 	disp := [110, x(19), RS1, x(5)]

### C-Type
***Opcode*** [31:29] := [111]\
\

	jump := [111, Imm[27:12], 000, Imm[11:2]]
	branch := [111, Imm[27:12], 001, Imm[11:2]]
	call := [111, Imm[17:2], 010, RS1, RD]
	`call rd, imm(rs1)`
	ret := [111, x(16), 011, RS1, x(5)]
	`ret rd`
	sync := [111, x(16), 110, x(10)]
	exit := [111, x(16), 111, x(10)]
\


### P-Type
***PseudoInstructions***
\
    li rd, value resolves to :=
        lui  rd, value[31:14]
        addi rd, rd, value[13:0]


\

## Register file parameters
Number of registers per register file: 32
R0 is a read-only register that has constant value 0
R1 is the read/write %BlockIdx register that has a value which is set for each kernel
R2 is a read-only %BlockDim register that has a constant value equal to the number of SIMD lanes per compute core
R3 is a read-only %ThreadIdx register that has a constant value identifying which SIMD lane the register file is in
