#!/usr/bin/env python

import csv

from sys import exit

def set_lut(lut, d, did):
    global did2sid
    global did2name
    if did not in did2sid:
        if did[0] != 'U': # Tubas don't have hats yet
            print "slave id not found for", did
        return
    if d>=255:
        print "delay constant too large:", d
        return
    sid = did2sid[did]
    if sid == 999:
        print "No board: {:3s} {}".format(did, did2name[did])
    else:
        lut[sid] = d

        
def print_lut(lut, var_name):
    lut = [str(i) for i in lut]
    print "const uint8_t ",var_name,"[] PROGMEM = {"
    print ", ".join(lut), "};"



from argparse import ArgumentParser
parser = ArgumentParser()
parser.add_argument('-d', dest="debug", action='count', help='Increase debug level.')
parser.add_argument('-m', dest="missing", action='count', help='Print out missing boards.')
parser.add_argument('-l', dest="list", action='count', help='Just list marchers.')
args = parser.parse_args()

# Find slave id from drill id
did2sid={}
# drill id from slave id
sid2did = {}
# Find marcher name from drill id
did2name = {}
# Find hat number from drill id
did2hid = {}
with open("hatList.csv", "rU") as fh:
    title=fh.next()
    reader = csv.DictReader(fh);
    for row in reader:
        try:
            sid = row['Circuit Board #']
            if sid == 'xxx':
                sid = 999
            sid = int(sid)
            did = row['Drill ID #']
            if args.debug>1:
                print row['First Name'], row['Last Name'], did, sid
            did2sid[did] = sid
            sid2did[sid] = did
            did2name[did] = row['First Name'] + " " + row['Last Name']
            did2hid[did] = row['Hat #']
        except:
            pass    

bench=[["S157", 157, 157, "X1"],
       ["S167", 167, 167, "X2"],
       ["S006",   6,   6, "X3"],
       ["S007",   7,   7, "X4"],
       ["S008",   8,   8, "X5"],
       ["S005",   5,   5, "X6"],
       ["S083",  83,  83, "X7"],
       ["S017",  17,  17, "X8"]
       ]

for h in bench:
    sid = int(h[2])
    did = h[3]
    did2sid[did] = sid
    sid2did[sid] = did
    did2name[did] = h[0]
    did2hid[did] = int(h[1])
        
if args.list==1:
    for did in sorted(did2sid.keys()):
        print "{:3s} {:3d} {}".format(did, did2sid[did], did2name[did])
    exit()
elif args.list==2:
    for sid in sorted(did2sid.values()):
        if sid == 999: continue
        print "{:3s} {:3d} {}".format(sid2did[sid], sid, did2name[sid2did[sid]])
    exit()
    

if args.missing:
    for did,sid in did2sid.iteritems():
        if sid==999:
            print "{:3s} {:3d} {}".format(did, sid, did2name[did])
    exit()

    
bench_ww = ['X1', 'X2', 'X3', 'X4']
bench_brass = ['X5', 'X6', 'X7', 'X8']

# Woodwinds set the hats down set 4 count 56
woodwind1 = [
    'F10', 'F13', 'F7', 'F11', 'F2', 'F8', 'F17', 'F14', 'F12', 'F18', 'F9',
    'F6', 'F19', 'F5', 'F20', 'F4', 'F16', 'F3', 'F15', 'F23', 'F22', 'F21',
    'S4', 'A1', 'A3', 'S2', 'A2', 'A12', 'S1', 'S3', 'A7', 'S7', 'A5', 'A6',
    'S8', 'S6', 'A8', 'A10', 'A4', 'A9', 'S5', 'A11', 'C3', 'C1', 'C22', 'C4',
    'C15', 'C16', 'C2', 'C17', 'C8', 'C23', 'C14', 'C12', 'C24', 'C5', 'C20',
    'C10', 'C9', 'C6', 'C13', 'C11', 'C7'
    ]

# Brass set the hats down set 4  count 66
brass1 = [
    'B7', 'B25', 'B21', 'B15', 'B10', 'B9', 'B24', 'B20', 'B2',
    'B14', 'B6', 'B5', 'B22', 'B13', 'B8', 'B4', 'B17', 'B26', 'B18', 'B23', 'B15', 'B3', 'B19',
    'M9', 'M7', 'M4', 'M8', 'M10', 'M2', 'M5', 'M3', 'M1', 'M6',
    'T14', 'T13', 'T15', 'T17', 'T16', 'T3', 'T4', 'T6', 'T1', 'T2', 'T5', 'T5', 'T9',
    'T7', 'T12', 'T8', 'T19', 'T11', 'T10', 'T20', 'T18'
    ]

