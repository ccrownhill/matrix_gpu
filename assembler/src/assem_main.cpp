#include <iostream>
#include <fstream>
#include <string>
#include "util.hpp"
#include "assembler.hpp"

// TODO implement -i -o command line options
// Requires two arguments:
// First argument: assembly filename
// Second argument: binary filename
int main(int argc, char **argv) {
    Assembler assembler;
    bool has_input_file = false;
    bool has_output_file = false;
    std::ifstream asm_stream;
    std::ofstream out_stream;
    std::string format = "bin";
    try{
        for(int i = 0; i < argc; i++) {
            if (argv[i] == std::string("-i")) {
                has_input_file = true;
                asm_stream.open(argv[i+1]);
            }
            else if (argv[i] == std::string("-o")) {
                has_output_file = true;
                out_stream.open(argv[i+1], std::ios::binary);
            }
            else if (argv[i] == std::string("-f")) {
                if(argv[i+1] == std::string("hex")) {
                    format = "hex";
                }
                else if(argv[i+1] == std::string("bin")) {
                    format = "bin";
                }
                else {
                    std::cerr << "Invalid format. Please enter 'hex' or 'bin'." << std::endl;
                    std::exit(1);
                }
            }
            else if (argv[i] == std::string("-h")) {
                std::cout << "Usage: " << argv[0] << " [-i <asm file>] [-o <bin file>] [-f <format>]" << std::endl; 
            }

        }
    }
    catch(...) {
        std::cerr << "Invalid command line arguments. Please enter '-h' for help." << std::endl;
        std::exit(1);
    }
    if(has_input_file && has_output_file) {
            assembler.assemble(asm_stream, out_stream, format);
            asm_stream.close();
            out_stream.close();
    }
    else if(has_input_file) {
        assembler.assemble(asm_stream, std::cout, format);
        asm_stream.close();
    }
    else if(has_output_file) {
        assembler.assemble(std::cin, out_stream, format);
        out_stream.close();
    }
    else {
        assembler.assemble(std::cin, std::cout, format);
    }

    
}
