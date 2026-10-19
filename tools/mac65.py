#!/usr/bin/env python3
"""mac65.py - an assembler for the Red Baron source (Atari's MAC65 dialect).

  python -I tools/mac65.py [--listing FILE] [--symbols FILE] [--quiet]

It assembles the modules of upstream/ in the order and at the addresses of
the load map (RBARON.MAP) and holds every byte against the release ROMs
(upstream/03xxxx.01, MAME's set redbaron). What comes out is the address of
every label and, for tools/mac2c.py, the program as a list of statements.

Not everything of 1981 is in the repository: the include files COND65
(structured-programming macros), ASCVG (text strings) and MBDIAG (the math
box diagnostic) are missing. The first two are built in here; RBINT is
assembled up to the self test, where MBDIAG would be included.
"""
import os
import re
import sys

HERE = os.path.dirname(os.path.abspath(__file__))
ROOT = os.path.dirname(HERE)
UP = os.path.join(ROOT, "upstream")

# module, file, the address of its .CSECT (RBARON.MAP; RBARON itself is absolute)
MODULES = [
    ("RBARON", "RBARON.MAC", None),
    ("RBCOIN", "RBCOIN.MAC", 0x70B7),
    ("RBSOUN", "RBSOUN.MAC", 0x71C4),
    ("RBGRND", "RBGRND.MAC", 0x72C8),
    ("VGUT", "VGUT.MAC", 0x7856),
    ("RBROM", "RBROM.MAC", 0x7909),
    ("RBINT", "RBINT.MAC", 0x79E2),
]
ROM_FILES = [("037001.01", 0x4800), ("037000.01", 0x5000), ("036999.01", 0x5800), ("036998.01", 0x6000),
             ("036997.01", 0x6800), ("036996.01", 0x7000), ("036995.01", 0x7800)]

OPS = {  # mnemonic: {mode: opcode}
    "ADC": dict(imm=0x69, zp=0x65, zpx=0x75, abs=0x6D, absx=0x7D, absy=0x79, indx=0x61, indy=0x71),
    "AND": dict(imm=0x29, zp=0x25, zpx=0x35, abs=0x2D, absx=0x3D, absy=0x39, indx=0x21, indy=0x31),
    "ASL": dict(acc=0x0A, zp=0x06, zpx=0x16, abs=0x0E, absx=0x1E),
    "BIT": dict(zp=0x24, abs=0x2C),
    "CMP": dict(imm=0xC9, zp=0xC5, zpx=0xD5, abs=0xCD, absx=0xDD, absy=0xD9, indx=0xC1, indy=0xD1),
    "CPX": dict(imm=0xE0, zp=0xE4, abs=0xEC),
    "CPY": dict(imm=0xC0, zp=0xC4, abs=0xCC),
    "DEC": dict(zp=0xC6, zpx=0xD6, abs=0xCE, absx=0xDE),
    "EOR": dict(imm=0x49, zp=0x45, zpx=0x55, abs=0x4D, absx=0x5D, absy=0x59, indx=0x41, indy=0x51),
    "INC": dict(zp=0xE6, zpx=0xF6, abs=0xEE, absx=0xFE),
    "JMP": dict(abs=0x4C, ind=0x6C),
    "JSR": dict(abs=0x20),
    "LDA": dict(imm=0xA9, zp=0xA5, zpx=0xB5, abs=0xAD, absx=0xBD, absy=0xB9, indx=0xA1, indy=0xB1),
    "LDX": dict(imm=0xA2, zp=0xA6, zpy=0xB6, abs=0xAE, absy=0xBE),
    "LDY": dict(imm=0xA0, zp=0xA4, zpx=0xB4, abs=0xAC, absx=0xBC),
    "LSR": dict(acc=0x4A, zp=0x46, zpx=0x56, abs=0x4E, absx=0x5E),
    "ORA": dict(imm=0x09, zp=0x05, zpx=0x15, abs=0x0D, absx=0x1D, absy=0x19, indx=0x01, indy=0x11),
    "ROL": dict(acc=0x2A, zp=0x26, zpx=0x36, abs=0x2E, absx=0x3E),
    "ROR": dict(acc=0x6A, zp=0x66, zpx=0x76, abs=0x6E, absx=0x7E),
    "SBC": dict(imm=0xE9, zp=0xE5, zpx=0xF5, abs=0xED, absx=0xFD, absy=0xF9, indx=0xE1, indy=0xF1),
    "STA": dict(zp=0x85, zpx=0x95, abs=0x8D, absx=0x9D, absy=0x99, indx=0x81, indy=0x91),
    "STX": dict(zp=0x86, zpy=0x96, abs=0x8E),
    "STY": dict(zp=0x84, zpx=0x94, abs=0x8C),
}
IMPLIED = dict(BRK=0x00, CLC=0x18, CLD=0xD8, CLI=0x58, CLV=0xB8, DEX=0xCA, DEY=0x88, INX=0xE8, INY=0xC8, NOP=0xEA,
               PHA=0x48, PHP=0x08, PLA=0x68, PLP=0x28, RTI=0x40, RTS=0x60, SEC=0x38, SED=0xF8, SEI=0x78, TAX=0xAA,
               TAY=0xA8, TSX=0xBA, TXA=0x8A, TXS=0x9A, TYA=0x98)