ww1_left=woodwind1[0:22]
ww1_left.reverse();
ww1_right = woodwind1[22:]


max_sid=175
delay_lsb = 25 # ms per count of delay

# everyone R->L 
# woodwinds take 5 seconds 
# brass take 5 seconds delayed by 2 seconds (100) counts
lut=[0xff for i in range(max_sid)]

inc = 5000 / delay_lsb / len(woodwind1)
d=0;
for did in reversed(woodwind1):
    set_lut(lut, d, did)
    d+=inc

inc = 5000 / delay_lsb / len(brass1)
d=1000/delay_lsb;
for did in reversed(brass1):
    set_lut(lut, d, did)
    d+=inc

inc = 2000 / delay_lsb / len(bench_ww)
d=0;
for did in reversed(bench_ww):
    set_lut(lut, d, did)
    d+=inc

inc = 2000 / delay_lsb / len(bench_brass)
d=2000/delay_lsb
for did in reversed(bench_brass):
    set_lut(lut, d, did)
    d+=inc

print_lut(lut, "all_l2r")


m1_set13 = [
    [ 'U7',  'U6',  'U4',  'U2',   'S8',  'S7',  'A6',  'A5',  'X1' ],
    [ 'U10', 'U5',  'U3',  'U1',   'A4',  'S6',  'A8',  'A7',       ],
    [ 'U8',  'B21', 'B22', 'B23',  'A3',  'S5',  'A10', 'A9',  'X2' ],
    [ 'U9',  'B24', 'B25', 'B26',  'A2',  'S4',  'A12', 'A11',      ],
    [ 'B13', 'B9',  'B15', 'B18',  'A1',  'S3',  'S2',  'S1',  'X3' ],
    [ 'B11', 'B14', 'B20', 'B17',  'C24', 'C23', 'C22', 'F21',      ],
    [ 'B12', 'B10', 'B15', 'B19',  'C20', 'F22', 'F23', 'C17', 'X4' ],
    [ 'T13', 'B8',  'B6',  'B4',   'C4',  'C3',  'C2',  'C1',       ],
    [ 'T14', 'B7',  'B5',  'B3',   'C7',  'C8',  'C9',  'C10', 'X5' ],
    [ 'T15', 'T4',  'T8',  'T12',  'C6',  'C16', 'C15', 'C11',      ],
    [ 'T16', 'T3',  'T7',  'T11',  'C5',  'C13', 'C14', 'C12', 'X6' ],
    [ 'T17', 'T2',  'T6',  'T10',  'F12', 'F11', 'F10', 'F9',       ],
    [ 'T18', 'T1',  'T5',  'T9',   'F1',  'F8',  'F13', 'F17', 'X7' ],
    [ 'T20', 'T19', 'M6',  'M5',   'F2',  'F7',  'F14', 'F18',      ],
    [ 'M7',  'M9',  'M4',  'M3',   'F3',  'F6',  'F15', 'F19', 'X8' ],
    [ 'M8',  'M10', 'M2',  'M1',   'F4',  'F5',  'F16', 'F20']
    ]
dt = 1125 / len(m1_set13) / delay_lsb
lut=[0xff for i in range(max_sid)]
d=0
for line in m1_set13:
    for did in line:
        set_lut(lut, d, did)
    d+=dt
print_lut(lut, "m1_set13")



