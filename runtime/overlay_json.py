import json, graphviz, collections

def load_exec(path):
    data = collections.defaultdict(lambda: {'val': '?', 'addr': '?'})
    try:
        with open(path) as f:
            for p in (line.split() for line in f if line.strip()):
                idx = hex(int(p[0], 0))
                data[idx]['val'] = p[1]
                if len(p) == 3: data[idx]['addr'] = p[2]
    except FileNotFoundError: print(f"Log {path} missing")
    return data

def enrich_and_render(json_in, log_path, json_out, img_out):
    with open(json_in) as f: ir = json.load(f)
    exec_data = load_exec(log_path)
    
    dot = graphviz.Digraph(format='png', graph_attr={'compound':'true', 'rankdir':'TB'})
    dot.attr('node', shape='record', fontname='Courier')

    for fn in ir['functions']:
        with dot.subgraph(name=f"cluster_{fn['name']}", 
                          graph_attr={'label': f"Fn: {fn['name']}", 'style':'filled', 'fillcolor':'white'}) as g:
            for bb in fn['blocks']:
                with g.subgraph(name=f"cluster_{bb['id']}", 
                                graph_attr={'label': f"BB: {bb['id']}", 'fillcolor':'lightpink', 'style':'filled'}) as b:
                    for i in bb['instructions']:
                        d = exec_data[i['id']]
                        
                        if i.get('has_val'):  i['runtime_val'] = d['val']
                        if i.get('has_addr'): i['runtime_addr'] = d['addr']
                        
                        parts = [i['text']]
                        if 'runtime_val' in i:  parts.append(f"VAL: {i['runtime_val']}")
                        if 'runtime_addr' in i: parts.append(f"ADDR: {i['runtime_addr']}")
                        b.node(i['id'], label="{" + " | ".join(parts) + "}")

    for e in ir['edges']:
        dot.edge(e['from'], e['to'], label=e.get('type',''), color=e.get('color','black'),
                 style='dashed' if e.get('color')=='red' else 'solid')

    with open(json_out, "w") as f: json.dump(ir, f, indent=4)
    dot.render(img_out, cleanup=True)
    print(f"Success: JSON -> {json_out}, IMG -> {img_out}.png")

if __name__ == "__main__":
    enrich_and_render(
        "log/json/ir_dump.json", 
        "log/values.log", 
        "log/json/ir_combined.json", 
        "log/pic/final"
    )