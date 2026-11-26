"""Small two-layer grid router for through-hole boards (pure Python).

Tracks run on a 0.5 mm grid, Manhattan moves only, vias allowed. Obstacles are
inflated by half track width plus clearance, so a finished route passes DRC with the
default 0.2 mm clearance. Good enough for a hand-placed board with a few dozen nets.
"""
import heapq
import math

G = 0.5          # grid pitch, mm
TRACK = 0.4      # routed track width
CLEAR = 0.2      # clearance used when inflating obstacles
VIA_NEIGH = [(-1, -1), (-1, 0), (-1, 1), (0, -1), (0, 1), (1, -1), (1, 0), (1, 1)]


class Router:
    def __init__(self, width, height, edge=1.2):
        self.w, self.h, self.edge = width, height, edge
        self.own = [{}, {}]            # layer -> {cell: net or '*'}
        self.cells = {}                # net -> [set(F cells), set(B cells)]
        self.fixed = {}                # net -> [set(F cells), set(B cells)] of pre-placed tracks
        self.tracks = []               # (net, layer, (x1,y1), (x2,y2))
        self.vias = []                 # (net, x, y)

    # ---- helpers ----
    def cell(self, x, y):
        return (round(x / G), round(y / G))

    def xy(self, c):
        return (c[0] * G, c[1] * G)

    def mark(self, layer, x, y, r, net):
        o = self.own[layer]
        for ix in range(int((x - r) / G) - 1, int((x + r) / G) + 2):
            for iy in range(int((y - r) / G) - 1, int((y + r) / G) + 2):
                if (ix * G - x) ** 2 + (iy * G - y) ** 2 <= r * r:
                    cur = o.get((ix, iy))
                    if cur is None or cur == net:
                        o[(ix, iy)] = net
                    elif cur != net:
                        o[(ix, iy)] = "*"

    def add_pad(self, x, y, r, net, thru=True):
        for layer in (0, 1) if thru else (0,):
            self.mark(layer, x, y, r + CLEAR + TRACK / 2 + 0.02, net)

    def add_fixed_track(self, net, layer, pts, width):
        """A pre-placed track (for example a wide power track). Registered as an obstacle and as part of its net."""
        half = width / 2 + CLEAR + TRACK / 2
        for a, b in zip(pts, pts[1:]):
            n = max(1, int(math.hypot(b[0] - a[0], b[1] - a[1]) / (G / 2)))
            for i in range(n + 1):
                x = a[0] + (b[0] - a[0]) * i / n
                y = a[1] + (b[1] - a[1]) * i / n
                self.mark(layer, x, y, half, net)
                self.fixed.setdefault(net, [set(), set()])[layer].add(self.cell(x, y))

    def free(self, layer, c, net):
        x, y = self.xy(c)
        if x < self.edge or y < self.edge or x > self.w - self.edge or y > self.h - self.edge:
            return False
        v = self.own[layer].get(c)
        return v is None or v == net

    def via_ok(self, c, net):
        if not (self.free(0, c, net) and self.free(1, c, net)):
            return False
        for dx, dy in VIA_NEIGH:
            n = (c[0] + dx, c[1] + dy)
            for layer in (0, 1):
                v = self.own[layer].get(n)
                if v is not None and v != net:
                    return False
        return True

    # ---- routing ----
    def route_net(self, net, pads):
        """pads: list of (x, y). Connects them all. Returns list of unreachable pad coordinates."""
        failed = []
        nc = self.cells.setdefault(net, [set(), set()])
        pcells = []
        for (x, y) in pads:
            c = self.cell(x, y)
            pcells.append(c)
            # short stub from exact pad centre to its grid cell, on both layers
            sx, sy = self.xy(c)
            if abs(sx - x) > 0.005 or abs(sy - y) > 0.005:
                for layer in (0, 1):
                    self.tracks.append((net, layer, (x, y), (sx, sy)))
            nc[0].add(c)
            nc[1].add(c)
        targets = [set(), set()]
        if net in self.fixed:
            fx = self.fixed[net]
            connected = set(fx[0]) | set(fx[1])
            remaining = [i for i in range(len(pads)) if pcells[i] not in connected]
        else:
            connected = {pcells[0]}
            remaining = list(range(1, len(pads)))
            targets[0].add(pcells[0])
            targets[1].add(pcells[0])
        # any fixed tracks of this net are targets too
        while remaining:
            # nearest unconnected pad to the connected set
            best = None
            for i in remaining:
                d = min(abs(pcells[i][0] - c[0]) + abs(pcells[i][1] - c[1]) for c in connected)
                if best is None or d < best[0]:
                    best = (d, i)
            i = best[1]
            remaining.remove(i)
            path = self._astar(net, pcells[i], targets, nc)
            if path is None:
                failed.append(pads[i])
                continue
            self._commit(net, path, targets, nc)
            connected.add(pcells[i])
            targets[0].add(pcells[i])
            targets[1].add(pcells[i])
        return failed

    def _astar(self, net, start, targets, nc):
        # targets: per-layer cell sets of the net routed so far; fixed tracks of the net count too
        fx = self.fixed.get(net, [set(), set()])
        tg = [targets[0] | fx[0], targets[1] | fx[1]]
        heap = []
        seen = {}
        for L in (0, 1):
            heapq.heappush(heap, (0.0, L, start, None))
        prev = {}
        while heap:
            cost, L, c, parent = heapq.heappop(heap)
            key = (L, c)
            if key in seen:
                continue
            seen[key] = cost
            prev[key] = parent
            if c in tg[L] and c != start:
                return self._backtrace(prev, key)
            for dx, dy in ((1, 0), (-1, 0), (0, 1), (0, -1)):
                n = (c[0] + dx, c[1] + dy)
                if (L, n) in seen or not self.free(L, n, net):
                    continue
                step = 1.0 + (0.3 if parent is not None and parent[1] != (c[0] - dx, c[1] - dy) and parent[0] == L else 0)
                heapq.heappush(heap, (cost + step, L, n, key))
            o = 1 - L
            if (o, c) not in seen and self.via_ok(c, net):
                heapq.heappush(heap, (cost + 12.0, o, c, key))
        return None

    def _backtrace(self, prev, key):
        out = []
        while key is not None:
            out.append(key)
            key = prev[key]
        return out[::-1]

    def _commit(self, net, path, targets, nc):
        # split into straight runs per layer
        runs = []
        cur = [path[0]]
        for k in path[1:]:
            if k[0] == cur[-1][0]:
                cur.append(k)
            else:
                runs.append(cur)
                cur = [k]
        runs.append(cur)
        for run in runs:
            L = run[0][0]
            pts = [r[1] for r in run]
            # merge collinear
            merged = [pts[0]]
            for i in range(1, len(pts) - 1):
                a, b, c = pts[i - 1], pts[i], pts[i + 1]
                if (b[0] - a[0], b[1] - a[1]) != (c[0] - b[0], c[1] - b[1]):
                    merged.append(b)
            merged.append(pts[-1])
            for a, b in zip(merged, merged[1:]):
                if a != b:
                    self.tracks.append((net, L, self.xy(a), self.xy(b)))
            for p in pts:
                x, y = self.xy(p)
                self.mark(L, x, y, TRACK / 2 + CLEAR + TRACK / 2, net)
                targets[L].add(p)
                nc[L].add(p)
        for i in range(len(path) - 1):
            if path[i][0] != path[i + 1][0]:
                x, y = self.xy(path[i][1])
                self.vias.append((net, x, y))
                for L in (0, 1):
                    self.mark(L, x, y, 0.4 + CLEAR + TRACK / 2 + 0.05, net)
                    targets[L].add(path[i][1])
                    nc[L].add(path[i][1])