BRANCH = dict(BPL=0x10, BMI=0x30, BVC=0x50, BVS=0x70, BCC=0x90, BCS=0xB0, BNE=0xD0, BEQ=0xF0)
PREFIX = {"I": "imm", "A": "abs", "AX": "absx", "AY": "absy", "Z": "zp", "ZX": "zpx", "ZY": "zpy", "NY": "indy",
          "NX": "indx", "N": "ind", "X": "autox", "Y": "autoy"}
SIZE = dict(imp=1, acc=1, imm=2, zp=2, zpx=2, zpy=2, indx=2, indy=2, rel=2, abs=3, absx=3, absy=3, ind=3)

# COND65 (missing): IFxx ... THEN skips the block unless xx holds; BEGIN ... xxEND loops until xx holds.
COND = dict(CC="BCS", CS="BCC", EQ="BNE", NE="BEQ", MI="BPL", PL="BMI", VC="BVS", VS="BVC")
# Macros that load half of an address: handled as the instruction they stand for.
HALF = dict(LAH=("LDA", "hi"), LAL=("LDA", "lo"), LXL=("LDX", "lo"), LYL=("LDY", "lo"), CPL=("CMP", "lo"),
            CPH=("CMP", "hi"), SAH=("SBC", "hi"), CXH=("CPX", "hi"))

IDENT = r"[A-Za-z0-9.$]+"


def key(name):
    """MAC65 looks at the first six characters of a symbol only."""
    return name.upper()[:6]


class AsmError(Exception):
    pass


class Stmt:
    """One assembled statement, for the listing and for mac2c."""
    __slots__ = ("module", "file", "line", "addr", "labels", "kind", "mnem", "mode", "expr", "cexpr", "value",
                 "data", "comment", "text", "scope", "macro", "target")

    def __init__(self, **kw):
        for s in self.__slots__:
            setattr(self, s, kw.get(s))


class Module:
    def __init__(self, asm, name, path, base):
        self.asm, self.name, self.path, self.base = asm, name, path, base
        self.sym = {}           # this pass
        self.prev = {}          # the pass before
        self.spell = {}
        self.globl = set()
        self.macros = {}
        self.stmts = []


