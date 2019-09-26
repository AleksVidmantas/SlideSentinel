#!/usr/bin/python
"""
#NMEA-0183 Structure for GGA messages
#   0      1            2          3        4           5  6   7   8    9   10  11  11  12   13
#$GPGGA, HHMMSS, ddmm.mmmmmmmmmmm, a, ddmm.mmmmmmmmmmm, a, x, xx, x.x, x.x, M, x.x, M, x.x, xxxx*hh
Entry Pos   Name
    0       NMEA Format ID
    1       UTC Time
    2       Latitude
    3       N/S Indicator
    4       Longitude
    5       E/W Indicator
    6       GPS Quality Indicator: 4 == Fixed Integer, 5 == Floating Integer
    7       Satellites Used
    8       HDOP
    9       Altitude
    10      Geoidal Seperation
    11      Age of Differential GPS data
    12      DGPS Station ID
    13      Checksum
"""
import math
from datetime import datetime
from decimal import Decimal, ROUND_UP
from ttff import test_ttff as tttff
from NMEAStructure import NMEA_FORMAT as nf

class test_continuity():
    def __init__(self, file_name):
        self.fname = file_name
        self.ttff_obj = tttff("NULL", 0)

    def acquire_reacquire_average(self, aArray):
        sum = count = 0
        for i in range(len(aArray)):
            sum += aArray[i]
            count += 1
        count += 1

        avg = sum / count

    def event_continuity(self):
        f = open(self.fname, "r")

        availabilityArray = []
        reacquireArray = []
        trackTimeInt = False
        beginTimeInt = beginTimeReacquire = 0
        for x in f:
            entry = x.split(',')
            try:
                if entry[format[3]] == 'R' or entry[format[3]] == '4':
                    if trackTimeInt == False:
                        trackTimeInt = True
                        beginTimeInt = entry[format[2]]
                        reacquireArray.append(int(self.ttff_obj.time_difference_in_seconds(beginTimeReacquire, entry[format[2]])))

                else:
                    if trackTimeInt == True:
                        beginTimeReacquire = entry[format[2]]
                        availabilityArray.append(int(self.ttff_obj.time_difference_in_seconds(beginTimeInt, entry[format[2]])))

                    trackTimeInt = False
            except:
                pass

        avg = self.acquire_reacquire_average(reacquireArray)

        return avg
