#!/bin/bash

GREEN='\033[0;32m'
RED='\033[0;31m'
NC='\033[0m'

# 1. Проверка входных данных
if [ -z "$1" ]; then
    echo -e "${RED}Usage: $0 <source_file_name> [args_for_run...]${NC}"
    echo -e "Example: $0 test2.c 5 10"
    exit 1
fi

SOURCE_FILE=$1
# Сдвигаем аргументы, чтобы в $@ остались только параметры для запуска программы
shift
RUN_ARGS=$@

# Извлекаем имя без расширения для путей (например, test2)
FILENAME=$(basename -- "$SOURCE_FILE")
NAME="${FILENAME%.*}"

echo -e "${GREEN}>>> Cleaning old artifacts...${NC}"
rm -f run log/values.log log/dot/*.dot log/pic/*.png prog/*.ll

echo -e "${GREEN}>>> Building plugin...${NC}"
cmake -S . -B build && cmake --build build
if [ $? -ne 0 ]; then echo -e "${RED}Failure building plugin!${NC}"; exit 1; fi

mkdir -p prog

echo -e "${GREEN}>>> Compiling ${SOURCE_FILE} to LLVM IR...${NC}"
# Используем переменную SOURCE_FILE вместо хардкода
clang -S -emit-llvm "tests/${SOURCE_FILE}" -o "prog/${NAME}.ll"
if [ $? -ne 0 ]; then echo -e "${RED}Failure to make IR!${NC}"; exit 1; fi

echo -e "${GREEN}>>> Instrumentation...${NC}"
opt -load-pass-plugin=./build/DefUsePlugin.so -passes="def-use-plugin" "prog/${NAME}.ll" -o "prog/${NAME}_inst.ll"

echo -e "${GREEN}>>> Compiling instrumented binary...${NC}"
clang++ "prog/${NAME}_inst.ll" scripts/logger.cpp -o run

echo -e "${GREEN}>>> Executing program with args: ${RUN_ARGS}${NC}"
# Запускаем программу. Она подхватит аргументы командной строки.
# Если нужен интерактивный ввод (scanf), он просто будет ждать в консоли.
./run ${RUN_ARGS}

echo -e "${GREEN}>>> Generating graphs...${NC}"
python3 scripts/overlay.py
dot -Tpng log/dot/final.dot -o log/pic/final.png

if [ -f log/pic/final.png ]; then
    echo -e "${GREEN}>>> DONE! Result: log/pic/final.png${NC}"
fi