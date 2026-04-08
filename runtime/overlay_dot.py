import sys

ADDR_FLAG = 0x80000000

# Store only the last value for each ID
values = {}
try:
    with open("log/values.log", "r") as f:
        for line in f:
            parts = line.split()
            idx = hex(int(parts[0], 0))
            val = parts[1]
            
            addr = hex(int(parts[2], 0)) if len(parts) == 3 else '?'
            
            values[idx] = {'val': val, 'addr': addr}

            if idx not in values:
                values[idx] = {'val': '?', 'addr': '?'}
            
            values[idx]['val'] = val
            if addr != '?':
                values[idx]['addr'] = addr
except FileNotFoundError:
    print("Run the instrumented program first to generate values.log")
    sys.exit(1)

with open("log/dot/graph.dot", "r") as fin, open("log/dot/final.dot", "w") as fout:
    data = fin.read()
    for idx, val in values.items():
        if val['val'] != '?':
            target_val = f"<val_{idx}> ?"
            data = data.replace(target_val, val['val'])
        if val['addr'] != '?':
            target_addr = f"<addr_{idx}> ?"
            data = data.replace(target_addr, val['addr'])
    fout.write(data)
