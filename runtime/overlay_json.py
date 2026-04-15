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

def enrich_and_render_defuse(json_in, img_out):
    with open(json_in) as f: ir = json.load(f)
    exec_data = load_values(json_in)
    
    defuse_graph = graphviz.Digraph(format='png', graph_attr={'compound':'true', 'rankdir':'TB'})
    defuse_graph.attr('node', shape='box', fontname='Courier')

    

    for fn in ir['functions']:
        with defuse_graph.subgraph(name=f"cluster_{fn['name']}", 
                          graph_attr={'label': f"Fn: {fn['name']}", 'style':'filled', 'fillcolor':'white'}) as g:
            for bb in fn['blocks']:
                with g.subgraph(name=f"cluster_{bb['id']}", 
                                graph_attr={'label': f"BB: {bb['id']}", 'fillcolor':'lightpink', 'style':'filled'}) as b:
                    for i in bb['instructions']:
                        parts = [i['text']]
                        if 'runtime_val' in i:  parts.append(f"VAL: {i['runtime_val']}")
                        if 'runtime_addr' in i: parts.append(f"ADDR: {i['runtime_addr']}")
                        
                        clean_parts = []
                        for p in parts:
                            p = str(p).replace('{', '\{').replace('}', '\}').replace('|', '\|')
                            p = p.replace('<', '\<').replace('>', '\>')
                            clean_parts.append(p)

                        node_label = "{" + " | ".join(clean_parts) + "}"
                        b.node(i['id'], label=node_label)

    for e in ir['edges']:
        is_backward = e.get('color') == 'red' or e.get('type') == 'cfg-link'
        defuse_graph.edge(e['from'], e['to'], label=e.get('type',''), color=e.get('color','black'),
                 style='dashed' if e.get('color')=='red' else 'solid', 
                 constraint='false' if is_backward else 'true')

    defuse_graph.render(img_out[:-4] if img_out.endswith('.png') else img_out, cleanup=True)
    print(f"Success: IMG -> {img_out}")

def enrich_and_render_call(json_in, img_out):
    with open(json_in) as f: 
        ir = json.load(f)
    exec_data = load_values(json_in)

    call_graph = graphviz.Digraph(format='png', graph_attr={'label': 'Call Graph', 'rankdir': 'LR'})
    call_graph.attr('node', shape='box', style='rounded,filled', fillcolor='lightblue', fontname='Courier')

    for fn in ir['functions']:
        call_graph.node(fn['name'], label=f"Function: {fn['name']}")

    for edge in ir.get('call_edges', []):
        inst_id = int(edge['id'], 16)
        
        args = []
        for i in range(1, 5):
            arg_data = exec_data.get(f"0x{inst_id + i:x}")
            if arg_data and arg_data['val'] != '?':
                args.append(f"arg{i-1}: {arg_data['val']}")
        
        ret_data = exec_data.get(edge['id'])
        ret_val = ret_data['val'] if ret_data else '?'

        label = f"ID: {edge['id']}\n"
        if args: label += "IN: (" + ", ".join(args) + ")\n"
        label += f"RET: {ret_val}"
        
        call_graph.edge(edge['caller'], edge['callee'], label=label)

    output_path = img_out[:-4] if img_out.endswith('.png') else img_out
    call_graph.render(output_path, cleanup=True)
    print(f"Success: Call Graph -> {img_out}")

if __name__ == "__main__":
    parser = argparse.ArgumentParser(prog='ProgramName')

    parser.add_argument('defuse_dump', help='Path to defuse dump JSON')
    parser.add_argument('defuse_png', help='Path to output defuse PNG')
    parser.add_argument('call_dump', help='Path to call dump JSON')
    parser.add_argument('call_png', help='Path to output call PNG')

    args = parser.parse_args()

    enrich_and_render_defuse(
        args.defuse_dump,
        args.defuse_png
    )
    enrich_and_render_call(
        args.call_dump,
        args.call_png
    )