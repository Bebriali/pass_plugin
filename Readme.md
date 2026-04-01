## how to run
### generating IR test file from our main 

```
clang -S -emit-llvm test<i>.c -o test.ll
```
... where i is a test number.

### compiling our project

```
cmake -DCMAKE_BUILD_TYPE=debug -S . -B build && cmake --build build
```

running it on opt and saving to instrumented.ll file for disabling affect on test.ll file

```
opt -load-pass-plugin=./build/DefUsePlugin.so -passes="def-use-plugin" test.ll -o instrumented.ll
```
... where i is a test number.
you should see *.dot file appered in the working directory.

### compiling test program with our runtime logger
```
clang instrumented.ll logger.cpp -o run
```
after running with
```
./run
```
you should see runtime.log appeared in the working directory

### overlaying values
using script on python
```
python3 overlay.py 
```
### viewing diffs in graph
```
dot -Tpng graph.dot -o graph.png
```
or
```
dot -Tpng final.dot -o final.png
```
### full console input
```
cmake -DCMAKE_BUILD_TYPE=debug -S . -B build && cmake --build build && \
opt -load-pass-plugin=./build/DefUsePlugin.so -passes="def-use-plugin" test.ll -o instrumented.ll && \
clang++ instrumented.ll logger.cpp -o run && \
./run && \
python3 overlay.py && \
dot -Tpng final.dot -o final.png
```