# from L -> R, Back to Front
m3_set21 = [
    ['B22', 'B24', 'B1',  'B23', 'B26', 'B6',  'B2',  'B8',  'B15', 'B4',  'B21', 'M9',  'M7',
     'B25', 'B19', 'M1',  'M6',  'M5',  'M3',  'T8',  'T19', 'T1',  'T15', 'T17', 'T14', 'T13'],
    ['B15', 'B5',  'B3',  'B18', 'B17', 'B14', 'B20', 'B7',  'B9',  'B13', 'B10', 'M10', 'M2', 'M4',
     'M8',  'T10', 'T11', 'T18', 'T20', 'T7',  'T9',  'T12', 'T5',  'T4',  'T6',  'T16', 'T3'],
    ['U8', 'U9', 'U2', 'U6', 'U4', 'U7', 'U1', 'U10', 'U5', 'U3',
     'Q3', 'Q2', 'Q1', 'SN1', 'SN2', 'SN3', 'SN4', 'SN5', 'O1', 'O2', 'O3', 'O4'],
    ['C7',  'C11', 'C13', 'C6',  'C9',  'C10', 'C20', 'C5',  'C24', 'C12', 'C14', 'C23', 'C8',  'C17', 'C2',  'C16',
     'C15', 'C4',  'C22', 'C1',  'C3',  'A11', 'S5',  'A9',  'A4',  'A10', 'A8',  'S6',  'S8',  'A6',  'A5',  'S7'],
    ['F10', 'F13', 'F7',  'F11', 'F2',  'F8',  'F17', 'F14', 'F12', 'F18', 'F9',  'F6',  'F19', 'F5',  'F20', 'F4',
     'F16', 'F3',  'F15', 'F23', 'F22', 'F21', 'F1',  'S4',  'A1',  'A3',  'S2',  'A2',  'A12', 'S1',  'S3',  'A7'],
    ['X1','X2','X3','X4','X5','X6','X7','X8']
    ]
# For effect 9, wave from L->R
bpm = 170
beats = 8
dt = 1000 * 60 * beats / bpm / delay_lsb
lut=[0xff for i in range(max_sid)]
for row in m3_set21:
    d=0
    row.reverse()
    for did in row:
        set_lut(lut, d, did) 
        d += dt / len(row)
print_lut(lut, "m3_set21")



# From L to right
m3_set30_A = [
    ['C13', 'C11', 'C7',  'F10', 'C6',  'F11', 'F7',  'F13', 'B16', 'B21', ],
    ['C14', 'C12', 'C24', 'F12', 'C23', 'F6',  'F9',  'F18'],
    ['B1',  'B24', 'B22', 'B15', 'B23', 'B18', 'B3',  'B5'],
    ['B4',  'B15', 'B9',  'M9',  'M10', 'B10', 'B13'],
    ['M3',  'M5',  'M6',  'T11', 'T8',  'T7',  'T20', 'T18'],
    ['T13', 'T14', 'T17', 'T16', 'T6',  'F16', 'C15', 'T3'],
    ['S5',  'A9',  'A4',  'A1',  'A11', 'F21', 'F1',  'S4'],
    ['S7',  'A6',  'A5',  'S1',  'S8',  'A7'],
    ['X1','X2','X3','X4']
    ]

m3_set30_B = [
    ['C5',  'C20', 'C10', 'C9',  'F14', 'F17', 'F8',  'F2', 'B12'],
    ['C16', 'C2',  'C17', 'C8',  'F4',  'F20', 'F5',  'F19'],
    ['B8',  'B2',  'B6',  'B26', 'B7',  'B20', 'B14', 'B17'],
    ['M1',  'B19', 'B25', 'M7',  'T10', 'M8',  'M4',  'M2'],
    ['T15', 'T1',  'T19', 'T2',  'T4',  'T5',  'T12', 'T9',],
    ['C4',  'C22', 'C1',  'C3',  'F3',  'F15', 'F23', 'F22'],
    ['A1',  'A8',  'S6',  'S8',  'A3',  'S2',  'A2',  'A12'],
    ['X5','X6','X7','X8']

    ]

lut=[0xff for i in range(max_sid)]
for row in m3_set30_A:
    for did in row:
        set_lut(lut, 0, did)
for row in m3_set30_B:
    for did in row:
        set_lut(lut, 1, did)

print_lut(lut, "m3_set30_AB")

# 1 = battery
# 2 = woodwind
# 3 = brass
# 100 = test bench
lut=[0xff for i in range(max_sid)]
for did in sorted(did2sid.keys()):
    if did[0] == 'O' or did[0:2] == 'SN' or did[0] == 'Q':
        d=1
    elif did[0] == 'F' or did[0] == 'S' or did[0] == 'C' or did[0] == 'A':
        d=2
    elif did[0] == 'B' or did[0] == 'U' or did[0] == 'M' or did[0] == 'T':
        d=3
    elif did[0] == 'X':
        d=100
    set_lut(lut, d, did);
print_lut(lut, "section")
