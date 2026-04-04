import sys

ADDR_FLAG = 0x80000000

# Store only the last value for each ID
values = {}
try:
    with open("log/values.log", "r") as f:
        for line in f:
            idx_str, val_str = line.split()
            idx, val = int(idx_str), int(val_str)

            actual_id = idx & ~ADDR_FLAG if bool(idx & ADDR_FLAG) else idx # BUG # FIXME[flops]: Magic constant

            if actual_id not in values:
                values[actual_id] = {'val': '?', 'addr': '?'}

            if bool(idx & ADDR_FLAG):
                values[actual_id]['addr'] = hex(val)
            else:
                values[actual_id]['val'] = str(val)
except FileNotFoundError:
    print("Run the instrumented program first to generate values.log")
    sys.exit(1)

with open("log/dot/graph.dot", "r") as fin, open("log/dot/final.dot", "w") as fout:
    data = fin.read()
    for idx, val in values.items():
        if val['val'] != '?':
            data = data.replace(f"<val_{idx}> ?", val['val'])
        if val['addr'] != '?':
            data = data.replace(f"<addr_{idx}> ?", val['addr'])
    fout.write(data)
