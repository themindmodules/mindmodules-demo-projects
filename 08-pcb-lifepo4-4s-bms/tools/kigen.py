"""Tiny KiCad schematic generator (KiCad 9/10 file format).

Takes a parts list (ref, library symbol, value, footprint, pin->net map, position)
and writes a .kicad_sch where every pin gets a local net label at its connection point.
Symbol drawings are copied from KiCad's own libraries, so the schematic looks standard.
Pure Python, no third-party packages.
"""
import re
import uuid

KICAD_SYM_DIR = r"D:\KICAD INSTALLATION\share\kicad\symbols"


# ---------- s-expression helpers ----------
def tokenize(text):
    i, n = 0, len(text)
    while i < n:
        c = text[i]
        if c in " \t\r\n":
            i += 1
        elif c in "()":
            yield c
            i += 1
        elif c == '"':
            j = i + 1
            buf = []
            while text[j] != '"':
                if text[j] == "\\":
                    buf.append(text[j + 1])
                    j += 2
                else:
                    buf.append(text[j])
                    j += 1
            yield ("s", "".join(buf))
            i = j + 1
        else:
            j = i
            while j < n and text[j] not in " \t\r\n()":
                j += 1
            yield text[i:j]
            i = j


def parse(text):
    stack = [[]]
    for tok in tokenize(text):
        if tok == "(":
            stack.append([])
        elif tok == ")":
            done = stack.pop()
            stack[-1].append(done)
        else:
            stack[-1].append(tok)
    return stack[0][0]


def find_block_text(libtext, name):
    """Return the raw text of the top-level (symbol "name" ...) block."""
    key = '(symbol "%s"' % name
    start = libtext.find(key)
    if start < 0:
        raise KeyError(name)
    depth, i, in_str = 0, start, False
    while True:
        c = libtext[i]
        if in_str:
            if c == "\\":
                i += 1
            elif c == '"':
                in_str = False
        else:
            if c == '"':
                in_str = True
            elif c == "(":
                depth += 1
            elif c == ")":
                depth -= 1
                if depth == 0:
                    return libtext[start:i + 1]
        i += 1


def node_str(n):
    return n[1] if isinstance(n, tuple) else n


def walk_pins(tree):
    pins = []

    def rec(node, unit=0):
        if isinstance(node, list) and len(node) > 1 and node[0] == "symbol":
            m = re.search(r"_(\d+)_\d+$", node_str(node[1]))
            if m:
                unit = int(m.group(1))
        if isinstance(node, list) and node and node[0] == "pin" and len(node) > 3:
            at = next(x for x in node if isinstance(x, list) and x and x[0] == "at")
            num = next(x for x in node if isinstance(x, list) and x and x[0] == "number")
            nm = next(x for x in node if isinstance(x, list) and x and x[0] == "name")
            pins.append({"x": float(at[1]), "y": float(at[2]), "rot": int(float(at[3])),
                         "number": node_str(num[1]), "name": node_str(nm[1]), "unit": unit})
            return
        if isinstance(node, list):
            for ch in node:
                rec(ch, unit)

    rec(tree)
    return pins


_cache = {}


def load_symbol(lib_id):
    """lib_id like 'Device:R' -> (embedded text with qualified name, pins)."""
    if lib_id in _cache:
        return _cache[lib_id]
    lib, name = lib_id.split(":")
    with open(KICAD_SYM_DIR + "\\" + lib + ".kicad_sym", encoding="utf-8") as f:
        libtext = f.read()
    block = find_block_text(libtext, name)
    if "(extends" in block:
        raise ValueError("symbol %s uses extends; pick a base symbol" % lib_id)
    pins = walk_pins(parse(block))
    embedded = block.replace('(symbol "%s"' % name, '(symbol "%s"' % lib_id, 1)
    _cache[lib_id] = (embedded, pins)
    return _cache[lib_id]


