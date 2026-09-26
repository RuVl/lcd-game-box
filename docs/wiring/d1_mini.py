# Wemos D1 mini (ESP8266) + 74HC595 + LCD1602 + джойстик через диодный мультиплексор + зуммер.
# Рендер:
#   uv run --with cairosvg python docs/wiring/d1_mini.py
# Пишет d1_mini.svg (и d1_mini.png для просмотра) рядом с этим файлом.

import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).parent))
from wokwi_style import BLK, BLU, GRN, RED, Scene  # noqa: E402

ORG = '#e8590c'  # контраст и звук
MAG = '#c2255c'  # 3,3 В
TEAL = '#0c8599'  # узлы мультиплексора осей
OUT = Path(__file__).with_suffix('.svg')
s = Scene(1500, 1010)
s.text(40, 46, 'LCD Game Box на Wemos D1 mini (ESP8266): подключение', size=24, weight='bold')
s.text(40, 72, 'Шина LCD через 74HC595 · оси джойстика по очереди на A0 через диоды · '
       'питание обозначено метками: одинаковые метки соединены', size=13, color='#495057')
P = s.pins

TAG = {'5V': RED, '3V3': MAG, 'GND': BLK}


def tag(pin, name, dx=0, dy=0):
    """Короткий провод от вывода к метке питания (как power flag в Wokwi/KiCad)."""
    x, y = pin
    ex, ey = x + dx, y + dy
    col = TAG[name]
    s.wire([(x, y), (ex, ey)], col, width=3)
    w = 12 + 8 * len(name)
    s.add(f"<rect x='{ex - w / 2:.1f}' y='{ey - 10:.1f}' width='{w}' height='20' rx='10' fill='{col}'/>"
          f"<text x='{ex:.1f}' y='{ey + 4:.1f}' font-size='11' font-weight='bold' fill='white' "
          f"text-anchor='middle' font-family='Inter, Helvetica, Arial, sans-serif'>{name}</text>", 'over')


def ground(pin, dy=-34):
    """Короткий провод к значку земли — для тесных мест, где метка не помещается."""
    x, y = pin
    ey = y + dy
    s.wire([(x, y), (x, ey)], BLK, width=3)
    sign = -1 if dy < 0 else 1
    for i, w in enumerate((18, 12, 6)):
        yy = ey + sign * i * 4
        s.add(f"<line x1='{x - w / 2}' y1='{yy}' x2='{x + w / 2}' y2='{yy}' stroke='{BLK}' stroke-width='2.5'/>", 'over')


# --- Wemos D1 mini, USB вниз --------------------------------------------------------------
BX, BY, BW, BH = 860, 170, 205, 274
s.pcb(BX, BY, BW, BH, '#1b4f9c', holes=[(BX + 14, BY + 14), (BX + BW - 14, BY + 14)])
s.add(f"<rect x='{BX + 48}' y='{BY + 18}' width='108' height='96' rx='4' fill='#c7ccd4' stroke='#8d949e'/>"
      f"<path d='M{BX + 56} {BY + 26} h18 v10 h-10 v10 h10 v10 h-10 v10 h10 v10 h-18' fill='none' stroke='#8d949e' stroke-width='2'/>"
      f"<text x='{BX + 104}' y='{BY + 70}' font-size='10' fill='#555' text-anchor='middle' font-family='sans-serif'>ESP-12F</text>"
      f"<rect x='{BX + BW / 2 - 22}' y='{BY + BH - 14}' width='44' height='22' rx='4' fill='#b8bec7' stroke='#7d8590'/>")
s.chip(BX + 70, BY + 150, 44, 30, 'CH340')
s.text(BX + BW / 2, BY + 230, 'WeMos D1 mini', size=12, weight='bold', color='white', anchor='middle', layer='parts')
PITCH = 22
left = ['RST', 'A0', 'D0', 'D5', 'D6', 'D7', 'D8', '3V3']
right = ['TX', 'RX', 'D1', 'D2', 'D3', 'D4', 'GND', '5V']
s.header(BX + 14, BY + 26, left, PITCH, True, 'right', bold={'A0', 'D5', 'D6', 'D7', 'D8', '3V3'}, prefix='b.')
s.header(BX + BW - 14, BY + 26, right, PITCH, True, 'left', bold={'D1', 'D2', 'D3', 'GND', '5V'}, prefix='b.')
tag(P['b.3V3'], '3V3', dx=-40)
tag(P['b.GND'], 'GND', dx=60)
tag(P['b.5V'], '5V', dx=60)

# --- 74HC595 ------------------------------------------------------------------------------
SP = 26
CX, CY = 440, 410  # вывод 16 (VCC) — левый верхний
s.text(CX + 7 * SP + 30, CY + 42, '74HC595', size=12, weight='bold')
s.add(f"<rect x='{CX - 18}' y='{CY + 8}' width='{7 * SP + 36}' height='56' rx='4' fill='#2b2b2b'/>"
      f"<circle cx='{CX - 6}' cy='{CY + 36}' r='5' fill='#555'/>")