class Asm:
    def __init__(self, quiet=False):
        self.quiet = quiet
        self.rom = bytearray(0x8000)
        self.have = bytearray(0x8000)
        for fn, at in ROM_FILES:
            d = open(os.path.join(UP, fn), "rb").read()[:0x800]
            self.rom[at:at + len(d)] = d
            for i in range(len(d)):
                self.have[at + i] = 1
        self.mods = [Module(self, n, os.path.join(UP, f), b) for n, f, b in MODULES]
        self.glob, self.glob_prev = {}, {}
        self.errors = []
        self.mismatch = {}
        self.final = False

    # --- files -----------------------------------------------------------------

    @staticmethod
    def read(path):
        d = open(path, "rb").read().replace(b"\x00", b"").decode("latin1")
        return d.replace("\r", "").split("\n")

    # --- symbols ---------------------------------------------------------------

    def lookup(self, name):
        k = key(name)
        m = self.m
        for d in (m.sym, m.prev, self.glob, self.glob_prev):
            if k in d:
                return d[k]
        return None

    def define(self, name, value):
        k = key(name)
        self.m.sym[k] = value & 0xFFFF if value >= 0 else value
        if k not in self.m.spell or len(name) > len(self.m.spell[k]):
            self.m.spell[k] = name.upper()

    # --- expressions -----------------------------------------------------------

    def tokens(self, s):
        out = []
        i, n = 0, len(s)
        while i < n:
            c = s[i]
            if c in " \t":
                i += 1
            elif c == "^":
                r = s[i + 1].upper()
                j = i + 2
                while j < n and s[j].isalnum():
                    j += 1
                base = dict(H=16, O=8, D=10, B=2)[r]
                out.append(("num", int(s[i + 2:j], base), base))
                i = j
            elif c.isdigit():
                j = i
                while j < n and (s[j].isalnum()):
                    j += 1
                word = s[i:j]
                if j < n and s[j] == "$":
                    out.append(("local", word))
                    j += 1
                elif j < n and s[j] == "." and word.isdigit():
                    out.append(("num", int(word, 10), 10))
                    j += 1
                else:
                    out.append(("num", int(word, self.radix), self.radix))
                i = j
            elif c == "'":
                out.append(("num", ord(s[i + 1]), 16))
                i += 2
            elif c.isalpha() or c in ".$":
                j = i
                while j < n and (s[j].isalnum() or s[j] in ".$"):
                    j += 1
                word = s[i:j]
                out.append(("loc",) if word == "." else ("sym", word))
                i = j
            elif c in "+-*/&!<>":
                out.append(("op", c))
                i += 1
            else:
                raise AsmError("character %r in expression %r" % (c, s))
        return out

    def expr(self, s):
        """Value and C text of an expression; evaluated left to right."""
        self.undefined = None
        toks = self.tokens(s)
        self.pos = 0
        v, c, _ = self.expr_seq(toks)
        if self.pos != len(toks):
            raise AsmError("expression %r" % s)
        return v, c

    def expr_seq(self, toks):
        v, c, last = self.term(toks)
        while self.pos < len(toks) and toks[self.pos][0] == "op" and toks[self.pos][1] in "+-*/&!":
            op = toks[self.pos][1]
            self.pos += 1
            w, d, dl = self.term(toks)
            if op == "+":
                v = v + w
            elif op == "-":
                v = v - w
            elif op == "*":
                v = v * w
            elif op == "/":
                v = int(v / w) if w else 0
            elif op == "&":
                v = v & w
            else:
                v = v | w
            cop = "|" if op == "!" else op
            if last and not (op in "+-" and last in "+-*/") and not (op in "*/" and last in "*/"):
                c = "(" + c + ")"
            if dl:
                d = "(" + d + ")"
            c = "%s %s %s" % (c, cop, d)
            last = op
        return v, c, last

    def term(self, toks):
        if self.pos >= len(toks):
            raise AsmError("expression ends early")
        t = toks[self.pos]
        self.pos += 1
        if t[0] == "op" and t[1] in "+-":
            v, c, last = self.term(toks)
            if t[1] == "-":
                return -v, "-" + ("(" + c + ")" if last else c), None
            return v, c, last
        if t[0] == "op" and t[1] == "<":
            v, c, last = self.expr_seq(toks)
            if self.pos >= len(toks) or toks[self.pos] != ("op", ">"):
                raise AsmError("missing >")
            self.pos += 1
            return v, "(" + c + ")" if last else c, None
        if t[0] == "num":
            return t[1], ("%d" % t[1] if t[2] == 10 or t[1] < 10 else "0x%X" % t[1]), None
        if t[0] == "loc":
            return self.pc, "0x%04X" % self.pc, None
        if t[0] == "local":
            k = (self.scope, int(t[1]))
            v = self.m.sym.get(k, self.m.prev.get(k))
            if v is None:
                self.undefined = t[1] + "$"
                v = 0
            return v, "0x%04X" % v, None
        v = self.lookup(t[1])
        if v is None:
            self.undefined = t[1]
            v = 0
        return v, "{%s}" % key(t[1]), None

    def cname(self, name):
        k = key(name)
        return cident(self.m.spell.get(k) or self.spell_any(k) or name.upper())

    def spell_any(self, k):
        for m in self.mods:
            if k in m.spell and (k in m.sym or k in m.prev):
                return m.spell[k]
        return None

    # --- output ----------------------------------------------------------------

    def emit(self, data, st=None):
        if self.final and st is not None:
            st.addr = self.pc
            st.data = bytes(data)
        for b in data:
            a = self.pc & 0xFFFF
            if self.final and a < 0x8000 and self.have[a] and self.rom[a] != (b & 0xFF):
                self.mismatch.setdefault(self.m.name, []).append((a, b & 0xFF, self.rom[a], self.where()))
            if self.final and a < 0x8000:
                self.cover[a] = 1
            self.pc += 1

    def where(self):
        return "%s:%d" % (os.path.basename(self.file), self.lineno)

    def add(self, **kw):
        st = Stmt(module=self.m.name, file=os.path.basename(self.file), line=self.lineno, addr=self.pc,
                  labels=self.pending, comment=self.comment, text=self.srctext, scope=self.scope,
                  macro=self.in_macro, **kw)
        self.pending = []
        if self.final:
            self.m.stmts.append(st)
        return st

    # --- one module ------------------------------------------------------------

    def run(self, m):
        self.m = m
        m.prev = m.sym
        m.sym = {}
        m.macros = {}
        m.stmts = []
        self.radix = 8          # MACRO-11's default; every file sets .RADIX 16
        self.radix = 16
        self.pc = m.base or 0       # (a module without .CSECT is in the unnamed one)
        self.section = "" if m.base else ". ABS."
        self.pcs = {}
        self.m68 = False
        self.scope = 0
        self.scopes = 0
        self.pending = []
        self.ifs = []
        self.cond_stack = []
        self.gen = 0
        self.stop = False
        self.mexit = False
        self.in_macro = None
        self.labl_seq = 0
        self.lines(self.read(m.path), m.path, 0)
        for k in m.globl:
            if k in m.sym:
                self.glob[k] = m.sym[k]

    def active(self):
        return all(f["on"] for f in self.ifs)

    def lines(self, lines, path, depth):
        i = 0
        while i < len(lines) and not self.stop:
            self.file, self.lineno = path, (i + 1 if depth == 0 else self.lineno)
            if depth == 0 or path != self.m.path:
                self.file, self.lineno = path, i + 1
            raw = lines[i]
            i += 1
            try:
                i = self.line(raw, lines, i, depth)
            except AsmError as e:
                if self.final:
                    self.errors.append("%s: %s   [%s]" % (self.where(), e, raw.strip()))

    def split_comment(self, raw):
        depth = 0
        for j, c in enumerate(raw):
            if c == "<":
                depth += 1
            elif c == ">":
                depth = max(0, depth - 1)
            elif c == ";" and depth == 0:
                return raw[:j], raw[j + 1:].strip()
        return raw, ""

    def line(self, raw, lines, i, depth):
        body, self.comment = self.split_comment(raw)
        self.srctext = raw
        s = body.strip()
        # --- inside a false conditional: only the conditionals themselves count
        if not self.active():
            mm = re.match(r"^(?:%s:\s*)?\.(IF|IFF|IFT|IFTF|ENDC)\b" % IDENT, s, re.I)
            if mm:
                self.directive("." + mm.group(1).upper(), s[mm.end():].strip(), lines, i)
            return i
        if not s:
            if self.comment and self.final and depth == 0:
                self.add(kind="comment")
            return i
        # --- labels
        while True:
            mm = re.match(r"^(%s)\s*:" % IDENT, s)
            if not mm:
                break
            self.label(mm.group(1))
            s = s[mm.end():].strip()
        if not s:
            return i
        # --- assignment
        mm = re.match(r"^(%s)\s*=\s*(.*)$" % IDENT, s)
        if mm and not re.match(r"^\.(IF|IIF)\b", s, re.I):
            name, rhs = mm.group(1), mm.group(2).strip()
            v, c = self.expr(rhs)
            if name == ".":
                if self.final:
                    self.add(kind="org", value=v)
                self.pc = v & 0xFFFF
            else:
                self.define(name, v)
                if self.final and not name.startswith("..."):
                    self.add(kind="equ", mnem=name.upper(), value=v, cexpr=c)
            return i
        mm = re.match(r"^(%s)\s*(.*)$" % IDENT, s)
        if not mm:
            raise AsmError("cannot read the statement")
        op, rest = mm.group(1).upper(), mm.group(2).strip()
        if op.startswith(".") and op not in self.m.macros and key(op) not in self.m.macros:
            return self.directive(op, rest, lines, i) or i
        self.instruction(op, rest, depth)
        return i

    def label(self, name):
        if re.match(r"^\d+\$$", name):
            self.m.sym[(self.scope, int(name[:-1]))] = self.pc
            self.pending.append("%s@%d" % (name, self.scope))   # (a local label, and the scope it is in)
            return
        self.scopes += 1
        self.scope = self.scopes
        self.define(name, self.pc)
        self.pending.append(name.upper())

    # --- directives ------------------------------------------------------------

    def split_args(self, s):
        out, cur, depth = [], "", 0
        for c in s:
            if c == "<":
                depth += 1
            elif c == ">":
                depth -= 1
            if c == "," and depth == 0:
                out.append(cur.strip())
                cur = ""
            else:
                cur += c
        if cur.strip() or out:
            out.append(cur.strip())
        return out

    def block(self, lines, i, opens, close):
        """The lines up to the matching close directive; returns (block, next index)."""
        depth, out = 1, []
        while i < len(lines):
            t = self.split_comment(lines[i])[0]
            mm = re.match(r"^\s*(?:%s:\s*)?(\.[A-Za-z]+)" % IDENT, t)
            d = mm.group(1).upper() if mm else ""
            if d in opens:
                depth += 1
            elif d == close:
                depth -= 1
                if depth == 0:
                    return out, i + 1
            out.append(lines[i])
            i += 1
        raise AsmError("no %s" % close)

    def condition(self, rest):
        parts = rest.split(",", 1)
        cond = parts[0].strip().upper()
        arg = parts[1] if len(parts) > 1 else ""
        if cond in ("B", "NB"):
            a = arg.split(",")[0].strip() if cond else ""
            blank = a.strip("<> \t") == ""
            return (blank if cond == "B" else not blank), arg.split(",", 1)[1] if "," in arg else ""
        first = self.split_args(arg)
        a = first[0] if first else ""
        tail = arg[arg.index(",") + 1:] if "," in arg else ""
        if cond in ("DF", "NDF"):
            d = self.lookup(a.strip()) is not None
            return (d if cond == "DF" else not d), tail
        v, _ = self.expr(a)
        if v >= 0x8000:
            v -= 0x10000
        r = dict(EQ=v == 0, Z=v == 0, NE=v != 0, NZ=v != 0, GT=v > 0, G=v > 0, GE=v >= 0, LT=v < 0, L=v < 0,
                 LE=v <= 0)[cond]
        return r, tail

    def directive(self, op, rest, lines, i):
        if op == ".IF":
            if not self.active():
                self.ifs.append(dict(cond=False, on=False, dead=True))
            else:
                c, _ = self.condition(rest)
                self.ifs.append(dict(cond=c, on=c, dead=False))
            return i
        if op in (".IFF", ".IFT", ".IFTF"):
            f = self.ifs[-1]
            if not f["dead"]:
                f["on"] = (not f["cond"]) if op == ".IFF" else f["cond"] if op == ".IFT" else True
            return i
        if op == ".ENDC":
            self.ifs.pop()
            return i
        if op == ".IIF":
            c, tail = self.condition(rest)
            if c:
                self.line(tail, lines, i, 1)
            return i
        if op in (".TITLE", ".SBTTL"):
            if self.final:
                self.add(kind="title", mnem=op, expr=rest if not self.comment else rest + ";" + self.comment)
            return i
        if op == ".MEXIT":
            self.mexit = True
            return i
        if op in (".LIST", ".NLIST", ".PAGE", ".ERROR", ".PRINT"):
            return i
        if op in (".GLOBL", ".GLOBB"):
            for n in self.split_args(rest):
                if n:
                    self.m.globl.add(key(n))
            return i
        if op in (".ENABL", ".ENABLE", ".DSABL"):
            if "M68" in rest.upper():
                self.m68 = op != ".DSABL"
            return i
        if op == ".RADIX":
            self.radix = int(rest) if rest.strip().isdigit() else self.expr(rest)[0]
            return i
        if op in (".ASECT", ".CSECT"):
            self.pcs[self.section] = self.pc
            self.section = ". ABS." if op == ".ASECT" else rest.strip().upper()
            self.pc = self.pcs.get(self.section, 0 if op == ".ASECT" else self.m.base)
            return i
        if op == ".BLKB":
            v, _ = self.expr(rest)
            if self.final:
                self.add(kind="blkb", value=v)
            self.pc += v
            return i
        if op in (".BYTE", ".WORD"):
            data, texts = [], []
            for a in self.split_args(rest):
                v, c = self.expr(a)
                if self.undefined and self.final:
                    self.errors.append("%s: undefined %s" % (self.where(), self.undefined))
                texts.append(c)
                if op == ".BYTE":
                    data.append(v & 0xFF)
                elif self.m68:
                    data += [(v >> 8) & 0xFF, v & 0xFF]
                else:
                    data += [v & 0xFF, (v >> 8) & 0xFF]
            st = self.add(kind="data", mnem=op, expr=rest, cexpr=", ".join(texts)) if self.final else None
            self.emit(data, st)
            return i
        if op == ".ASCVG":
            text = rest.strip().strip("<>")
            data = [ascvg(ch) for ch in text]
            data[-1] |= 0x80
            st = self.add(kind="data", mnem=op, expr=rest) if self.final else None
            self.emit(data, st)
            return i
        if op == ".END":
            self.stop = True
            return i
        if op == ".INCLUDE":
            name = rest.split()[0].upper()
            if name in ("COND65", "ASCVG"):
                return i
            if name == "MBDIAG":        # missing: the self test is not assembled from here on
                self.stop = True
                return i
            path = os.path.join(UP, name + ".MAC")
            save = (self.file, self.lineno)
            self.lines(self.read(path), path, 0)
            self.stop = False           # (an include's .END ends the include only)
            self.file, self.lineno = save
            return i
        if op == ".MACRO":
            parts = [p for p in re.split(r"[,\s]+", rest.strip()) if p]
            body, j = self.block(lines, i, (".MACRO",), ".ENDM")
            self.m.macros[key(parts[0])] = (parts[1:], body)
            return j
        if op == ".REPT":
            n, _ = self.expr(rest)
            body, j = self.block(lines, i, (".REPT", ".IRPC", ".IRP"), ".ENDR")
            for _ in range(n):
                self.lines_inline(body)
            return j
        if op == ".IRPC":
            parts = rest.split(",", 1)
            formal, chars = parts[0].strip(), parts[1].strip().strip("<>")
            body, j = self.block(lines, i, (".REPT", ".IRPC", ".IRP"), ".ENDR")
            for ch in chars:
                self.lines_inline([substitute(b, [formal], [ch]) for b in body])
            return j
        if op == ".VCTRS":
            return i
        raise AsmError("directive %s" % op)

    def lines_inline(self, body):
        j = 0
        while j < len(body) and not self.stop and not self.mexit:
            raw = body[j]
            j += 1
            j = self.line(raw, body, j, 1)

    # --- instructions and macros -------------------------------------------------

    def gen_label(self):
        self.gen += 1
        return 9000 + self.gen

    def instruction(self, op, rest, depth):
        m = self.m
        if op in HALF and (key(op) in m.macros or True):
            mnem, half = HALF[op]
            v, c = self.expr(rest)
            b = (v >> 8) & 0xFF if half == "hi" else v & 0xFF
            st = self.add(kind="ins", mnem=mnem, mode="imm", expr=rest, value=b,
                          cexpr=("HI(%s)" if half == "hi" else "LO(%s)") % c) if self.final else None
            self.emit([OPS[mnem]["imm"], b], st)
            return
        if op == "MATSTT":
            st = self.add(kind="ins", mnem="MATSTT", mode="imp") if self.final else None
            self.emit([0x2C, 0x00, 0x18, 0x30, 0xFB], st)
            return
        if key(op) in m.macros:
            formals, body = m.macros[key(op)]
            actual = []
            for a in self.split_args(rest) if "," in rest or len(formals) <= 1 else rest.split():
                if a.startswith("\\"):
                    v, _ = self.expr(a[1:])
                    a = ("%X" if self.radix == 16 else "%d") % v
                actual.append(a)
            actual += [""] * (len(formals) - len(actual))
            save = self.in_macro
            self.in_macro = self.in_macro or op
            depth_ifs = len(self.ifs)
            self.lines_inline([substitute(b, formals, actual) for b in body])
            if self.mexit:              # left in the middle: its open conditionals go with it
                del self.ifs[depth_ifs:]
                self.mexit = False
            self.in_macro = save
            return
        if op in ("THEN", "BEGIN") or (len(op) == 4 and op[:2] == "IF" and op[2:] in COND) or \
                (len(op) == 5 and op[2:] == "END" and op[:2] in COND):
            self.structured(op)
            return
        if op in IMPLIED:
            st = self.add(kind="ins", mnem=op, mode="imp") if self.final else None
            self.emit([IMPLIED[op]], st)
            return
        if op in BRANCH:
            v, c = self.expr(rest)
            if self.undefined and self.final and not a_in_selftest(self.pc):
                self.errors.append("%s: undefined %s" % (self.where(), self.undefined))
            off = v - (self.pc + 2)
            if self.undefined:
                off = self.rom[self.pc + 1]
            if self.final and not -128 <= (off if off < 128 else off - 256) <= 127:
                self.errors.append("%s: branch out of range" % self.where())
            st = self.add(kind="ins", mnem=op, mode="rel", expr=rest, value=v, target=rest.strip()) if self.final else None
            self.emit([BRANCH[op], off & 0xFF], st)
            return
        if op not in OPS:
            raise AsmError("unknown operation %s" % op)
        modes = OPS[op]
        if not rest:
            if "acc" not in modes:
                raise AsmError("%s needs an operand" % op)
            st = self.add(kind="ins", mnem=op, mode="acc") if self.final else None
            self.emit([modes["acc"]], st)
            return
        mode, e = "auto", rest
        if rest.startswith("#"):
            mode, e = "imm", rest[1:]
        elif "," in rest:
            head, tail = rest.split(",", 1)
            if head.strip().upper() in PREFIX:
                mode, e = PREFIX[head.strip().upper()], tail
            else:
                raise AsmError("addressing %r" % rest)
        v, c = self.expr(e)
        undefined = self.undefined
        if mode in ("auto", "autox", "autoy"):
            suffix = {"auto": "", "autox": "x", "autoy": "y"}[mode]
            small = 0 <= v < 0x100 and not undefined and ("zp" + suffix) in modes
            mode = ("zp" if small else "abs") + suffix
            if op in ("JMP", "JSR"):
                mode = "abs"
            # (where the source leaves the size open, the release ROM settles it)
            a = self.pc
            if self.final and a < 0x8000 and self.have[a] and mode in modes and self.rom[a] != modes[mode]:
                for alt in (("abs" if small else "zp") + suffix,):
                    if alt in modes and self.rom[a] == modes[alt] and (alt.startswith("abs") or v < 0x100):
                        mode = alt
        if mode not in modes:
            raise AsmError("%s has no mode %s" % (op, mode))
        if undefined and self.final:
            if a_in_selftest(self.pc):
                v = self.rom[self.pc + 1] | (self.rom[self.pc + 2] << 8)
            else:
                self.errors.append("%s: undefined %s" % (self.where(), undefined))
        st = None
        if self.final:
            st = self.add(kind="ins", mnem=op, mode=mode, expr=e.strip(), cexpr=c, value=v & 0xFFFF,
                          target=e.strip() if op in ("JMP", "JSR") else None)
        if SIZE[mode] == 2:
            self.emit([modes[mode], v & 0xFF], st)
        else:
            self.emit([modes[mode], v & 0xFF, (v >> 8) & 0xFF], st)

    def structured(self, op):
        if op == "THEN":
            n = self.cond_stack.pop()
            self.m.sym[(self.scope, n)] = self.pc
            self.pending.append("%d$@%d" % (n, self.scope))
            return
        if op == "BEGIN":
            n = self.gen_label()
            self.cond_stack.append(n)
            self.m.sym[(self.scope, n)] = self.pc
            self.pending.append("%d$@%d" % (n, self.scope))
            return
        if op.startswith("IF"):
            n = self.gen_label()
            self.cond_stack.append(n)
            br = COND[op[2:]]
        else:
            n = self.cond_stack.pop()
            br = COND[op[:2]]
        v = self.m.sym.get((self.scope, n), self.m.prev.get((self.scope, n), self.pc))
        st = self.add(kind="ins", mnem=br, mode="rel", expr="%d$" % n, value=v, target="%d$" % n) if self.final else None
        self.emit([BRANCH[br], (v - (self.pc + 2)) & 0xFF], st)

    # --- all of it -------------------------------------------------------------

    def assemble(self):
        for p in range(4):
            self.final = p == 3
            self.glob_prev, self.glob = self.glob, {}
            self.cover = bytearray(0x8000)
            for m in self.mods:
                self.run(m)
        return self


