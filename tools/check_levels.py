#!/usr/bin/env python3
"""Проверка уровней Mario из src/games/mario/MarioLevels.h.

Для каждого уровня:
  * перебором всех состояний Марио проверяет, что флаг/топор достижим. Правила движения
    повторяют MarioGame::gameTick(): прыжки, пружины, трубы, кирпичи, движущиеся платформы,
    пираньи и огненные шары из лавы (они зависят только от номера шага и учитываются точно).
    Гумбы, купы и Боузер не моделируются;
  * считает нужные спрайты (правила MarioGame::usedSprites()): их должно быть не больше 7;
  * предупреждает о врагах, которые ходят под кирпичами, где их нельзя перепрыгнуть.

Запуск: python3 tools/check_levels.py   (код возврата 1 — есть проблемы)
"""
import re
import sys
from math import lcm
from pathlib import Path

LEVELS_H = Path(__file__).resolve().parent.parent / "src/games/mario/MarioLevels.h"
TYPES_H = Path(__file__).resolve().parent.parent / "src/games/mario/MarioTypes.h"


def constant(src, name):
    return int(re.search(rf"constexpr \w+ {name} = (\d+);", src).group(1))


types = TYPES_H.read_text()
AIR = constant(types, "AIR_TICKS")
SPRING_AIR = constant(types, "SPRING_AIR")
CAM_LEAD = constant(types, "CAM_LEAD")
LIFT_EVERY = constant(types, "LIFT_EVERY")
PLANT_PERIOD = constant(types, "PLANT_PERIOD")
PLANT_UP = constant(types, "PLANT_UP")
POD_PERIOD = constant(types, "POD_PERIOD")
MAX_LEVEL_LEN = constant(types, "MAX_LEVEL_LEN")


def load_levels():
    src = LEVELS_H.read_text()
    strings = dict(re.findall(r'const char (\w+)\[\] PROGMEM =\s*"([^"]*)";', src))
    rows = re.findall(r"\{(\w+), (\w+), (\d+), (\d+), (\d+), (true|false), (true|false)\}", src)
    return [(f"{w}-{n}", strings[t], strings[b], c == "true", f == "true") for t, b, w, n, _, c, f in rows]


def sprites(top, bot, castle, fire):
    s = {"GROUND"}
    for c in top:
        s |= {"o": {"COIN"}, "?": {"QBLOCK", "USED", "COIN"}, "M": {"QBLOCK", "USED", "MUSHROOM"},
              "#": {"BRICK"}}.get(c, set())
    for c in bot:
        s |= {"P": {"PIPE"}, "T": {"PIPE", "PLANT"}, "G": {"GOOMBA"}, "K": {"KOOPA", "SHELL"},
              "L": {"LIFT"}, "S": {"SPRING"}, "f": {"FIRE"}, "B": {"BOWSER"}, "A": {"AXE"}}.get(c, set())
    if castle and (" " in bot or "f" in bot):
        s.add("LAVA")
    if fire and "B" in bot:
        s.add("FIRE")
    return s


def lifts_of(bot):
    res, i = [], 0
    while i < len(bot):
        if bot[i] != "L":
            i += 1
            continue
        j = i
        while j < len(bot) and bot[j] == "L":
            j += 1
        lo = i
        while lo > 0 and bot[lo - 1] == " ":
            lo -= 1
        hi = j - 1
        while hi + 1 < len(bot) and bot[hi + 1] == " ":
            hi += 1
        res.append((lo, hi, j - i, i - lo))
        i = j
    return res


