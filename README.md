# matrix_gpu - GPU+Compiler with Custom Linear Algebra Language for FPGA

![logo](./logo.png)

Started as a group project for Imperial College London's year 2 end of year project
with Ryan Voecks, Petr Olšan, Orlan Forshaw, Zian Lin, and Gabriel Tetrault.

## Original Plotting Compiler

I was responsible for part of the GPU design in System Verilog and mainly the
whole design of the Compiler.
Originally, the compiler just generated GPU machine code to plot math expressions
like these on the screen:

```
2.3sin(x/y) + 2.3cos(x/y)
```

## Linear Algebra Language Compiler

Then I came up with a programming language designed for linear algebra operations
and adapted the compiler to output GPU machine code to execute matrix operations
heavily parallelised.

Example code for dot product:

```
$z = |2,1|[1., 2.]
$m = |1,2|[1., 2.] dot $z
$mc = |1,1|[5.]
$first = |3,1|[1.,2.,3.]
$second = |3,1|[1., 2., 3.]
$final = $first + $second
$finalCheck = |3,1|[1.,4.,6.]
$beforeRelu = |2,2|[-3.0, -5.5, 3., 8.]
$afterRelu = relu(-$beforeRelu)
$reluCheck = |2,2|[3.0, 5.5, 0., 0.]
.plot $m 0.0 10.
.plot $mc 0.0 10.0
.plot $final 0.0 10.0
.plot $finalCheck 0.0 10.0
.plot $beforeRelu 0.0 10.0
.plot $afterRelu 0.0 10.0
.plot $reluCheck 0.0 10.0
```

where `.plot $someVarName min max` will plot the array as a rectangular heatmap
(they look something like the logo above) with `min` the coldest value and `max`
the hottest value of the heatmap.

## Garbage Collection

To store matrices multi-dimensional arrays in memory the compiler outputs code
to allocate and populate the according memory.
It also uses an automatic garbage collection mechanism to reuse memory of variables
that are not used anymore.

## Testing in Simulation

### Setting Up New Tests

The Go program testgen.go was used to generate C++ testbenches for Verilator as well as
scripts to compile the RTL with Verilator along with the testbench. To do this testgen.go used
template files for the script and the testbench and then used the text/template package to fill
them with values from the YAML file.
Moreover, the YAML file can specify assembly files to be used which testgen.go will assemble
(after first recompiling the assembler), and then place correctly inside the testbench to send
them to the GPU interface in different cycles among the correct control instructions. It is even
possible to just specify math equations in a file in the YAML’s directory which will be
automatically compiled before being assembled.
In the top-level directory of the repository a Bash script automatically runs all the tests in the
directory test where each test is one directory containing at least a YAML file, and, optionally,
math expression files or assembly. It generates the corresponding testbenches in a different
directory along with all error messages and runs them.

See example tests in the `test` directory.

### Running Tests

Build the docker container: (need to be inside repo top level directory)

```
docker build -t epic_accelerator .
```

To run all tests in directory `test` run this in the container:

```
./test.sh
```

Or to just run the test `sampletest` in the `test` directory run:

```
./test.sh test/sampletes
```

(Can specify any number of tests as arguments.)