def a_in_selftest(pc):
    return pc >= 0x7AA5


def ascvg(ch):
    """A character as the text strings hold it: its offset in VGMSGA, the table of character pictures."""
    if ch == " ":
        return 0
    if ch.isdigit():
        return 2 * (ord(ch) - ord("0")) + 2
    return 2 * (ord(ch.upper()) - ord("A")) + 22


def substitute(line, formals, actual):
    if not formals:
        return line
    amap = {f.upper(): a for f, a in zip(formals, actual)}
    out, last_sub = [], False
    pos = 0
    for mm in re.finditer(IDENT + r"|'", line):
        out.append(line[pos:mm.start()])
        tok = mm.group(0)
        pos = mm.end()
        if tok == "'":
            # the concatenation mark: one is dropped on each side of an argument
            nxt = re.match(IDENT, line[pos:])
            if last_sub or (nxt and nxt.group(0).upper() in amap):
                last_sub = False
                continue
            out.append(tok)
            last_sub = False
        elif tok.upper() in amap:
            out.append(amap[tok.upper()])
            last_sub = True
        else:
            out.append(tok)
            last_sub = False
    out.append(line[pos:])
    return "".join(out)


def cident(name):
    """A source symbol as part of a C identifier."""
    s = name.replace(".", "_").replace("$", "S")
    return s


