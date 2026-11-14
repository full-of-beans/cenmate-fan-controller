#!/usr/bin/env python3
"""Compare KiCad 10 XML and IPC-D-356 exports with the placed board data."""
import argparse
import json
import re
from collections import Counter, defaultdict
from pathlib import Path
from xml.etree import ElementTree


def parse_board(path):
    stack = [[]]
    for token in re.findall(r'"(?:[^"\\]|\\.)*"|[^\s()"]+|[()]', path.read_text()):
        if token == '(':
            stack.append([])
        elif token == ')':
            node = stack.pop()
            stack[-1].append(node)
        else:
            stack[-1].append(json.loads(token) if token.startswith('"') else token)
    root, = stack[0]
    return root


def children(node, name):
    return [entry for entry in node if isinstance(entry, list) and entry[0] == name]


def value(node, name):
    return next((entry[1] for entry in children(node, name)), '')


def ipc_pads(path):
    aliases = {}
    pads = []
    for line in path.read_text().splitlines():
        if line.startswith('P  NNAME'):
            alias, net = line[8:].strip().split(maxsplit=1)
            aliases[alias] = net
        if line[:3] in {'317', '327', '367'} and line[26] == '-':
            net = line[3:17].strip()
            pads.append((line[20:26].strip(), line[27:31].strip(), aliases.get(net, net).upper()))
    return Counter(pads)


def net_groups(pins):
    groups = defaultdict(set)
    for pin, net in pins.items():
        groups[net].add(pin)
    return {frozenset(group) for group in groups.values()}


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('schematic_xml', type=Path)
    parser.add_argument('board_ipc356', type=Path)
    parser.add_argument('board', type=Path)
    args = parser.parse_args()

    schematic = ElementTree.parse(args.schematic_xml)
    board = parse_board(args.board)
    placed = defaultdict(list)
    for footprint in children(board, 'footprint'):
        fields = {p[1]: p[2] for p in children(footprint, 'property')}
        placed[fields['Reference']].append((footprint, fields))

    components = {c.attrib['ref']: c for c in schematic.findall('./components/comp')}
    schematic_nets = {}
    for net in schematic.findall('./nets/net'):
        for node in net.findall('node'):
            key = (node.attrib['ref'], node.attrib['pin'])
            schematic_nets[key] = net.attrib['name']

    findings = []
    rows = []
    board_only = []
    expected_ipc = Counter()
    actual_nets = {}
    for ref, instances in sorted(placed.items()):
        for footprint, fields in instances:
            attrs = set(next(iter(children(footprint, 'attr')), [])[1:])
            pads = [(p[1], value(p, 'net')) for p in children(footprint, 'pad') if p[1]]
            expected_ipc.update((ref, pin, net.upper()) for pin, net in pads if net)
            if ref not in components:
                if (footprint[1] != 'FanController:MountingHole_2.2mm_M2_Pad_4mm'
                        or not {'board_only', 'exclude_from_bom', 'exclude_from_pos_files'} <= attrs
                        or len(instances) != 1
                        or children(footprint, 'path') or pads != [('1', 'GND')]):
                    findings.append([ref, 'unexpected board-only footprint'])
                board_only.append({'ref': ref, 'position': children(footprint, 'at')[0][1:],
                                   'attributes': sorted(attrs), 'pads': pads})
                continue
            component = components[ref]
            properties = {p.attrib['name']: p.attrib.get('value', '')
                          for p in component.findall('property')}
            metadata = {'Value': component.findtext('value', ''),
                        **{k: properties.get(k, '') for k in ('Manufacturer', 'MPN', 'DigiKey_PN')}}
            for field, expected in metadata.items():
                actual = fields.get(field, '')
                if expected != actual:
                    findings.append([ref, field, expected, actual])
            if footprint[1] != component.findtext('footprint'):
                findings.append([ref, 'footprint', component.findtext('footprint'), footprint[1]])
            for flag in ('exclude_from_bom', 'dnp', 'exclude_from_board'):
                if (flag in properties) != (flag in attrs):
                    findings.append([ref, 'population flag', flag])
            if value(footprint, 'path').rsplit('/', 1)[-1] != component.findtext('tstamps'):
                findings.append([ref, 'symbol link', component.findtext('tstamps'),
                                 value(footprint, 'path')])
            pins = {p.attrib['num'] for p in component.findall('./units/unit/pins/pin')}
            if pins != {pin for pin, _ in pads} or len(pads) != len(pins):
                findings.append([ref, 'pins versus pads', sorted(pins), sorted(pads)])
            actual_nets.update(((ref, pin), net) for pin, net in pads)
            rows.append({'ref': ref, 'footprint': footprint[1], 'metadata': metadata,
                         'pads': dict(sorted(pads)), 'attributes': sorted(attrs)})

    for ref in sorted(components):
        if len(placed[ref]) != 1:
            findings.append([ref, 'footprint count', len(placed[ref])])
    for key in sorted(schematic_nets.keys() | actual_nets.keys()):
        if schematic_nets.get(key) != actual_nets.get(key):
            findings.append([*key, 'net', schematic_nets.get(key), actual_nets.get(key)])
    schematic_groups = net_groups(schematic_nets)
    if schematic_groups != net_groups(actual_nets):
        findings.append(['net membership differs'])

    actual_ipc = ipc_pads(args.board_ipc356)
    for label, difference in [('missing IPC pads', expected_ipc - actual_ipc),
                              ('unexpected IPC pads', actual_ipc - expected_ipc)]:
        if difference:
            findings.append([label, sorted(difference.items())])
    print(json.dumps({'component_count': len(components),
                      'pin_pad_count': len(schematic_nets),
                      'net_count': len(schematic_groups),
                      'components': rows, 'board_only': board_only,
                      'findings': findings}, indent=2, sort_keys=True))
    return bool(findings)


if __name__ == '__main__':
    raise SystemExit(main())
