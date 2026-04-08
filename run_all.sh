#!/usr/bin/bash

GREEN='\033[0;32m'
RED='\033[0;31m'
NC='\033[0m'

# checking input
if [ -z "$1" ]; then
    echo -e "${RED}Usage: $0 <source_file_name> [args_for_run...]${NC}"
    echo -e "Example: $0 test2.c 5 10"
    exit 1
fi

SOURCE_FILE=$1
shift
RUN_ARGS=$@

FILENAME=$(basename -- "$SOURCE_FILE")
NAME="${FILENAME%.*}"

echo -e "${GREEN}>>> Cleaning old artifacts...${NC}"
rm -f run log/values.log log/dot/*.dot log/json/*.json log/pic/*.png prog/*.ll

echo -e "${GREEN}>>> Building plugin...${NC}"
cmake -S . -B build && cmake --build build
if [ $? -ne 0 ]; then echo -e "${RED}Failure building plugin!${NC}"; exit 1; fi

mkdir -p prog

echo -e "${GREEN}>>> Compiling ${SOURCE_FILE} to LLVM IR...${NC}"
clang -S -emit-llvm "tests/${SOURCE_FILE}" -o "prog/${NAME}.ll"
if [ $? -ne 0 ]; then echo -e "${RED}Failure to make IR!${NC}"; exit 1; fi

echo -e "${GREEN}>>> Instrumentation...${NC}"
opt -load-pass-plugin=./build/DefUsePlugin.so -passes="def-use-plugin" "prog/${NAME}.ll" -o "prog/${NAME}_inst.ll"

echo -e "${GREEN}>>> Compiling instrumented binary...${NC}"
clang++ "prog/${NAME}_inst.ll" runtime/logger.cpp -o run

echo -e "${GREEN}>>> Executing program with args: ${RUN_ARGS}${NC}"
./run ${RUN_ARGS}

echo -e "${GREEN}>>> Generating graphs...${NC}"
python3 runtime/overlay_json.py

if [ -f log/pic/final.png ]; then
    echo -e "${GREEN}>>> DONE! Result: log/pic/final.png${NC}"
fi
