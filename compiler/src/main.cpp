#include <iostream>
#include <cmath>
#include <utility>
#include <memory>
#include <sstream>
#include <string>
#include <cstdint>
#include <fstream>
#include <filesystem>

#include "context.hpp"
#include "ast.hpp"
#include "lex.hpp"
#include "constants.hpp"
#include "codegen.hpp"
#include "parser.hpp"


int main(int argc, char *argv[])
{
    const char *usage =
        "Usage: conv [-s|--single-out] [-o/--out output file] [input asm file]";
    bool useStdout = true;
    bool useStdin = true;
    std::streambuf *coutBak = std::cout.rdbuf();
    std::streambuf *cinBak = std::cin.rdbuf();
    std::ifstream in;
    std::ofstream out;
    bool singleOut = false;
    for (int i = 1; i < argc; i++) {
        if (argv[i] == std::string("-o")
                || argv[i] == std::string("--out")) {
            if (i < argc - 1) {
                useStdout = false;
                out.open(argv[i+1]);
                std::cout.rdbuf(out.rdbuf());
                i++;
            } else {
                std::cerr << usage << std::endl;
                std::exit(1);
            }
        } else if (argv[i] == std::string("-s")
                || argv[i] == std::string("--single-out")) {
            singleOut = true;
        } else {
            in.open(argv[i]);
            std::cin.rdbuf(in.rdbuf());
            useStdin = false;
        }
    }

    std::string inStr = "";
    for (std::string line; std::getline(std::cin, line, '\n');) {
        inStr += line;
    }
    std::istringstream inStream {inStr};

    std::shared_ptr<ASTNode> astRoot = parse::Parse(inStream);



    PrintVisitor *pvisitor = new PrintVisitor(std::cerr);
    astRoot->Accept(pvisitor);
    delete pvisitor;

    std::shared_ptr<CodeGen> codeGen = std::make_shared<CodeGen>();
    codeGen->singleOut = singleOut;

    if (!codeGen->singleOut) {
        // PROGRAM to reset frame buffer to make it all 0
        codeGen->ResetMem(std::cout);
    }

    ASMGenVisitor *avisitor = new ASMGenVisitor(codeGen, std::cout);
    astRoot->Accept(avisitor);
    delete avisitor;

    if (!useStdout) {
        out.close();
    }

    if (!useStdin) {
        in.close();
    }
    std::cout.rdbuf(coutBak);
    std::cin.rdbuf(cinBak);
}
