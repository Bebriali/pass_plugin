import sys

# Store only the last value for each ID
values = {}
try:
    with open("values.log", "r") as f:
        for line in f:
            idx, val = line.split()
            values[idx] = val
except FileNotFoundError:
    print("Run the instrumented program first to generate runtime.log")
    sys.exit(1)

with open("graph.dot", "r") as fin, open("final.dot", "w") as fout:
    data = fin.read()
    for idx, val in values.items():
        data = data.replace(f"<val_{idx}> ?", f" {val}")
    fout.write(data)