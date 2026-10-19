#!/usr/bin/env python3
"""mac2c.py - the Red Baron program as C, routine for routine.

  python -I tools/mac2c.py

Reads the source through tools/mac65.py (which holds it against the ROMs)
and writes

  src/core/rb_symbols.h   every symbol of the source with its value
  src/game/game.h         the routines
  src/game/*.c            the routines, one C statement per instruction

Each routine of the source becomes a C function of the same name; the
6502's registers are kept in the machine and every instruction is a macro
of src/game/m6502.h, so that the C computes the same in every bit and the
1981 source reads one to one against it. The original's labels and comments
stand next to the code.

How routines are cut: a label is a function when it is called (JSR), or
jumped at from outside the function it lies in. A jump or a fall through to
another function is a call followed by a return. What the translation
cannot do by rule is listed in the tables below.
"""
import os
import re
import sys

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import mac65  # noqa: E402

ROOT = mac65.ROOT

# RBARON.MAC is cut into files at these source lines.
RBARON_FILES = [
    (728, "main", "MAIN LOOP, SOUNDS, END OF LIFE, GAME INITIALIZATION, SCORE"),
    (1656, "states", "STATES, HIGH SCORE TABLE ROUTINES"),
    (2140, "objects", "NEW MOTION OBJECTS, SHELLS AND PLANES; UPDATE OBJECTS"),
    (3546, "ground", "GROUND SEQUENCE"),
    (4378, "display", "DISPLAY: PLAYFIELD, PLANE MO, EXPLOSIONS, SHELLS"),
    (5272, "alpha", "ALPHANUMERICS"),
    (5479, "collide", "COLLISION DETECT, HOUSEKEEPING (I/O)"),
    (5962, "math", "TRIG CALCULATION & Z PROJECTION, NUMERIC ROUTINES"),
]
MODULE_FILES = dict(RBCOIN=("coin", "RBCOIN, TCN65: THE COIN ROUTINE"), RBSOUN=("sound", "RBSOUN: POKEY SOUND SYSTEM CONTROL"),
                    RBGRND=("grnd", "RBGRND: NMI, POSITION BEAM, PROJECT, ROTATE, DECODE"),
                    VGUT=("vgut", "VGUT: VECTOR GENERATOR UTILITIES"), RBROM=("earom", "RBROM: EAROM INTERFACE"),
                    RBINT=("int", "RBINT: MESSAGES, DIGITS, POWER ON"))

