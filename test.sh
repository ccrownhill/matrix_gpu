#!/bin/bash

set -e

rm -rf ./test_out/

if (( $# >= 1 )); then
    test_dirs="$*"
else
    test_dirs=$(ls -d ./test/*)
fi


for dir in $test_dirs; do
    if [[ "${dir: -1}" == "/" ]]; then
        dir="${dir::-1}"
    fi
    test_name="${dir##*/}"
    echo "$test_name..."
    out_dir=./test_out/"$test_name"
    mkdir -p "$out_dir"
    # if .m file exists change compile it with compiler
    src_file="$(ls "$dir"/*.m 2>/dev/null || echo "")"
    if [[ -e "$src_file" ]]; then
        cd compiler
        make bin/conv > /dev/null
        cd ..
        #src_basename="${src_file##*/}"
        basename="${src_file%.m}"
        file_no=0;
        rm -f "${src_file%.m}"*.asm
        while read -r line; do
            echo "$line" >> $(printf "${basename}%03d.asm" $file_no)
            [[ "$line" == "exit" ]] && file_no=$((file_no + 1))
        done <<< $(./compiler/bin/conv "$src_file" 2>"$out_dir"/ast_printed.txt)
        chown 1000:1000 "${src_file%.m}"*.asm
    fi
    echo -e "\tGenerate testbench and test script..."
    ./testgen/testgen --outdir="$out_dir" "$dir"/*.yaml
    echo -e "\tRunning test..."
    cd "$out_dir"
    sh *.sh 2>test_stderr.txt 1>test_stdout.txt && \
        echo -e "\t\e[32mSUCCESS\e[0m" || echo -e "\t\e[31mFAIL\e[0m"
    cd - > /dev/null
done