def main():
    quiet = "--quiet" in sys.argv
    asm = Asm(quiet).assemble()
    bad = sum(len(v) for v in asm.mismatch.values())
    for mod in asm.mods:
        n = sum(len(st.data) for st in mod.stmts if st.data)
        mm = asm.mismatch.get(mod.name, [])
        print("%-7s %5d bytes, %d differ from the ROMs" % (mod.name, n, len(mm)))
        if not quiet:
            for a, mine, rom, where in mm[:12]:
                print("    %04X here %02X ROM %02X  (%s)" % (a, mine, rom, where))
    for e in asm.errors[:40]:
        print("error:", e)
    covered = sum(asm.cover[0x4800:0x8000])
    print("%d of %d bytes of 4800-7FFF assembled, %d differ, %d errors" % (covered, 0x3800, bad, len(asm.errors)))
    if "--listing" in sys.argv:
        path = sys.argv[sys.argv.index("--listing") + 1]
        with open(path, "w", encoding="latin1") as f:
            for mod in asm.mods:
                for st in mod.stmts:
                    if st.kind in ("ins", "data"):
                        f.write("%04X  %-12s %s\n" % (st.addr, " ".join("%02X" % b for b in st.data[:4]),
                                                      st.text.rstrip() if st.text else ""))
                    elif st.text:
                        f.write("                   %s\n" % st.text.rstrip())
    return 1 if bad or asm.errors else 0


if __name__ == "__main__":
    sys.exit(main())