# Functions whether or not anything calls them.
ENTRIES = {"START", "MAIN", "NMI", "PWRON"}
# Written by hand (src/game/frame.c): the main line's wait for the interrupt.
HAND = {"INTWAI"}
# The self test (RBINT from RAMTST on) is not ported.
LIMIT = {"RBINT": 0x7B0F}
# MAIN is cut where it waits for the frame's end: what follows the wait is a function of its own.
SPLIT_AFTER_JSR = {("MAIN_V", "INTWAI"): "MAIN2"}
# In these functions a jump to the label ends the function (the frame is over).
JUMP_RETURNS = {("MAIN2", "MAIN")}
# What a line of the picture is (core/tags.h): these calls draw with the
# tag given, which the machine notes next to every word of vector RAM
# written meanwhile. Nothing the game computes changes. (source file, line
# of the JSR): tag
TAG_AT = {
    ("RBARON.MAC", 798): "RB_TAG_COCKPIT",      # MAIN: LOADRAM NOREST, the player's own plane
    ("RBARON.MAC", 808): "RB_TAG_LAND",         # MAIN: SCENE
    ("RBARON.MAC", 809): "RB_TAG_GSHELL",       # MAIN: GRSMTN, the guns on the ground
    ("RBARON.MAC", 810): "RB_TAG_PLANE",        # MAIN: MOTOBJ
    ("RBARON.MAC", 811): "RB_TAG_BLIMP",        # MAIN: BLMOTN
    ("RBARON.MAC", 812): "RB_TAG_SHELL",        # MAIN: SHLMOT
    ("RBARON.MAC", 821): "RB_TAG_COPYRIGHT",    # MAIN: LOADRAM RCOPYR
    ("RBARON.MAC", 822): "RB_TAG_LIVES",        # MAIN: DSPLIF
    ("RBARON.MAC", 823): "RB_TAG_EOL",          # MAIN: EOLSEQ, the end of a life
    ("RBARON.MAC", 824): "RB_TAG_STATES",       # MAIN: STATES, the high score pages, the banner
    ("RBARON.MAC", 825): "RB_TAG_TEXT",         # MAIN: ALPHAS, score and messages
    ("RBARON.MAC", 1329): "RB_TAG_PLANE",       # PLCRSH: the plane in pieces (its pieces keep its colour)
    ("RBARON.MAC", 2114): "RB_TAG_PLANE",       # DSPBNR: the banner's plane
    ("RBARON.MAC", 2120): "RB_TAG_STATES",      # DSPBNR: its outline
    ("RBARON.MAC", 4445): "RB_TAG_MOUNTAIN",    # SCENE: PFDSPL, a mountain on the horizon
    ("RBARON.MAC", 4467): "RB_TAG_HORIZON",     # SCENE: SCENE0, the horizon between them
    ("RBARON.MAC", 4502): "RB_TAG_MOUNTAIN",    # SCENE: SCENE1 and SCENE2, a mountain come off it
    ("RBARON.MAC", 4503): "RB_TAG_MOUNTAIN",
    ("RBARON.MAC", 4507): "RB_TAG_GROUND",      # SCENE: GRDISP, what stands on the ground
    ("RBARON.MAC", 4772): "RB_TAG_DRONE",       # MOTOBJ: DRONE
    ("RBARON.MAC", 4774): "RB_TAG_DRONE",       # MOTOBJ: DRNEXP, a drone in pieces
    ("RBARON.MAC", 4896): "RB_TAG_PROP",        # PLANE: PLNPRP, the propeller's pictures
    ("RBARON.MAC", 3678): "RB_TAG_EXPLOSION",   # GRDISP: a ground object blowing up (EXCRCL)
    ("RBARON.MAC", 3685): "RB_TAG_EXPLOSION",
    # ALPHAS: the texts at the top and the messages
    ("RBARON.MAC", 5283): "RB_TAG_SCORE",       # the score's digits
    ("RBARON.MAC", 5284): "RB_TAG_SCORE",       # and its last 0
    ("RBARON.MAC", 5299): "RB_TAG_VALUE",       # the plane's value
    ("RBARON.MAC", 5300): "RB_TAG_VALUE",
    ("RBARON.MAC", 5312): "RB_TAG_HISCORE",     # the best score
    ("RBARON.MAC", 5313): "RB_TAG_HISCORE",
    ("RBARON.MAC", 5315): "RB_TAG_HISCORE",
    ("RBARON.MAC", 5317): "RB_TAG_HISCORE",     # its initials
    ("RBARON.MAC", 5321): "RB_TAG_LABEL",       # SCORE
    ("RBARON.MAC", 5325): "RB_TAG_LABEL",       # VALUE
    ("RBARON.MAC", 5327): "RB_TAG_LABEL",       # HIGH SCORE
    ("RBARON.MAC", 5331): "RB_TAG_WARN",        # the guns are overheated
    ("RBARON.MAC", 5338): "RB_TAG_CREDITS",     # CREDITS
    ("RBARON.MAC", 5346): "RB_TAG_CREDITS",
    ("RBARON.MAC", 5351): "RB_TAG_CREDITS",     # a half credit
    ("RBARON.MAC", 5364): "RB_TAG_ATTRACT",     # two plays minimum
    ("RBARON.MAC", 5366): "RB_TAG_ATTRACT",
    ("RBARON.MAC", 5368): "RB_TAG_ATTRACT",
    ("RBARON.MAC", 5398): "RB_TAG_ATTRACT",     # 1 COIN 1 PLAY
    ("RBARON.MAC", 5403): "RB_TAG_ATTRACT",
    ("RBARON.MAC", 5411): "RB_TAG_ATTRACT",
    ("RBARON.MAC", 5416): "RB_TAG_ATTRACT",
    ("RBARON.MAC", 5421): "RB_TAG_INSERT",      # INSERT COINS
    ("RBARON.MAC", 5427): "RB_TAG_INSERT",      # FREE PLAY
    ("RBARON.MAC", 5431): "RB_TAG_INSERT",      # PRESS START
    ("RBARON.MAC", 5440): "RB_TAG_BONUS",       # BONUS LIFE, 300 POINTS, BEHIND YOU
}
# These calls draw a whole biplane through DECODE: its vectors get notes
# of which point they go to and how far away it is (render/solid.h).
NOTE_AT = {
    ("RBARON.MAC", 4915): "RB_OBJ_PLANE",       # PLANE: PLDECD
    ("RBARON.MAC", 5129): "RB_OBJ_PIECES",      # PLNEXP: the pieces of a plane
    ("RBARON.MAC", 4445): "RB_OBJ_MOUNTAIN",    # SCENE: PFDSPL, a mountain on the horizon
    ("RBARON.MAC", 4503): "RB_OBJ_MOUNTAIN",    # SCENE: SCENE2 (falls into PFDSPL), one come off it
    ("RBARON.MAC", 3645): "RB_OBJ_GROUND",      # GRDISP: an object on the ground (its type: TEMP2)
}
# C put after a statement of the source (file, line).
INSERT_AFTER = {
    ("RBGRND.MAC", 693): "rb_note_point(m);",   # DECODE: the point a vector goes to (X: its number * 6)
    ("RBGRND.MAC", 716): "m->note = 0;",        # DECODE: written; the beam's other moves go to no point
}