def q(s):
    return '"' + str(s).replace("\\", "\\\\").replace('"', '\\"') + '"'


def uid():
    return str(uuid.uuid4())


LABEL_ANGLE = {0: 180, 180: 0, 90: 270, 270: 90}


def generate_sch(path, project, title, rev, parts, comment=""):
    """parts: list of dict(ref, lib, value, fp, pins{num:net}, at=(x,y))"""
    root = uid()
    out = []
    w = out.append
    w("(kicad_sch\n\t(version 20250114)\n\t(generator \"eeschema\")\n\t(generator_version \"9.0\")")
    w("\t(uuid %s)\n\t(paper \"A4\")" % q(root))
    w("\t(title_block\n\t\t(title %s)\n\t\t(rev %s)\n\t\t(company \"TheMindModules\")\n\t\t(comment 1 %s)\n\t)"
      % (q(title), q(rev), q(comment)))
    w("\t(lib_symbols")
    seen = set()
    for p in parts:
        if p["lib"] in seen:
            continue
        seen.add(p["lib"])
        emb, _ = load_symbol(p["lib"])
        w("\t\t" + emb.replace("\n", "\n\t\t"))
    w("\t)")

    labels = []
    for p in parts:
        emb, pins = load_symbol(p["lib"])
        unit = p.get("unit", 1)
        pins = [pin for pin in pins if pin["unit"] in (0, unit)]
        sx, sy = p["at"]
        w("\t(symbol\n\t\t(lib_id %s)\n\t\t(at %s %s 0)\n\t\t(unit %d)\n\t\t(exclude_from_sim no)\n\t\t(in_bom %s)\n\t\t(on_board yes)\n\t\t(dnp no)\n\t\t(uuid %s)"
          % (q(p["lib"]), sx, sy, unit, "no" if p["ref"].startswith("#") else "yes", q(uid())))
        hide = "(effects (font (size 1.27 1.27)) (hide yes))"
        w("\t\t(property \"Reference\" %s (at %s %s 0) (effects (font (size 1.27 1.27)) (justify left)))"
          % (q(p["ref"]), round(sx + 3.5, 2), round(sy - 3.0, 2)))
        w("\t\t(property \"Value\" %s (at %s %s 0) (effects (font (size 1.27 1.27)) (justify left)))"
          % (q(p["value"]), round(sx + 3.5, 2), round(sy + 3.0, 2)))
        w("\t\t(property \"Footprint\" %s (at %s %s 0) %s)" % (q(p["fp"]), sx, sy, hide))
        w("\t\t(property \"Datasheet\" \"~\" (at %s %s 0) %s)" % (sx, sy, hide))
        for pin in pins:
            w("\t\t(pin %s (uuid %s))" % (q(pin["number"]), q(uid())))
        w("\t\t(instances (project %s (path %s (reference %s) (unit %d))))\n\t)"
          % (q(project), q("/" + root), q(p["ref"]), unit))
        for pin in pins:
            net = p["pins"].get(pin["number"])
            if not net:
                continue
            px, py = round(sx + pin["x"], 3), round(sy - pin["y"], 3)
            ang = LABEL_ANGLE[pin["rot"] % 360]
            just = "right bottom" if ang == 180 else "left bottom"
            labels.append((net, px, py, ang, just))

    for net, px, py, ang, just in labels:
        w("\t(label %s (at %s %s %s) (effects (font (size 1.27 1.27)) (justify %s)) (uuid %s))"
          % (q(net), px, py, ang, just, q(uid())))
    w("\t(sheet_instances (path \"/\" (page \"1\")))\n\t(embedded_fonts no)\n)")
    with open(path, "w", encoding="utf-8") as f:
        f.write("\n".join(out) + "\n")


def write_project(path, name):
    import json
    with open(path, "w", encoding="utf-8") as f:
        json.dump({"meta": {"filename": name + ".kicad_pro", "version": 3}}, f, indent=2)
