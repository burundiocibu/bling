#!/usr/bin/env python

import csv

from sys import exit

def set_lut(lut, d, did):
    global did2sid
    global did2name
    if did not in did2sid:
        print "slave id not found for", did
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

bench=[["S002",   2,   2, "X1",],
       ["S167", 167, 167, "X3"],
       ["S006",   6,   6, "X5"],
       ["S007",   7,   7, "X7"],
       ["S008",   8,   8, "X9"],
       ["S005",   5,   5, "X11"],
       ["S083",  83,  83, "X13"],
       ["S017",  17,  17, "X15"]
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

    
bench_ww = ['X1', 'X3', 'X5', 'X7']
bench_brass = ['X9', 'X11', 'X13', 'X15']

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

lut=[0xff for i in range(max_sid)]
d=0;
for did in woodwind1+bench_ww:
    set_lut(lut, d, did);
print_lut(lut, "ww_pres");

d=1
for did in brass1+bench_brass:
    set_lut(lut, d, did);
print_lut(lut, "wwb_pres");


# everyone L->R 
# woodwinds take 5 seconds  
# brass take 5 seconds delayed by 2 seconds (100) counts
lut=[0xff for i in range(max_sid)]
delay_lsb = 40 # ms per count of delay

inc = 5000 / 40 / len(woodwind1)
d=0;
for did in reversed(woodwind1):
    set_lut(lut, d, did);
    d+=inc

inc = 5000 / 40 / len(brass1)
d=1000/delay_lsb;
for did in reversed(brass1):
    set_lut(lut, d, did);
    d+=inc

inc = 2000 / 40 / len(bench_ww)
d=0;
for did in reversed(bench_ww):
    set_lut(lut, d, did);
    d+=inc

inc = 2000 / 40 / len(bench_brass)
d=2000/delay_lsb;
d=2000/delay_lsb
for did in reversed(bench_brass):
    set_lut(lut, d, did);
    d+=inc

print_lut(lut, "all_l2r");