def lift_pos(lift, t):
    lo, hi, w, o0 = lift
    m = hi - lo + 1 - w + 1
    if m <= 1:
        return lo
    cyc = 2 * (m - 1)
    k = (t // LIFT_EVERY + o0) % cyc
    return lo + (k if k < m else cyc - k)


def check(name, top, raw, castle, fire):
    problems, warnings = [], []
    n = len(raw)
    if len(top) != n:
        problems.append(f"ряды разной длины: {len(top)} и {n}")
        return problems, warnings
    if n > MAX_LEVEL_LEN:
        problems.append(f"длина {n} больше MAX_LEVEL_LEN = {MAX_LEVEL_LEN}")
    sp = sprites(top, raw, castle, fire)
    if len(sp) > 7:
        problems.append(f"нужно {len(sp)} спрайтов, помещается 7: {sorted(sp)}")

    # Нижний ряд без врагов и платформ — как bottomRow в прошивке
    bot = "".join("_" if c in "GK" else "=" if c == "B" else " " if c == "L" else c for c in raw)
    goals = [i for i, c in enumerate(bot) if c in "|A"]
    if len(goals) != 1:
        problems.append("должен быть ровно один флагшток '|' или топор 'A'")
        return problems, warnings
    goal = goals[0]
    lifts = lifts_of(raw)
    period = lcm(PLANT_PERIOD, POD_PERIOD,
                 *[LIFT_EVERY * 2 * max(1, hi - lo + 1 - w) for lo, hi, w, _ in lifts])
    blk = lambda c: c in "?M#"

    def lift_at(x, t):
        return any(lift_pos(l, t) <= x < lift_pos(l, t) + l[2] for l in lifts)

    def solid(x, t):
        return bot[x] not in " f" or lift_at(x, t)

    def plant_up(x, t):
        return bot[x] == "T" and (t + x * 3) % PLANT_PERIOD < PLANT_UP

    def pod_row(x, t):
        if bot[x] != "f":
            return -1
        ph = (t + x * 5) % POD_PERIOD
        return 1 if ph in (0, 4) else 0 if 1 <= ph <= 3 else -1

    def step(state, dx, jump):
        x, y, air, t, cam = state
        t1 = t + 1
        if y == 1:  # платформы везут Марио
            for l in lifts:
                a, b = lift_pos(l, t), lift_pos(l, t1)
                if a != b and a <= x < a + l[2]:
                    if x + (b - a) >= cam:
                        x += b - a
                    break
        jumped = False
        grounded = (y == 1 and solid(x, t1)) or (y == 0 and bot[x] in "PT")
        if jump and grounded and not (y == 1 and blk(top[x])):
            air = SPRING_AIR if (y == 1 and bot[x] == "S") else AIR
            y, jumped = 0, True
        if dx:
            nx = x + dx
            if cam <= nx < n and not (y == 1 and bot[nx] in "PT") and not (y == 0 and blk(top[nx])):
                x = nx
                if y == 0 and plant_up(x, t1) or pod_row(x, t1) == y:
                    return None
        if y == 0 and not jumped:
            if bot[x] in "PT":
                air = 0
            elif air > 0:
                air -= 1
            else:
                y = 1
        if y == 1 and not solid(x, t1):
            return None
        if y == 0 and plant_up(x, t1) or pod_row(x, t1) == y:
            return None
        return (x, y, air, t1 % period, max(cam, x - CAM_LEAD))

    start = (1, 1, 0, 0, 0)
    seen, stack, best = {start}, [start], 1
    while stack:
        s = stack.pop()
        for dx in (-1, 0, 1):
            for jump in (False, True):
                nxt = step(s, dx, jump)
                if nxt and nxt not in seen:
                    seen.add(nxt)
                    stack.append(nxt)
                    best = max(best, nxt[0])
    if best < goal:
        problems.append(f"до цели (клетка {goal}) не дойти: застревает на клетке {best}")

    for gx, c in enumerate(raw):
        if c not in "GK":
            continue
        lo = gx
        while lo > 0 and bot[lo - 1] in "_=":
            lo -= 1
        hi = gx
        while hi < n - 1 and bot[hi + 1] in "_=":
            hi += 1
        roof = sum(blk(top[i]) for i in range(lo, hi + 1))
        if roof > (hi - lo + 1) // 2:
            warnings.append(f"враг '{c}' на клетке {gx} ходит под кирпичами ({roof} из {hi - lo + 1})")
    return problems, warnings


def main():
    ok = True
    for name, top, bot, castle, fire in load_levels():
        problems, warnings = check(name, top, bot, castle, fire)
        status = "OK" if not problems else "ОШИБКА"
        print(f"{name}: {status} (длина {len(bot)}, спрайтов {len(sprites(top, bot, castle, fire))})")
        for p in problems:
            print(f"   ✗ {p}")
        for w in warnings:
            print(f"   ! {w}")
        ok &= not problems
    sys.exit(0 if ok else 1)


if __name__ == "__main__":
    main()
