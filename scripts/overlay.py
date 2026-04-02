import sys

# Store only the last value for each ID
values = {}
try:
    with open("log/values.log", "r") as f:
        for line in f:
            idx, val = line.split()
            values[idx] = val
except FileNotFoundError:
    print("Run the instrumented program first to generate values.log")
    sys.exit(1)

with open("log/dot/graph.dot", "r") as fin, open("log/dot/final.dot", "w") as fout:
    data = fin.read()
    for idx, val in values.items():
        data = data.replace(f"<val_{idx}> ?", f" {val}")
    fout.write(data)