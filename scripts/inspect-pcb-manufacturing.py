#!/usr/bin/env python3
"""Measure the CENMATE KiCad 10 CAM exports in mm using Gerbonara and Shapely."""

import argparse
import re
import runpy
from collections import Counter
from itertools import combinations, groupby
from math import acos, cos, degrees, dist, hypot, radians, sin
from functools import lru_cache
from pathlib import Path

from gerbonara import ExcellonFile, GerberFile
from gerbonara import graphic_primitives as gp
from gerbonara.utils import MM, approximate_arc
from shapely.geometry import LineString, Point, Polygon
from shapely.ops import polygonize_full, unary_union


def centerline(primitive):
    start = primitive.x1, primitive.y1
    end = primitive.x2, primitive.y2
    if isinstance(primitive, gp.Arc):
        points = list(approximate_arc(
            primitive.cx, primitive.cy, *start, *end, primitive.clockwise,
            max_error=0.00005, clip_max_error=False,
        ))
        return LineString([start, *points[1:-1], end])
    return LineString([start, end])


def geometry(primitive):
    if isinstance(primitive, gp.Circle):
        return Point(primitive.x, primitive.y).buffer(primitive.r, quad_segs=48)
    if isinstance(primitive, (gp.Line, gp.Arc)):
        return centerline(primitive).buffer(primitive.width / 2, quad_segs=48)
    polygon = Polygon(primitive.to_arc_poly().approximate_arcs(
        max_error=0.00005, clip_max_error=False,
    ).outline)
    # KiCad zones use a self-touching outline with linked hole contours.
    return polygon if polygon.is_valid else polygon.buffer(0)


def painted(primitives):
    result = Polygon()
    for dark, run in groupby(primitives, key=lambda p: p.polarity_dark):
        shape = unary_union([geometry(primitive) for primitive in run])
        result = result.union(shape) if dark else result.difference(shape)
    return result


def flash_id(obj):
    attrs = getattr(obj, 'attrs', {})
    label = '.'.join(attrs.get('.P', ())[:2]) or '.'.join(attrs.get('.C', ()))
    return f'{label}@({obj.x:.4f},{-obj.y:.4f})'


def outline(profile):
    primitives = [p for obj in profile.objects for p in obj.to_primitives(MM)]
    lines = [centerline(p) for p in primitives]
    endpoints = [(p.x1, p.y1) for p in primitives] + [(p.x2, p.y2) for p in primitives]
    gaps = [min(dist(a, b) for j, b in enumerate(endpoints) if i != j)
            for i, a in enumerate(endpoints)]
    def canonical(point):
        return min(other for other in endpoints if dist(point, other) <= 0.00003)

    joined = [LineString([canonical(line.coords[0]), *list(line.coords)[1:-1],
                          canonical(line.coords[-1])]) for line in lines]
    polygons, cuts, dangles, invalid = polygonize_full(joined)
    if len(polygons.geoms) != 1 or any(not g.is_empty for g in (cuts, dangles, invalid)):
        raise ValueError('Profile does not form a single clean contour')
    print('Profile bounds, primitives, shortest segment, largest endpoint gap:',
          polygons.geoms[0].bounds, len(lines), round(min(line.length for line in lines), 6),
          round(max(gaps), 6))
    print('Arc radii:', [round(hypot(p.x1-p.cx, p.y1-p.cy), 6)
                         for p in primitives if isinstance(p, gp.Arc)])
    angles = []
    for i, j in combinations(range(len(endpoints)), 2):
        if dist(endpoints[i], endpoints[j]) > 0.00003:
            continue
        directions = []
        for index in (i, j):
            line = lines[index % len(lines)]
            point = line.interpolate(0.0001 if index < len(lines) else line.length-0.0001)
            x, y = endpoints[index]
            directions.append((point.x-x, point.y-y))
        (x, y), (u, v) = directions
        angle = degrees(acos(max(-1, min(1, (x*u+y*v)/(hypot(x, y)*hypot(u, v))))))
        angles.append((round(180-angle, 3), endpoints[i]))
    print('Profile tangent deflections (degrees, CAM XY):', sorted(angles, reverse=True))
    return polygons.geoms[0]


