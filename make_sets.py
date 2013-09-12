#!/usr/bin/env python

import csv

from sys import exit

from argparse import ArgumentParser
parser = ArgumentParser()
parser.add_argument('-d', dest="debug", action='count', help='Increase debug level.')
parser.add_argument('-m', dest="missing", action='count', help='Print out missing boards.')
args = parser.parse_args()

# Find slave id from drill id
did2sid={}
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
            did2name[did] = row['First Name'] + " " + row['Last Name']
            did2hid[did] = row['Hat #']
        except:
            pass    


if args.debug:
    print "Record for {} marchers".format(len(did2sid))
    print "Missing Flute Boards:"
    for did,sid in did2sid.iteritems():
        if did[0] == 'F' and sid==999:
            print "{:3s} {:3s} {}".format(did, did2hid[did], did2name[did])


if args.missing>1:
    print "Marcher IDs with missing boards:"
    for did,sid in did2sid.iteritems():
        if sid==999:
            print "{:3s} {}".format(did, did2name[did])
    exit()


# Woodwinds set the hats down
set4_count56 = [
    'F10', 'F13', 'F7', 'F11', 'F2', 'F8', 'F17', 'F14', 'F12', 'F18', 'F9',
    'F6', 'F19', 'F5', 'F20', 'F4', 'F16', 'F3', 'F15', 'F23', 'F22', 'F21',
    'S4', 'A1', 'A3', 'S2', 'A2', 'A12', 'S1', 'S3', 'A7', 'S7', 'A5', 'A6',
    'S8', 'S6', 'A8', 'A10', 'A4', 'A9', 'S5', 'A11', 'C3', 'C1', 'C22', 'C4',
    'C15', 'C16', 'C2', 'C17', 'C8', 'C23', 'C14', 'C12', 'C24', 'C5', 'C20',
    'C10', 'C9', 'C6', 'C13', 'C11', 'C7'
    ]

# Brass set the hats down
set4_count66 = [
    'B7', 'B25', 'B21', 'B15', 'B10', 'B9', 'B24', 'B20', 'B2',
    'B14', 'B6', 'B5', 'B22', 'B13', 'B8', 'B4', 'B17', 'B26', 'B18', 'B23', 'B15', 'B3', 'B19',
    'M9', 'M7', 'M4', 'M8', 'M10', 'M2', 'M5', 'M3', 'M1', 'M6',
    'T14', 'T13', 'T15', 'T17', 'T16', 'T3', 'T4', 'T6', 'T1', 'T2', 'T5', 'T5', 'T9',
    'T7', 'T12', 'T8', 'T19', 'T11', 'T10', 'T20', 'T18'
    ]


def print_delays(mid_list, var_name):
    global did2sid
    global did2name
    lut=[0xff for i in range(150)]

    for i in range(len(mid_list)):
        did = mid_list[i]
        if did not in did2sid:
            print "slave id not found for", did
            continue
        sid = did2sid[did]
        if sid == 999:
            if args.missing:
                print "No board: {:3s} {}".format(did, did2name[did])
        else:
            lut[sid] = i

    lut = [str(i) for i in lut]
    print "const uint8_t ",var_name,"[] PROGMEM = {"
    print ", ".join(lut), "};"
    

print_delays(set4_count56, "e3_delay")
print_delays(set4_count66, "e4_delay")
