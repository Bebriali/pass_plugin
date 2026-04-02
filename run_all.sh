#!/bin/bash

GREEN='\033[0;32m'
RED='\033[0;31m'
NC='\033[0m'

echo -e "${GREEN}>>> clear old logs and exec files...${NC}"
rm -f run log/values.log log/runtime_values.log log/dot/graph.dot log/dot/final.dot log/pic/graph.png log/pic/final.png run prog/instrumented.ll

echo -e "${GREEN}>>> building plugin (CMake)...${NC}"
cmake -DCMAKE_BUILD_TYPE=debug -S . -B build && cmake --build build
if [ $? -ne 0 ]; then echo -e "${RED}Failure building plugin!${NC}"; exit 1; fi

echo -e "${GREEN}>>> Making dir prog/...${NC}"
mkdir -p prog
if [ $? -ne 0 ]; then echo -e "${RED}Failure to build prog/ folder!${NC}"; exit 1; fi

echo -e "${GREEN}>>> Making *.ll test file...${NC}"
clang -S -emit-llvm tests/test2.c -o prog/test.ll
if [ $? -ne 0 ]; then echo -e "${RED}Failure to make *.ll test file!${NC}"; exit 1; fi

echo -e "${GREEN}>>> Instrumentation (opt)...${NC}"
opt -load-pass-plugin=./build/DefUsePlugin.so -passes="def-use-plugin" prog/test.ll -o prog/instrumented.ll
if [ $? -ne 0 ]; then echo -e "${RED}Failure of LLVM Instrumentation!${NC}"; exit 1; fi

echo -e "${GREEN}>>> Compiling test program...${NC}"
clang++ prog/instrumented.ll scripts/logger.cpp -o run
if [ $? -ne 0 ]; then echo -e "${RED}Failure to compile to run!${NC}"; exit 1; fi

echo -e "${GREEN}>>> program execution (filling values.log)...${NC}"
./run

echo -e "${GREEN}>>> Generating no-value graph (Python + Dot)...${NC}"
python3 scripts/overlay.py && dot -Tpng log/dot/graph.dot -o log/pic/graph.png
echo -e "${GREEN}>>> Generating final graph (Python + Dot)...${NC}"
python3 scripts/overlay.py && dot -Tpng log/dot/final.dot -o log/pic/final.png

if [ -f log/pic/final.png ]; then
    echo -e "${GREEN}>>> DONE! result is in final.png${NC}"
else
    echo -e "${RED}>>> Failure in creating the final image.${NC}"
fi