def layer_features(copper):
    features = []
    for obj in copper.objects:
        attrs = getattr(obj, 'attrs', {})
        if not isinstance(attrs, dict):
            attrs = {}
        kind = type(obj).__name__
        if kind == 'Region':
            net, label = copper.region_net, f'{copper.region_net} plane'
        else:
            net, = attrs.get('.N', ('',))
            label = flash_id(obj) if kind == 'Flash' else f'trace@({obj.x1},{-obj.y1})'
        features.append((kind, net, label, painted(obj.to_primitives(MM))))
    return features


@lru_cache(maxsize=1024)
def bounds(shape):
    return shape.bounds


def box_gap(a, b):
    x1, y1, x2, y2 = bounds(a)
    u1, v1, u2, v2 = bounds(b)
    return hypot(max(x1-u2, u1-x2, 0), max(y1-v2, v1-y2, 0))


def shortest_pairs(features, condition):
    return sorted((round(a[3].distance(b[3]), 5), a[2], b[2])
                  for a, b in combinations(features, 2) if condition(a, b) and box_gap(a[3], b[3]) < 1.5)


def inspect_side(copper, mask, silk, board, holes):
    features = layer_features(copper)
    copper_image = unary_union([g for _, _, _, g in features])
    mask_features = [('Flash', '', flash_id(o), painted(o.to_primitives(MM))) for o in mask.objects]
    pad_flashes = [o for o in copper.objects if type(o).__name__ == 'Flash' and '.P' in o.attrs]
    pad_centers = Counter((round(o.x, 5), round(o.y, 5)) for o in pad_flashes)
    mask_centers = Counter((round(o.x, 5), round(o.y, 5)) for o in mask.objects)
    if pad_centers != mask_centers:
        raise ValueError('Mask opening and component pad centers differ')
    print('Registered mask openings:', len(mask.objects))
    mask_image = unary_union([g for _, _, _, g in mask_features])
    silk_image = painted(p for o in silk.objects for p in o.to_primitives(MM))
    print('Copper edge:', sorted((round(g.distance(board.boundary), 5), name)
                                 for _, _, name, g in features)[:3])
    for left, right in [('Line', 'Line'), ('Flash', 'Flash'), ('Line', 'Flash')]:
        print(left, right, 'different-net gaps:',
              shortest_pairs(features, lambda a, b: a[1] != b[1]
                             and sorted((a[0], b[0])) == sorted((left, right)))[:6])
    print('Plane gaps:', shortest_pairs(features, lambda a, b: a[1] != b[1]
          and 'Region' in (a[0], b[0]))[:6])
    print('Trace widths:', sorted({round(o.aperture.diameter, 6) for o in copper.objects
                                   if type(o).__name__ == 'Line'}))
    print('Mask webs:', shortest_pairs(mask_features, lambda a, b: True)[:8])
    print('Q mask webs:', shortest_pairs(mask_features, lambda a, b:
          a[2][:2] == b[2][:2] and a[2][:2] in {'Q1', 'Q2'}))
    print('Smallest mask opening bounding-box span:',
          min(min(g.bounds[2]-g.bounds[0], g.bounds[3]-g.bounds[1])
              for _, _, _, g in mask_features))
    print('Silk over mask openings / outside profile, mm²:',
          round(silk_image.intersection(mask_image).area, 7),
          round(silk_image.difference(board).area, 7))
    print('Mask area outside copper, mm²:', round(mask_image.difference(copper_image).area, 5))
    print('Mask and neighboring trace gaps:', sorted(
        (round(opening.distance(g), 5), label, name)
        for _, _, label, opening in mask_features
        for kind, net, name, g in features
        if kind == 'Line' and box_gap(opening, g) < 0.25
        and not any(kind == 'Flash' and n == net and pad.centroid.distance(opening.centroid) < 0.001
                    for kind, n, _, pad in features)
    )[:8])
    strokes = [(o.aperture.diameter, o.attrs.get('.C', ()), o.x1, -o.y1)
               for o in silk.objects if type(o).__name__ in {'Line', 'Arc'}]
    print('Minimum legend stroke (width, component, board XY):', min(strokes))
    print('Minimum legend-to-mask gap:', min(
        (round(silk_image.distance(opening), 5), label) for _, _, label, opening in mask_features))
    print('Copper outside profile, mm²:', round(copper_image.difference(board).area, 7))
    for _, hole in holes:
        center = hole.centroid
        pads = [(name, net, g) for kind, net, name, g in features
                if kind == 'Flash' and g.centroid.distance(center) < 0.001]
        if len(pads) != 1:
            raise ValueError(f'Expected one registered copper flash at {center}, found {pads}')
        name, net, pad = pads[0]
        ring = pad.boundary.distance(hole) if pad.covers(hole) else 0
        nearby = [(hole.distance(g), label) for _, n, label, g in features if n != net and box_gap(hole, g) < 2]
        gap, neighbor = min(nearby, default=(2, "none within 2 mm"))
        print(f'Drill {name} {net} ring={ring:.4f} edge={board.boundary.distance(hole):.4f}'
              f' other-net copper={gap:.4f} ({neighbor}) mask-open={mask_image.intersects(hole)}')
    return features


