# Arduino UNO + LCD1602 + джойстик + зуммер. Рендер:
#   uv run --with cairosvg python docs/wiring/uno.py
# Пишет uno.svg (и uno.png для просмотра) рядом с этим файлом.

import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).parent))
from wokwi_style import BLK, BLU, GRN, RED, YEL, Scene  # noqa: E402

ORG = '#e8590c'  # контраст и звук
OUT = Path(__file__).with_suffix('.svg')
s = Scene(1400, 980)
s.text(40, 46, 'LCD Game Box на Arduino UNO: подключение', size=24, weight='bold')
s.text(40, 72, 'Красный +5 В · чёрный GND · зелёный шина LCD · синий джойстик · оранжевый контраст и звук',
       size=13, color='#495057')
P = s.pins
PITCH = 24

# --- Arduino UNO, повёрнута: цифровой разъём снизу -----------------------------------
BX, BY, BW, BH = 360, 150, 600, 300
s.pcb(BX, BY, BW, BH, '#00878f', holes=[(BX + 20, BY + 20), (BX + BW - 20, BY + BH - 20), (BX + 20, BY + BH - 60)])
s.add(f"<rect x='{BX + BW - 36}' y='{BY + 70}' width='60' height='56' rx='4' fill='#b8bec7' stroke='#7d8590'/>"
      f"<rect x='{BX + BW - 30}' y='{BY + 190}' width='52' height='60' rx='6' fill='#2b2b2b'/>")
s.chip(BX + 170, BY + 130, 260, 44, 'ATmega328P')
s.text(BX + 300, BY + 210, 'Arduino UNO', size=16, weight='bold', color='white', anchor='middle', layer='parts')

X0 = BX + 60
digital = ['0', '1', '2', '3', '4', '5', '6', '7']
digital2 = ['8', '9', '10', '11', '12', '13', 'GND', 'AREF']
BOT = BY + BH - 16
s.header(X0, BOT, digital, PITCH, False, 'above', bold={'2', '3', '4', '5'}, prefix='u.')
s.header(X0 + 8 * PITCH + 16, BOT, digital2, 30, False, 'above', bold={'8', '9', '11', '12'}, prefix='u.')
TOP = BY + 16
analog = ['A5', 'A4', 'A3', 'A2', 'A1', 'A0']
power = ['VIN', 'GND', 'GND ', '5V', '3V3', 'RST', 'IORF']
s.header(X0 + 10, TOP, analog, PITCH, False, 'below', bold={'A0', 'A1'}, prefix='u.')
s.header(X0 + 10 + 6 * PITCH + 24, TOP, power, 30, False, 'below', bold={'5V', 'GND'}, prefix='u.')

# --- LCD1602, выводы сверху -------------------------------------------------------------
LX, LY, LW, LH = 200, 640, 640, 270
s.pcb(LX, LY, LW, LH, '#2f7d32', rx=6, holes=[(LX + 16, LY + 16), (LX + LW - 16, LY + 16),
                                               (LX + 16, LY + LH - 16), (LX + LW - 16, LY + LH - 16)])
s.add(f"<rect x='{LX + 50}' y='{LY + 60}' width='{LW - 100}' height='{LH - 100}' rx='6' fill='#1b1b1b'/>"
      f"<rect x='{LX + 66}' y='{LY + 74}' width='{LW - 132}' height='{LH - 128}' rx='3' fill='#9ec33b'/>")
for row, line in enumerate(['  LCD GAME BOX  ', ' < SUPER MARIO >']):
    for c, ch in enumerate(line):
        cx, cy = LX + 76 + c * 30.5, LY + 84 + row * 52
        s.add(f"<rect x='{cx}' y='{cy}' width='26' height='44' fill='#8fb332'/>")
        if ch != ' ':
            s.text(cx + 13, cy + 34, ch, size=30, weight='bold', color='#1e3a0b', anchor='middle', layer='parts')
lcd = ['VSS', 'VDD', 'V0', 'RS', 'RW', 'E', 'D0', 'D1', 'D2', 'D3', 'D4', 'D5', 'D6', 'D7', 'A', 'K']
LPX = X0 + 2 * PITCH - 10 * PITCH  # D4…D7 точно под D2…D5 платы: провода прямые
s.header(LPX, LY + 18, lcd, PITCH, False, 'below', bold={'RS', 'E', 'D4', 'D5', 'D6', 'D7'}, prefix='l.')
s.text(LX + LW - 20, LY + LH - 12, 'LCD1602 (HD44780)', size=11, weight='bold', color='white', anchor='end', layer='parts')

# --- шина LCD --------------------------------------------------------------------------
for lp, up in zip(['D4', 'D5', 'D6', 'D7'], ['2', '3', '4', '5']):
    s.wire([P['u.' + up], P['l.' + lp]], GRN)
RS_ROW, E_ROW = 520, 540
s.wire([P['u.12'], (P['u.12'][0], RS_ROW), (P['l.RS'][0], RS_ROW), P['l.RS']], GRN)
s.wire([P['u.11'], (P['u.11'][0], E_ROW), (P['l.E'][0], E_ROW), P['l.E']], GRN)

# --- питание: две шины слева и справа, сверху перемычка ----------------------------------
L5, LG = 110, 140    # левые шины: +5 В снаружи, GND внутри
R5, RG = 1320, 1290  # правые шины
TOP5, TOPG = 100, 120
u5, ug = P['u.5V'], P['u.GND']
s.wire([u5, (u5[0], TOP5)], RED)
s.wire([ug, (ug[0], TOPG)], BLK)
s.wire([(L5, 930), (L5, TOP5), (R5, TOP5), (R5, 930)], RED)
s.wire([(LG, 930), (LG, TOPG), (RG, TOPG), (RG, 930)], BLK)
s.dot(u5[0], TOP5, RED)
s.dot(ug[0], TOPG, BLK)
s.text(L5 - 8, 940, '+5 В', size=11, weight='bold', color=RED, anchor='middle')
s.text(LG + 14, 940, 'GND', size=11, weight='bold', color=BLK, anchor='middle')

