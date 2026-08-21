#!/usr/bin/env python3
import re
import sys
import os

def generate_docs(dict_file, kit_file, header_file, output_file, inc_file=None):
    with open(kit_file, 'r') as f:
        kit_content = f.read()
    with open(dict_file, 'r') as f:
        dict_content = f.read()
    with open(header_file, 'r') as f:
        header_content = f.read()

    if inc_file:
        with open(inc_file, 'w') as f:
            for match in re.finditer(r'/\*\s*@doc(?:(?:\((.*?)\))?)\n(.*?)\n\s*@enddoc\s*\*/', kit_content, re.DOTALL):
                key = match.group(1) or ""
                body = match.group(2)
                f.write(f'KIT_DOC_BEGIN("{key}", "{key}", "skode.c", 0)\n')
                for line in body.split('\n'):
                    line_escaped = line.replace('"', '\\"') + '\\n'
                    f.write(f'KIT_DOC_LINE("{key}", "{line_escaped}")\n')
                f.write(f'KIT_DOC_END("{key}")\n')

    # Parse skode.h for skode_opcode_t values
    enum_map = {}
    enum_block = re.search(r'typedef enum \{([^}]+)\}\s*skode_opcode_t;', header_content, re.DOTALL)
    if enum_block:
        val = 0
        lines = enum_block.group(1).split(',')
        for line in lines:
            line = line.split('//')[0].strip()
            if not line: continue
            if '=' in line:
                name, v = line.split('=')
                name = name.strip()
                val = int(v.strip())
                enum_map[name] = val
            else:
                name = line.strip()
                enum_map[name] = val
            val += 1

    # Parse skode-dict.c
    dict_map = {}
    dict_blocks = re.split(r'\{\s*WID\(', dict_content)[1:] # Split into blocks starting after WID(
    for block in dict_blocks:
        if '"' not in block: continue
        cmd_name = block.split('"')[1]
        
        props = {'min_args': '0', 'max_args': '0', 'opcode_id': 'None'}
        if '.min_args' in block:
            m = re.search(r'\.min_args\s*=\s*(\w+)', block)
            if m: props['min_args'] = m.group(1)
        if '.max_args' in block:
            m = re.search(r'\.max_args\s*=\s*(\w+)', block)
            if m: props['max_args'] = m.group(1)
        if '.opcode_id' in block:
            m = re.search(r'\.opcode_id\s*=\s*([A-Z0-9_]+)', block)
            if m: props['opcode_id'] = m.group(1)
            
        dict_map[cmd_name] = props

    blocks = re.finditer(r'/\*\s*@doc\((.*?)\)\n(.*?)\n\s*@enddoc\s*\*/', kit_content, re.DOTALL)
    docs = {}
    for match in blocks:
        key = match.group(1).strip()
        body = match.group(2).strip()
        
        entry = {'key': key, 'body': body, 'description': ''}
        for line in body.split('\n'):
            line = line.strip()
            if ':' in line:
                k, v = line.split(':', 1)
                entry[k.strip().lower()] = v.strip()
                
        if key.startswith('command.'):
            cmd = key.replace('command.', '')
            cat = entry.get('category', 'misc')
            
            if cmd in dict_map:
                entry.update(dict_map[cmd])
            
            if cat not in docs: docs[cat] = []
            docs[cat].append(entry)

    long_docs = re.finditer(r'/\*\s*@doc\n(.*?)\n\s*@enddoc\s*\*/', kit_content, re.DOTALL)
    long_doc_map = {}
    for match in long_docs:
        body = match.group(1).strip()
        first_line = body.split('\n')[0].strip()
        if first_line.startswith('`') and '`' in first_line[1:]:
            cmd = first_line.split('`')[1]
            long_doc_map[cmd] = body

    with open(output_file, 'w') as f:
        f.write('# Skred/Pulp Scripting Reference\n\n')
        f.write('This document lists all available commands for `.skred` files loaded via CLAP plugin state, along with their C API mappings for developers.\n\n')
        
        for cat in sorted(docs.keys()):
            f.write(f'## Category: {cat.title()}\n\n')
            for entry in sorted(docs[cat], key=lambda x: x.get('name', '')):
                name = entry.get('name', entry['key'])
                summary = entry.get('summary', '')
                op_id = entry.get('opcode_id', 'None')
                min_args = entry.get('min_args', '0')
                max_args = entry.get('max_args', '0')
                
                op_val_str = f" (Value: {enum_map[op_id]})" if op_id in enum_map else ""
                
                f.write(f'### `{name}`\n\n')
                f.write(f'- **C API Opcode**: `{op_id}`{op_val_str}\n')
                f.write(f'- **Arguments**: `{min_args}` to `{max_args}`\n')
                if summary:
                    f.write(f'- **Summary**: {summary}\n\n')
                    
                if name in long_doc_map:
                    f.write(f'{long_doc_map[name]}\n\n')
                
                f.write('---\n\n')

if __name__ == '__main__':
    if len(sys.argv) < 5:
        print("Usage: gen_docs.py <skode-dict.c> <skode.c> <skode.h> <output.md> [output.inc]")
        sys.exit(1)
    generate_docs(sys.argv[1], sys.argv[2], sys.argv[3], sys.argv[4], sys.argv[5] if len(sys.argv) >= 6 else None)