def inspect_connectivity(sides, holes):
    nets = sorted({n for features in sides for _, n, _, _ in features})
    for net in nets:
        pieces = []
        for side, features in enumerate(sides):
            shape = unary_union([g for _, n, _, g in features if n == net])
            pieces.extend((side, p) for p in getattr(shape, 'geoms', [shape]) if not p.is_empty)
        groups = [{i} for i in range(len(pieces))]
        for _, hole in holes:
            joins = {i for i, (_, p) in enumerate(pieces) if p.covers(hole.centroid)}
            if len(joins) > 1:
                matching = [group for group in groups if group & joins]
                groups = [group for group in groups if not group & joins]
                groups.append(set.union(*matching))
        print('CAM net', net, 'pieces by side',
              [sum(side == i for side, _ in pieces) for i in (0, 1)],
              'connected groups after plating', len(groups))
        if len(groups) != 1:
            raise ValueError(f'Disconnected artwork for {net}')


def verify_source(board_path, holes, layers):
    reader = runpy.run_path(str(Path(__file__).with_name('check-schematic-pcb-parity.py')))
    children, value = reader['children'], reader['value']
    board = reader['parse_board'](board_path)

    def at(node):
        x, y, *angle = map(float, children(node, 'at')[0][1:])
        return x, y, radians(angle[0] if angle else 0)

    def check(x, y, width, length):
        candidates = [(obj, shape) for obj, shape in holes if shape.centroid.distance(Point(x, -y)) < 0.001]
        obj, _ = candidates[0] if len(candidates) == 1 else (None, None)
        if obj is None or abs(obj.aperture.diameter - width) > 0.001:
            raise ValueError(f'Drill mismatch at board XY {(x, y)}')
        actual_length = obj.aperture.diameter
        if type(obj).__name__ == 'Line':
            actual_length += hypot(obj.x2-obj.x1, obj.y2-obj.y1)
        if abs(actual_length-length) > 0.0011:
            raise ValueError(f'Drill length mismatch at board XY {(x, y)}')

    placed_pads = {side: Counter() for side in ('F.Cu', 'B.Cu')}
    count = 0
    for via in children(board, 'via'):
        x, y, _ = at(via)
        diameter = float(value(via, 'drill'))
        check(x, y, diameter, diameter)
        count += 1
    for footprint in children(board, 'footprint'):
        x, y, rotation = at(footprint)
        fields = {prop[1]: prop[2] for prop in children(footprint, 'property')}
        for pad in children(footprint, 'pad'):
            pad_layers = children(pad, 'layers')[0][1:]
            for side, expected in placed_pads.items():
                if side in pad_layers or '*.Cu' in pad_layers:
                    expected[(fields['Reference'], pad[1], value(pad, 'net'))] += 1
            if pad[2] != 'thru_hole':
                continue
            px, py, _ = at(pad)
            drill = children(pad, 'drill')[0][1:]
            sizes = list(map(float, drill[1:] if drill[0] == 'oval' else drill))
            check(x+px*cos(rotation)+py*sin(rotation),
                  y-px*sin(rotation)+py*cos(rotation), min(sizes), max(sizes))
            count += 1
    if count != len(holes):
        raise ValueError(f'Expected {count} plated features from source, found {len(holes)}')
    print('Drill centers, tool widths and slot lengths matched against source:', count)
    for side, extension in [('F.Cu', '.gtl'), ('B.Cu', '.gbl')]:
        copper = layers[extension]
        actual = Counter((o.attrs['.P'][0], o.attrs['.P'][1], o.attrs['.N'][0])
                         for o in copper.objects if type(o).__name__ == 'Flash' and '.P' in o.attrs)
        expected = placed_pads[side]
        if actual != expected:
            raise ValueError(f'{side} pad flash mismatch: extra {actual-expected}, missing {expected-actual}')
        widths = Counter(round(float(value(seg, 'width')), 6) for seg in children(board, 'segment')
                         if value(seg, 'layer') == side)
        drawn = Counter(round(o.aperture.diameter, 6) for o in copper.objects if type(o).__name__ == 'Line')
        if widths != drawn:
            raise ValueError(f'{side} routed stroke width inventory differs from source')
        print(side, 'pad flashes and routed stroke width inventory match source:', sum(actual.values()), widths)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('output', type=Path)
    parser.add_argument('board', type=Path)
    args = parser.parse_args()
    layers = {}
    suffixes = {
        '.gtl': (('Copper', 'L1', 'Top'), 'Positive'),
        '.gbl': (('Copper', 'L2', 'Bot'), 'Positive'),
        '.gts': (('Soldermask', 'Top'), 'Negative'),
        '.gbs': (('Soldermask', 'Bot'), 'Negative'),
        '.gto': (('Legend', 'Top'), 'Positive'),
        '.gbo': (('Legend', 'Bot'), 'Positive'),
        '.gm1': (('Profile', 'NP'), None),
    }
    paths = [p for p in args.output.iterdir() if p.suffix in suffixes or p.suffix == '.drl']
    if len(paths) != 9:
        raise ValueError(f'Expected seven Gerbers and two drill files, found {paths}')
    for suffix, (function, polarity) in suffixes.items():
        path, = args.output.glob(f'fan-controller-*{suffix}')
        layer = layers[suffix] = GerberFile.open(path)
        attrs = layer.file_attrs
        if (attrs['.FileFunction'] != function
                or attrs.get('.FilePolarity') != ((polarity,) if polarity else None)
                or not layer.objects):
            raise ValueError(f'Unexpected layer content or attributes in {path}')
        counts = Counter(type(o).__name__ for o in layer.objects)
        if suffix in {'.gtl', '.gbl'} and counts['Region']:
            # Gerbonara 1.6.3 does not expose region object attributes.
            nets = re.findall(r'%TO\.N,([^*]+)\*%\s*G36\*', path.read_text())
            if counts['Region'] != 1 or len(nets) != 1:
                raise ValueError(f'Cannot identify the copper region in {path}')
            layer.region_net, = nets
        print(path.name, counts)
        (args.output / f'{path.name}.svg').write_text(str(layer.to_svg(force_bounds=((49, -130), (82, -48)))))
    board = outline(layers['.gm1'])
    holes = []
    for classification, x2 in [('PTH', 'Plated'), ('NPTH', 'NonPlated')]:
        path = args.output / f'fan-controller-{classification}.drl'
        if f'TF.FileFunction,{x2},1,2,{classification}' not in path.read_text():
            raise ValueError(f'Wrong drill classification for {path}')
        drill = ExcellonFile.open(path)
        print(classification, sorted(Counter((type(o).__name__, o.aperture.diameter)
                                            for o in drill.objects).items()))
        if classification == 'NPTH' and drill.objects:
            raise ValueError('This board audit expects no NPTH objects')
        holes.extend((obj, painted(obj.to_primitives(MM))) for obj in drill.objects)
    print('Minimum hole spacing:', min(a.distance(b) for (_, a), (_, b) in combinations(holes, 2)))
    verify_source(args.board, holes, layers)
    sides = []
    for suffixes in [('.gtl', '.gts', '.gto'), ('.gbl', '.gbs', '.gbo')]:
        print('\nSIDE', suffixes)
        sides.append(inspect_side(*(layers[s] for s in suffixes), board, holes))
    inspect_connectivity(sides, holes)


if __name__ == '__main__':
    main()
