import json, graphviz, collections, argparse

def load_values(path):
    data = collections.defaultdict(lambda: {'val': '?', 'addr': '?'})
    try:
        with open(path) as f:
            payload = json.load(f)
            values = payload.get('values', payload)
            for entry in values:
                idx = entry.get('id')
                if not idx:
                    continue
                data[idx]['val'] = entry.get('val', '?')
                if 'addr' in entry:
                    data[idx]['addr'] = entry.get('addr', '?')
    except FileNotFoundError:
        print(f"Values file {path} missing")
    except json.JSONDecodeError as exc:
        print(f"Failed to parse values JSON {path}: {exc}")
    return data

def enrich_and_render(json_in, img_out):
    with open(json_in) as f: ir = json.load(f)
    exec_data = load_values(json_in)
    
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

    dot.render(img_out[:-4] if img_out.endswith('.png') else img_out, cleanup=True)
    print(f"Success: IMG -> {img_out}")

if __name__ == "__main__":
    parser = argparse.ArgumentParser(prog='ProgramName')

    parser.add_argument('ir_dump', help='Path to IR dump JSON')
    parser.add_argument('output_png', help='Path to output PNG')

    args = parser.parse_args()
    enrich_and_render(
        args.ir_dump,
        args.output_png
    )