# MAIN moves everything up to its label 20$ and draws from there on: the
# drawing is a function of its own (MAIN_VIEW), entered through a routine
# by hand that can keep the state the picture is drawn from
# (game/frame.h, render/subframe.h).
SPLIT_AT_LOCAL = {"MAIN": (20, "rb_view_begin", "MAIN_VIEW")}

CKEYWORDS = {"do", "if", "int", "for", "char", "long", "short", "case", "else", "enum", "goto", "void", "auto",
             "break", "const", "float", "union", "while", "double", "extern", "return", "signed", "sizeof", "static",
             "struct", "switch", "default", "typedef", "continue", "register", "unsigned", "volatile", "inline",
             "restrict", "m", "main", "abs", "div", "exp", "log", "sin", "cos", "tan", "pow", "time", "rand", "exit"}

key = mac65.key


def clean(comment):
    c = comment.replace("/*", "/ *").replace("*/", "* /").replace("\t", " ").rstrip()
    return c[:-1] + "/" if c.endswith("\\") else c


class Translator:
    def __init__(self):
        self.asm = mac65.Asm(quiet=True).assemble()
        if self.asm.errors or self.asm.mismatch:
            sys.exit("mac2c: the source does not assemble to the ROMs (run tools/mac65.py)")
        self.name_symbols()
        self.collect()
        self.find_functions()

    # --- symbols -----------------------------------------------------------------

    def name_symbols(self):
        """A C name for every symbol: RB_ and the source's name, '.' as '_', '$' dropped
        (where that collides with another symbol: as 'S')."""
        self.sym = {}                   # (module, key) -> (cname, value, spelling)
        by_c = {}
        rows = []
        for m in self.asm.mods:
            m.all = dict(m.prev)
            m.all.update(m.sym)
            for k, v in m.all.items():
                if not isinstance(k, str) or k.startswith("...") or re.match(r"^STAT[0-9A-F]+$", k):
                    continue
                rows.append((m, k, v, m.spell.get(k, k)))
        for stage in (0, 1):
            for m, k, v, spell in rows:
                plain = spell.replace("$", "").replace(".", "_")
                marked = spell.replace("$", "S").replace(".", "_")
                if stage == 0:
                    by_c.setdefault(plain, set()).add((v, plain == marked))
                    continue
                users = by_c[plain]
                cn = plain
                if len({u[0] for u in users}) > 1:       # the name stands for different things
                    if plain != marked:
                        cn = marked
                    elif len({u for u in users if u[1]}) > 1 or sum(1 for mm, kk, vv, ss in rows
                                                                    if ss.replace("$", "").replace(".", "_") == plain
                                                                    and ss == spell and vv != v):
                        cn = plain + "_" + m.name
                self.sym[(m.name, k)] = (cn, v, spell)
        # a global symbol has the name its defining module gave it
        self.glob = {}
        for m in self.asm.mods:
            for k in m.globl:
                if k in m.all and (m.name, k) in self.sym:
                    self.glob[k] = self.sym[(m.name, k)]

    def cname(self, module, k):
        k = key(k)
        if (module, k) in self.sym:
            return self.sym[(module, k)][0]
        if k in self.glob:
            return self.glob[k][0]
        return mac65.cident(k)

    def cexpr(self, st):
        if st.cexpr is None:
            return ""
        return re.sub(r"\{([^}]+)\}", lambda mm: "RB_" + self.cname(st.module, mm.group(1)), st.cexpr)

    # --- statements ----------------------------------------------------------------

    def collect(self):
        """Per module: the statements with the labels moved onto the instruction or data they name."""
        self.code = {}
        self.label_at = {}              # (module, key) -> index of its instruction
        self.glabel = {}                # key -> (module, index), for the global ones
        for m in self.asm.mods:
            out, pend = [], []
            limit = LIMIT.get(m.name, 0x10000)
            for st in m.stmts:
                if st.addr is not None and st.kind in ("ins", "data") and st.addr >= limit:
                    break
                labels = st.labels or []
                if st.kind in ("ins", "data"):
                    st.labels = pend + labels
                    pend = []
                elif st.kind in ("comment", "title"):
                    pend += labels          # (a label on a line of its own)
                    st.labels = []
                else:
                    pend = []
                out.append(st)
            self.code[m.name] = out
            for i, st in enumerate(out):
                if st.kind == "ins":
                    for lb in st.labels:
                        if not is_loc(lb):
                            self.label_at[(m.name, key(lb))] = i
                            if key(lb) in m.globl:
                                self.glabel[key(lb)] = (m.name, i)

    def resolve(self, module, name):
        """Where a named label is: (module, index) or None."""
        k = key(name)
        if (module, k) in self.label_at:
            return module, self.label_at[(module, k)]
        return self.glabel.get(k)

    @staticmethod
    def is_local(name):
        return bool(re.match(r"^\d+\$$", name.strip()))

    def find_functions(self):
        self.starts = {}                # module -> set of indices
        for mod, code in self.code.items():
            s = set()
            prev_code = False
            for i, st in enumerate(code):
                if st.kind == "ins":
                    if not prev_code and any(not is_loc(lb) for lb in st.labels):
                        s.add(i)        # code begins here (after data or at the top)
                    prev_code = True
                elif st.kind == "data":
                    prev_code = False
            self.starts[mod] = s
        for e in ENTRIES:
            r = self.resolve("RBARON", e) or self.glabel.get(key(e))
            if r:
                self.starts[r[0]].add(r[1])
        for mod, code in self.code.items():
            for st in code:
                if st.kind == "ins" and st.mnem == "JSR":
                    r = self.resolve(mod, st.target)
                    if r:
                        self.starts[r[0]].add(r[1])
        changed = True
        while changed:
            changed = False
            for mod, code in self.code.items():
                owner = self.owners(mod)
                for i, st in enumerate(code):
                    if st.kind != "ins" or st.mnem == "JSR" or not st.target or self.is_local(st.target):
                        continue
                    r = self.resolve(mod, st.target)
                    if not r:
                        continue
                    if r[0] != mod or self.owners(r[0])[r[1]] != owner[i]:
                        if r[1] not in self.starts[r[0]]:
                            self.starts[r[0]].add(r[1])
                            changed = True

    def owners(self, mod):
        code = self.code[mod]
        own, cur = [None] * len(code), None
        for i, st in enumerate(code):
            if i in self.starts[mod]:
                cur = i
            elif st.kind == "data":
                cur = None
            own[i] = cur if st.kind == "ins" else None
        return own

    def fname(self, mod, i):
        st = self.code[mod][i]
        lb = [x for x in st.labels if not is_loc(x)][0]
        return "rb_" + self.cname(mod, lb).lower()

    def flabel(self, mod, i):
        return [x for x in self.code[mod][i].labels if not is_loc(x)][0]

    # --- one instruction ---------------------------------------------------------------

    def operand(self, st, write=False):
        e = self.cexpr(st)
        mode = st.mode
        if mode == "imm":
            return e, None
        w = "W" if write else ""
        addr = {"zp": e, "abs": e, "zpx": "ZX(%s)" % e, "zpy": "ZY(%s)" % e, "absx": "AX%s(%s)" % (w, e),
                "absy": "AY%s(%s)" % (w, e), "indy": "IY%s(%s)" % (w, e), "indx": "IX(%s)" % e}[mode]
        return None, addr

    def ins(self, st):
        """The C for an instruction that is no jump."""
        mn = st.mnem
        if mn == "MATSTT":
            return "MATSTT();"
        if st.mode == "imp":
            if mn == "TXS":
                return "/* TXS: the stack is C's */"
            if mn == "TSX":
                return "LDX(0xFF);"
            return mn + "();"
        if st.mode == "acc":
            return mn + "A();"
        writes = mn in ("STA", "STX", "STY", "INC", "DEC", "ASL", "LSR", "ROL", "ROR")
        imm, addr = self.operand(st, writes)
        if writes:
            return "%s(%s);" % (mn, addr)
        if imm is not None:
            return "%s(%s);" % (mn, imm)
        return "%s(RD(%s));" % (mn, addr)

    # --- writing -------------------------------------------------------------------------

    def write(self):
        outdir = os.path.join(ROOT, "src", "game")
        os.makedirs(outdir, exist_ok=True)
        protos = []
        files = []
        for m in self.asm.mods:
            code = self.code[m.name]
            if m.name == "RBARON":
                cuts = RBARON_FILES
                for n, (line, fn, title) in enumerate(cuts):
                    end = cuts[n + 1][0] if n + 1 < len(cuts) else 10 ** 9
                    idx = [i for i, st in enumerate(code) if line <= st.line < end and st.file == "RBARON.MAC"]
                    files.append((fn, title, m.name, idx))
            else:
                fn, title = MODULE_FILES[m.name]
                first = min(self.starts[m.name]) if self.starts[m.name] else 0
                # the statements before the first routine are declarations
                idx = [i for i in range(len(code)) if i >= self.first_text(m.name)]
                files.append((fn, title, m.name, idx))
        for fn, title, mod, idx in files:
            text = self.file_text(fn, title, mod, idx, protos)
            with open(os.path.join(outdir, fn + ".c"), "w", encoding="ascii", errors="replace", newline="\n") as f:
                f.write(text)
        self.write_header(os.path.join(outdir, "game.h"), protos)
        self.write_symbols(os.path.join(ROOT, "src", "core", "rb_symbols.h"))
        return files

    def first_text(self, mod):
        code = self.code[mod]
        for i, st in enumerate(code):
            if st.kind in ("ins", "data"):
                # back up over the comments that stand right in front
                j = i
                while j > 0 and code[j - 1].kind == "comment":
                    j -= 1
                return j
        return len(code)

    def file_text(self, fn, title, mod, idx, protos):
        code = self.code[mod]
        own = self.owners(mod)
        out = []
        out.append("/* %s.c - %s\n *\n * Red Baron (Atari, 1981), %s of the original source as C.\n"
                   " * Written by tools/mac2c.py from upstream/%s: change the translator or its\n"
                   " * tables, not this file. The routines, labels and comments are the original's;\n"
                   " * one C statement stands for one instruction (src/game/m6502.h).\n */\n"
                   % (fn, title, mod, code[idx[0]].file if idx else mod))
        out.append('#include "game/game.h"\n#include "game/m6502.h"\n')
        pos = 0
        idxset = set(idx)
        n = len(idx)
        cur = None                      # the function being written
        body = []
        pending_comments = []

        def flush_comments(indent):
            for c in pending_comments:
                out.append(("%s/* %s */" % (indent, clean(c))) if c else "")
            pending_comments.clear()

        def close():
            nonlocal cur
            if cur is not None:
                out.extend(self.finish(body))
                out.append("}\n")
                body.clear()
                cur = None

        for pos in range(n):
            i = idx[pos]
            st = code[i]
            if st.kind == "comment":
                if cur is not None and own_next(code, own, i) == cur:
                    body.append(("comment", st.comment))
                else:
                    close()
                    pending_comments.append(st.comment)
                continue
            if st.kind == "title":
                if cur is not None and own_next(code, own, i) == cur:
                    body.append(("comment", "---- %s ----" % (st.expr or "")))
                    continue
                close()
                flush_comments("")
                out.append("\n/* ---- %s ---- */\n" % clean(st.expr or ""))
                continue
            if st.kind in ("equ", "org", "blkb"):
                continue
            if st.kind == "data":
                close()
                flush_comments("")
                names = " ".join(lb + ":" for lb in st.labels)
                out.append("/* %04X %s %s %s%s */" % (st.addr, names, st.mnem, clean(st.expr or ""),
                                                     ("   ;" + clean(st.comment)) if st.comment else ""))
                continue
            # an instruction
            if own[i] is None:
                close()
                flush_comments("")
                out.append("/* %04X (not reached) %s */" % (st.addr, clean(st.text.strip())))
                continue
            if i in self.starts[mod]:
                close()
                cur = i
                name = self.flabel(mod, i)
                if key(name) in HAND:
                    flush_comments("")
                    out.append("/* %s: written by hand, src/game/frame.c */\n" % name)
                    cur = None
                    self.skipping = i
                    continue
                self.skipping = None
                flush_comments("")
                protos.append((self.fname(mod, i), name, st.addr, fn))
                out.append("/* %s (%04X) */" % (name, st.addr))
                out.append("void %s(rb_machine *m)\n{" % self.fname(mod, i))
                self.fn_mod, self.fn_start, self.fn_scope0 = mod, i, st.scope
                self.fn_name = name
            elif getattr(self, "skipping", None) is not None and own[i] == self.skipping:
                continue
            if cur is None:
                continue
            for c in pending_comments:
                body.append(("comment", c))
            pending_comments.clear()
            self.statement(mod, i, st, body, own, protos, fn)
        close()
        flush_comments("")
        return "\n".join(out) + "\n"

    def clabel(self, mod, st_scope, name):
        """The C label for a label of the source, inside the current function."""
        if self.is_local(name):
            n = int(name.strip()[:-1])
            if st_scope == self.fn_scope0:
                return "l%d" % n
            return "%s_l%d" % (self.scope_name.get(st_scope, "s%d" % st_scope), n)
        c = self.cname(mod, name).lower()
        return c + "_" if c in CKEYWORDS else c

    def statement(self, mod, i, st, body, own, protos, fn):
        code = self.code[mod]
        # the scope's name, for local labels of the later scopes of a function
        if not hasattr(self, "scope_name"):
            self.scope_name = {}
        for lb in st.labels:
            if not is_loc(lb):
                self.scope_name[st.scope] = self.cname(mod, lb).lower()
        for lb in st.labels:
            if is_loc(lb) and key(self.fn_name) in SPLIT_AT_LOCAL:
                n, entry, name = SPLIT_AT_LOCAL[key(self.fn_name)]
                if lb.split("@")[0] == "%d$" % n and int(lb.split("@")[1]) == self.fn_scope0:
                    body.append(("code", "%s(m); return;" % entry, "(the frame's picture: game/frame.h)"))
                    body.append(("split", name, st.addr))
                    self.fn_name = name
                    self.fn_start = -1
        for lb in st.labels:
            if i == self.fn_start and not is_loc(lb):
                body.append(("label", "top"))
            elif is_loc(lb):
                name, scope = lb.split("@")
                body.append(("label", self.clabel(mod, int(scope), name)))
            else:
                body.append(("label", self.clabel(mod, st.scope, lb)))
        cm = st.comment or ""
        mn = st.mnem
        nxt_same = i + 1 < len(code) and own[i + 1] == own[i]

        def goto(target):
            """C for a transfer of control to the label target."""
            t = target.strip()
            if self.is_local(t):
                cut = SPLIT_AT_LOCAL.get(key(self.fn_name))
                if cut and int(t[:-1]) == cut[0] and st.scope == self.fn_scope0:
                    return "{ %s(m); return; }" % cut[1]
                return "goto %s;" % self.clabel(mod, st.scope, t)
            if (key(self.fn_name), key(t)) in JUMP_RETURNS:
                return "return;"
            r = self.resolve(mod, t)
            if r is None:
                return "{ rb_not_ported(m, \"%s\"); return; }" % t.upper()
            if r[0] == mod and r[1] == self.fn_start:
                return "goto top;"
            if r[0] == mod and self.owners(mod)[r[1]] == self.fn_start and r[1] not in self.starts[mod]:
                return "goto %s;" % self.clabel(mod, code[r[1]].scope, t)
            if key(self.flabel(r[0], r[1])) in HAND:
                return "{ rb_%s(m); return; }" % self.cname(r[0], t).lower()
            return "{ %s(m); return; }" % self.fname(r[0], r[1])

        if mn in mac65.BRANCH:
            if st.value == st.addr:
                body.append(("code", "if (%s) { rb_hang(m); return; }" % mn, cm))
            else:
                body.append(("code", "if (%s) %s" % (mn, goto(st.target)), cm))
        elif mn == "JMP":
            if st.mode == "ind":
                body.append(("code", "rb_not_ported(m, \"JMP (%s)\"); return;" % st.expr, cm))
            elif st.value == st.addr:
                body.append(("code", "rb_hang(m); return;", cm))
            else:
                g = goto(st.target)
                body.append(("code", g[2:-2] if g.startswith("{ ") else g, cm))
            return
        elif mn == "JSR":
            r = self.resolve(mod, st.target)
            if r is None:
                body.append(("code", "rb_not_ported(m, \"%s\");" % st.target.upper(), cm))
            else:
                callee = self.flabel(r[0], r[1])
                call = "%s(m)" % ("rb_" + self.cname(r[0], callee).lower())
                if (st.file, st.line) in TAG_AT:
                    call = "RB_TAGGED(%s, %s)" % (TAG_AT[(st.file, st.line)], call)
                if (st.file, st.line) in NOTE_AT:
                    call = "RB_NOTED(%s, %s)" % (NOTE_AT[(st.file, st.line)], call)
                body.append(("code", call + ";", cm))
                split = SPLIT_AFTER_JSR.get((key(self.fn_name), key(callee)))
                if split:
                    body.append(("code", "return;", ""))
                    body.append(("split", split, st.addr + 3))
                    self.fn_name = split
                    self.fn_start = -1          # (its start is no label: nothing jumps there)
        elif mn in ("RTS", "RTI"):
            body.append(("code", "return;", cm))
            return
        else:
            body.append(("code", self.ins(st), cm))
        if (st.file, st.line) in INSERT_AFTER:
            body.append(("code", INSERT_AFTER[(st.file, st.line)], "(an addition: game/frame.h)"))
        # falling out of the function into the next one
        if not nxt_same:
            j = i + 1
            while j < len(code) and code[j].kind not in ("ins", "data"):
                j += 1
            if j < len(code) and code[j].kind == "ins" and j in self.starts[mod]:
                if key(self.flabel(mod, j)) in HAND:
                    body.append(("code", "rb_%s(m);" % self.cname(mod, self.flabel(mod, j)).lower(), ""))
                else:
                    body.append(("code", "%s(m);" % self.fname(mod, j), "(falls into it)"))

    def finish(self, body):
        """The body's lines; labels nothing jumps at are left out."""
        parts, cur = [], []
        for b in body:
            if b[0] == "split":
                parts.append((cur, b))
                cur = []
            else:
                cur.append(b)
        parts.append((cur, None))
        lines = []
        for part, split in parts:
            used = set()
            for b in part:
                if b[0] == "code":
                    for mm in re.finditer(r"goto (\w+);", b[1]):
                        used.add(mm.group(1))
            uses_m = False
            for b in part:
                if b[0] == "label":
                    if b[1] in used:
                        lines.append("%s:" % b[1])
                elif b[0] == "comment":
                    lines.append("    /* %s */" % clean(b[1]) if b[1] else "")
                else:
                    code, cm = b[1], b[2]
                    uses_m = uses_m or "(" in code
                    text = "    " + code
                    if cm:
                        text = "%-43s /* %s */" % (text, clean(cm))
                    lines.append(text)
            # a label at the very end needs a statement
            if lines and lines[-1].endswith(":"):
                lines.append("    ;")
            if not uses_m:
                lines.append("    (void)m;")
            if split:
                name, addr = split[1], split[2]
                lines.append("}\n")
                lines.append("/* %s (%04X): the rest of the routine */" % (name, addr))
                lines.append("void rb_%s(rb_machine *m)\n{" % name.lower())
                self.extra_protos.append(("rb_" + name.lower(), name, addr))
        return lines

    extra_protos = []

    def write_header(self, path, protos):
        with open(path, "w", encoding="ascii", newline="\n") as f:
            f.write("/* game.h - the routines of the Red Baron program (written by tools/mac2c.py).\n *\n"
                    " * rb_xxx is the routine XXX of the source; the address is where it stands in\n"
                    " * the ROMs. The registers go in and out through m->r, as on the 6502.\n */\n"
                    "#ifndef RB_GAME_H\n#define RB_GAME_H\n\n#include \"core/machine.h\"\n\n")
            last = None
            for cn, name, addr, fn in protos:
                if fn != last:
                    f.write("\n/* %s.c */\n" % fn)
                    last = fn
                f.write("%-46s /* %04X */\n" % ("void %s(rb_machine *m);" % cn, addr))
            f.write("\n/* cut out of a routine */\n")
            for cn, name, addr in self.extra_protos:
                f.write("%-46s /* %04X */\n" % ("void %s(rb_machine *m);" % cn, addr))
            f.write("\n#include \"game/frame.h\"\n\n#endif\n")

    def write_symbols(self, path):
        seen = {}
        groups = {}
        for m in self.asm.mods:
            for k, v in m.all.items():
                if (m.name, k) not in self.sym:
                    continue
                cn, v, spell = self.sym[(m.name, k)]
                if cn in seen:
                    if seen[cn] != v:
                        sys.stderr.write("mac2c: %s has two values (%X, %X)\n" % (cn, seen[cn], v))
                    continue
                seen[cn] = v
                groups.setdefault(m.name, []).append((v, cn, spell))
        with open(path, "w", encoding="ascii", newline="\n") as f:
            f.write("/* rb_symbols.h - written by tools/mac2c.py. Do not edit.\n *\n"
                    " * Every symbol of the Red Baron source (upstream, the release of\n"
                    " * 14-SEP-81, MAME's set redbaron) under its original name: RAM cells, hardware\n"
                    " * addresses, constants, and the labels of the program and its tables in the\n"
                    " * ROMs. '.' in a name is '_', '$' is dropped.\n */\n"
                    "#ifndef RB_SYMBOLS_H\n#define RB_SYMBOLS_H\n")
            for m in self.asm.mods:
                rows = sorted(groups.get(m.name, []))
                if not rows:
                    continue
                f.write("\n/* %s */\n" % m.name)
                for v, cn, spell in rows:
                    f.write("#define RB_%-10s 0x%04X\n" % (cn, v & 0xFFFF))
            f.write("\n#endif\n")


def is_loc(lb):
    """A local label (10$), as mac65 hands it on: with its scope (10$@7)."""
    return "$@" in lb


def own_next(code, own, i):
    """The function the next instruction after statement i belongs to."""
    j = i + 1
    while j < len(code) and code[j].kind not in ("ins", "data"):
        j += 1
    if j < len(code) and code[j].kind == "ins":
        return own[j]
    return None


def main():
    t = Translator()
    files = t.write()
    nfun = sum(len(s) for s in t.starts.values())
    print("mac2c: %d routines in %d files" % (nfun, len(files)))
    return 0


if __name__ == "__main__":
    sys.exit(main())
