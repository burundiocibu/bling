#!/usr/bin/env python

import csv

# Find slave id from drill id
did2sid={}
with open("HatList_Sep_9_2013.csv", "rU") as fh:
    reader = csv.DictReader(fh);
    for row in reader:
        try:
            sid = int(row['Circuit Board #'])
            did = row['Drill ID #']
            #print row['First Name'], row['Last Name'], did, sid
            did2sid[did] = sid
        except:
            pass    

print did2sid
print len(did2sid)

# Woodwinds in front of field
set4_count56 =[
    'F10', 'F13', 'F7', 'F11', 'F2', 'F8', 'F17', 'F14', 'F12', 'F18', 'F9',
    'F6', 'F19', 'F5', 'F20', 'F4', 'F16', 'F3', 'F15', 'F23', 'F22', 'F21',
    'S4', 'A1', 'A3', 'S2', 'A2', 'A12', 'S1', 'S3', 'A7', 'S7', 'A5', 'A6',
    'S8', 'S6', 'A8', 'A10', 'A4', 'A9', 'S5', 'A11', 'C3', 'C1', 'C22', 'C4'
    'C15', 'C16', 'C2', 'C17', 'C8', 'C23', 'C14', 'C12', 'C24', 'C5', 'C20',
    'C10', 'C9', 'C6', 'C13', 'C11', 'C7'
    ]

set4_count66 = [
    'B7', 'B25', 'B21', 'B15', 'B10', 'B9', 'B24', 'B20', 'B2',
    'B14', 'B6', 'B5', 'B22', 'B13', 'B8', 'B4', 'B17', 'B26', 'B18', 'B23', 'B15', 'B3', 'B19',
    'M9', 'M7', 'M4', 'M8', 'M10', 'M2', 'M5', 'M3', 'M1', 'M6',
    'T14', 'T13', 'T15', 'T17', 'T16', 'T3', 'T4', 'T6', 'T1', 'T2', 'T5', 'T5', 'T9',
    'T7', 'T12', 'T8', 'T19', 'T11', 'T10', 'T20', 'T18'
    ]

lut=[0xffff for i in range(150)]

for i in range(len(set4_count56)):
    did = set4_count56[i]
    if did not in did2sid:
        print "slave id not found for", did
        continue
    sid = did2sid[did]
    lut[sid] = i * 30 # 30 ms delay between marcher * 44 in line = 1.3 seconds across whole line

lut = [str(i) for i in lut]
print "const uint16_t set4_count56[] PROGMEM = {"
print ", ".join(lut), "};"