top = ['VCC', 'Q0', 'DS', 'OE', 'ST_CP', 'SH_CP', 'MR', "Q7'"]   # выводы 16…9
bot = ['Q1', 'Q2', 'Q3', 'Q4', 'Q5', 'Q6', 'Q7', 'GND']          # выводы 1…8
# Подписи выводов — внутри корпуса тёмным по светлому, номера — снаружи.
s.header(CX, CY, top, SP, False, 'above', prefix='c.', labels=False)
s.header(CX, CY + 72, bot, SP, False, 'below', prefix='c.', labels=False)
for n in range(8):
    s.text(CX + n * SP, CY + 22, top[n].replace('_CP', ''), size=8, color='#e9ecef', anchor='middle', layer='parts')
    s.text(CX + n * SP, CY + 58, bot[n], size=8, color='#e9ecef', anchor='middle', layer='parts')
    s.text(CX + n * SP + 9, CY - 8, str(16 - n), size=8, color='#868e96')
    s.text(CX + n * SP + 9, CY + 90, str(n + 1), size=8, color='#868e96')
tag(P['c.VCC'], '3V3', dy=-60)
tag(P['c.MR'], '3V3', dy=-60)
tag(P['c.OE'], 'GND', dy=-44)
tag(P['c.GND'], 'GND', dy=40)

# D5 → SH_CP, D6 → ST_CP, D7 → DS
for chip_pin, board_pin in [('SH_CP', 'D5'), ('ST_CP', 'D6'), ('DS', 'D7')]:
    c, b = P['c.' + chip_pin], P['b.' + board_pin]
    s.wire([c, (c[0], b[1]), b], GRN)

# --- LCD1602 ------------------------------------------------------------------------------
LX, LY, LW, LH = 200, 650, 700, 290
s.pcb(LX, LY, LW, LH, '#2f7d32', rx=6, holes=[(LX + 16, LY + 16), (LX + LW - 16, LY + 16),
                                               (LX + 16, LY + LH - 16), (LX + LW - 16, LY + LH - 16)])
s.add(f"<rect x='{LX + 50}' y='{LY + 70}' width='{LW - 100}' height='{LH - 110}' rx='6' fill='#1b1b1b'/>"
      f"<rect x='{LX + 66}' y='{LY + 84}' width='{LW - 132}' height='{LH - 138}' rx='3' fill='#9ec33b'/>")
for row, line in enumerate(['  LCD GAME BOX  ', ' <   TETRIS   > ']):
    for c, ch in enumerate(line[:16]):
        cx, cy = LX + 78 + c * 34, LY + 94 + row * 56
        s.add(f"<rect x='{cx}' y='{cy}' width='30' height='48' fill='#8fb332'/>")
        if ch != ' ':
            s.text(cx + 15, cy + 36, ch, size=30, weight='bold', color='#1e3a0b', anchor='middle', layer='parts')
lcd = ['VSS', 'VDD', 'V0', 'RS', 'RW', 'E', 'D0', 'D1', 'D2', 'D3', 'D4', 'D5', 'D6', 'D7', 'A', 'K']
LPX = P['c.Q4'][0] - 10 * SP  # D4…D7 точно под Q4…Q7: провода прямые
s.header(LPX, LY + 18, lcd, SP, False, 'below', bold={'RS', 'E', 'D4', 'D5', 'D6', 'D7'}, prefix='l.')
s.text(LX + LW - 20, LY + LH - 12, 'LCD1602 (HD44780)', size=11, weight='bold', color='white', anchor='end', layer='parts')

for q, d in zip(['Q4', 'Q5', 'Q6', 'Q7'], ['D4', 'D5', 'D6', 'D7']):
    s.wire([P['c.' + q], P['l.' + d]], GRN)
s.wire([P['c.Q1'], (P['c.Q1'][0], 548), (P['l.RS'][0], 548), P['l.RS']], GRN)
s.wire([P['c.Q3'], (P['c.Q3'][0], 578), (P['l.E'][0], 578), P['l.E']], GRN)

ground(P['l.VSS'])
tag(P['l.VDD'], '5V', dy=-62)
ground(P['l.RW'])
ground(P['l.K'])
a = P['l.A']
s.wire([a, (a[0], 600), (a[0] + 60, 600)], RED)
s.resistor((a[0] + 60, 600), (a[0] + 160, 600), '220 Ом')
tag((a[0] + 160, 600), '5V', dx=50)

# Подстроечный резистор контраста слева от дисплея
PX, PY = 80, 470
s.add(f"<rect x='{PX}' y='{PY}' width='70' height='60' rx='6' fill='#1c7ed6' stroke='#1864ab'/>"
      f"<circle cx='{PX + 35}' cy='{PY + 30}' r='18' fill='#e7f5ff' stroke='#1864ab' stroke-width='2'/>"
      f"<line x1='{PX + 35}' y1='{PY + 16}' x2='{PX + 35}' y2='{PY + 44}' stroke='#1864ab' stroke-width='4'/>", 'over')
