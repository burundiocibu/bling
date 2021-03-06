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

max_sid=100
# Find slave id from drill id
did2sid={}
# drill id from slave id
sid2did = {}
# Find marcher name from drill id
did2name = {}
# Find hat number from drill id
did2hid = {}
with open("slaves", "r") as fh:
    reader = csv.DictReader(fh);
    for row in reader:
        try:
            sid = int(row['slave_id'])
            did = row['drill_id'].strip()
            name = row['name'].strip()
            did2name[did] = name
            did2sid[did] = sid
            sid2did[sid] = did
        except:
            print "Error reading slaves"
            pass    

        
if args.list==1:
    for did in sorted(did2sid.keys()):
        print "{:4s} {:3d} {}".format(did, did2sid[did], did2name[did])
    exit()
elif args.list==2:
    for sid in sorted(did2sid.values()):
        if sid == 999: continue
        print "{:3s} {:3d} {}".format(sid2did[sid], sid, did2name[sid2did[sid]])
    exit()


one_special = ['DG36', 'X1']

all_intro = [
    'DG9',  'DG10', 'DG11', 'DG12', 'DG13', 'DG14', 'DG15', 'DG18',
    'DG20', 'DG22', 'DG23', 'DG26', 'DG27', 'DG35', 'DG36', 'DG47'
    'X1', 'X2', 'X3'
    ]

all_balad = all_intro + [
    'DG24', 'DG25', 'DG30', 'DG31', 'DG32', 'DG34', 'DG37', 'DG38', 
    'DG40', 'DG41', 'DG42', 'DG43', 'DG44', 'DG45', 'DG50', 'DG55',
    'X4', 'X5', 'X6'
    ]

lut=[0x00 for i in range(max_sid)]
for did in sorted(did2sid.keys()):
    d=lut[did2sid[did]]
    if did in one_special: d |= 0x1
    if did in all_intro: d |= 0x2
    if did in all_balad: d |= 0x4
    set_lut(lut, d, did)
print "/* bit 1:special one, 2:intro , 3:balad */"
print_lut(lut, "section")