# VSS, RW → GND; VDD → +5 В
vss, vdd, rw = P['l.VSS'], P['l.VDD'], P['l.RW']
s.wire([(LG, 612), (vss[0], 612), vss], BLK)
s.dot(LG, 612, BLK)
s.wire([(L5, 626), (vdd[0], 626), vdd], RED)
s.dot(L5, 626, RED)
s.wire([rw, (rw[0], 612), (vss[0], 612)], BLK)
s.dot(vss[0], 612, BLK)

# Подстроечный резистор контраста: крайние выводы на шины, средний — на V0
PX, PY = 190, 450
s.add(f"<rect x='{PX}' y='{PY}' width='70' height='60' rx='6' fill='#1c7ed6' stroke='#1864ab'/>"
      f"<circle cx='{PX + 35}' cy='{PY + 30}' r='18' fill='#e7f5ff' stroke='#1864ab' stroke-width='2'/>"
      f"<line x1='{PX + 35}' y1='{PY + 16}' x2='{PX + 35}' y2='{PY + 44}' stroke='#1864ab' stroke-width='4'/>", 'over')
s.text(PX + 35, PY - 10, '10 кОм', size=11, anchor='middle')
s.text(PX + 35, PY - 26, 'контраст', size=11, anchor='middle')
pg, pw, p5 = (PX + 12, PY + 60), (PX + 35, PY + 60), (PX + 58, PY + 60)
s.wire([pg, (pg[0], 545), (LG, 545)], BLK)
s.dot(LG, 545, BLK)
s.wire([p5, (p5[0], 575), (L5, 575)], RED)
s.dot(L5, 575, RED)
v0 = P['l.V0']
s.wire([pw, (pw[0], 560), (v0[0], 560), v0], ORG)

# Подсветка: A через 220 Ом к +5 В, K к GND
a, k = P['l.A'], P['l.K']
s.wire([k, (k[0], 628), (RG, 628)], BLK)
s.dot(RG, 628, BLK)
s.wire([a, (a[0], 606), (1150, 606)], RED)
s.resistor((1150, 606), (1250, 606), '220 Ом (подсветка)')
s.wire([(1250, 606), (R5, 606)], RED)
s.dot(R5, 606, RED)

# --- джойстик справа сверху ---------------------------------------------------------------
JX, JY = 1100, 180
s.pcb(JX, JY, 190, 170, '#222831', rx=8)
s.add(f"<circle cx='{JX + 105}' cy='{JY + 85}' r='54' fill='#3a3f47' stroke='#11151a' stroke-width='3'/>"
      f"<circle cx='{JX + 105}' cy='{JY + 85}' r='34' fill='#111' stroke='#555' stroke-width='3'/>"
      f"<circle cx='{JX + 105}' cy='{JY + 85}' r='12' fill='#444'/>")
s.header(JX + 14, JY + 30, ['GND', '+5V', 'VRx', 'VRy', 'SW'], 26, True, 'right',
         bold={'GND', '+5V', 'VRx', 'VRy', 'SW'}, prefix='j.')
s.text(JX + 105, JY + 162, 'джойстик KY-023', size=11, weight='bold', color='white', anchor='middle', layer='parts')
jg, j5, jx, jy, jsw = P['j.GND'], P['j.+5V'], P['j.VRx'], P['j.VRy'], P['j.SW']
s.wire([jg, (1075, jg[1]), (1075, 142), (RG, 142)], BLK)
s.dot(RG, 142, BLK)
s.wire([j5, (1062, j5[1]), (1062, 134), (R5, 134)], RED)
s.dot(R5, 134, RED)
a0, a1 = P['u.A0'], P['u.A1']
s.wire([a0, (a0[0], 132), (1049, 132), (1049, jx[1]), jx], BLU)
s.wire([a1, (a1[0], 124), (1036, 124), (1036, jy[1]), jy], BLU)
d8 = P['u.8']
s.wire([d8, (d8[0], 490), (1023, 490), (1023, jsw[1]), jsw], BLU)

# --- зуммер справа снизу ----------------------------------------------------------------
ZX, ZY = 1120, 790
s.add(f"<circle cx='{ZX}' cy='{ZY}' r='44' fill='#1e1e1e'/><circle cx='{ZX}' cy='{ZY}' r='8' fill='#555'/>", 'over')
s.text(ZX, ZY + 66, 'пассивный зуммер', size=11, weight='bold', anchor='middle')
zp, zm = (ZX - 16, ZY - 44), (ZX + 16, ZY - 44)
s.text(zp[0] - 6, zp[1] + 14, '+', size=12, weight='bold', color='white', anchor='end', layer='over')
d9 = P['u.9']
s.wire([d9, (d9[0], 500), (1000, 500), (1000, 680)], ORG)
s.resistor((1000, 680), (zp[0], 680), '100 Ом')
s.wire([(zp[0], 680), zp], ORG)
s.wire([zm, (zm[0], 700), (RG, 700)], BLK)
s.dot(RG, 700, BLK)

s.note(40, 960, ['RW (5) и VSS (1) дисплея — на GND. D0…D3 не подключаются: дисплей работает в 4-битном режиме.'])
s.save(OUT, 'LCD Game Box на Arduino UNO')