s.text(PX + 35, PY - 26, 'контраст', size=11, anchor='middle')
s.text(PX + 35, PY - 10, '10 кОм', size=11, anchor='middle')
tag((PX + 12, PY + 60), 'GND', dy=40)
tag((PX + 58, PY + 60), '5V', dy=40)
v0 = P['l.V0']
s.wire([(PX + 35, PY + 60), (PX + 35, 592), (v0[0], 592), v0], ORG)

# --- джойстик и диодный мультиплексор ------------------------------------------------------
JX, JY = 1320, 170
s.pcb(JX, JY, 170, 170, '#222831', rx=8)
s.add(f"<circle cx='{JX + 100}' cy='{JY + 85}' r='50' fill='#3a3f47' stroke='#11151a' stroke-width='3'/>"
      f"<circle cx='{JX + 100}' cy='{JY + 85}' r='30' fill='#111' stroke='#555' stroke-width='3'/>")
jpins = ['GND', '+5V', 'VRx', 'VRy', 'SW']
jy0 = P['b.D1'][1] - 2 * PITCH  # VRx на уровне D1, VRy — D2, SW — D3
s.header(JX + 14, jy0, jpins, PITCH, True, 'right', bold=set(jpins), prefix='j.')
s.text(JX + 100, JY + 162, 'джойстик KY-023', size=11, weight='bold', color='white', anchor='middle', layer='parts')
tag(P['j.GND'], 'GND', dx=-40)
tag(P['j.+5V'], '3V3', dx=-40)
s.text(JX + 85, JY + 196, '+5V джойстика — на 3V3!', size=11, weight='bold', color=MAG, anchor='middle')

# SW → D3 напрямую
s.wire([P['j.SW'], P['b.D3']], BLU)

# VRx → 1 кОм → узел X → D1; узел X → 1N4007 → A0. То же для VRy → узел Y → D2.
diode_top = 120
xs = {'X': 1140, 'Y': 1110}  # где диод уходит вверх с узла
for axis, jpin, bpin in [('X', 'VRx', 'D1'), ('Y', 'VRy', 'D2')]:
    jp, bp = P['j.' + jpin], P['b.' + bpin]
    y = bp[1]
    s.resistor((jp[0] - 60, y), (jp[0] - 140, y), '')
    s.wire([jp, (jp[0] - 60, y)], BLU)
    s.wire([(jp[0] - 140, y), (xs[axis], y)], BLU)
    s.wire([(xs[axis], y), bp], TEAL)
    s.dot(xs[axis], y, TEAL)
    s.diode((xs[axis], y - 30), (xs[axis], diode_top + 44), '')
    s.wire([(xs[axis], y), (xs[axis], y - 30)], TEAL)
    s.wire([(xs[axis], diode_top + 44), (xs[axis], diode_top)], BLU)
    s.text(xs[axis] + 8, y - 6, f'узел {axis}', size=10, color=TEAL)
s.text(xs['X'] + 22, diode_top + 70, '2 × 1N4007', size=11, weight='bold')
s.text(xs['X'] + 22, diode_top + 86, 'полоска — к A0', size=10, color='#495057')
s.text(JX - 100, P['j.VRy'][1] + 30, '2 × 1 кОм', size=11, weight='bold', anchor='middle')
# катоды вместе → A0 поверх платы
a0 = P['b.A0']
s.wire([(xs['X'], diode_top), (840, diode_top), (840, a0[1]), a0], BLU)
s.dot(xs['Y'], diode_top, BLU)

# --- зуммер ---------------------------------------------------------------------------------
d8 = P['b.D8']
ZX, ZY = 1010, 570
zp = (ZX - 16, ZY - 40)
s.wire([d8, (800, d8[1]), (800, 490), (870, 490)], ORG)
s.resistor((870, 490), (950, 490), '100 Ом')
s.wire([(950, 490), (zp[0], 490), zp], ORG)
s.add(f"<circle cx='{ZX}' cy='{ZY}' r='40' fill='#1e1e1e'/><circle cx='{ZX}' cy='{ZY}' r='7' fill='#555'/>", 'over')
s.text(ZX + 50, ZY + 4, 'зуммер', size=11, weight='bold')
tag((ZX + 16, ZY - 40), 'GND', dx=60, dy=0)

s.note(40, 975, ['RW дисплея — на GND. D0…D3 не подключаются. LCD питается от 5 В, а 74HC595 — от 3,3 В: '
                 'HD44780 при 5 В читает 3,3 В как единицу.',
                 'Кнопку стика (D3 = GPIO0) не держать при включении: плата уйдёт в режим прошивки.'])
s.save(OUT, 'LCD Game Box на Wemos D1 mini')
