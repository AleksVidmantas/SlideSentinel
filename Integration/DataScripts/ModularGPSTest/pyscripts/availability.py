#!/usr/bin/python
import math
from datetime import datetime
from decimal import Decimal, ROUND_UP
from ttff import test_ttff as tttff
from NMEAStructure import NMEA_FORMAT as nf

class test_availability():
    def __init__(self, file_name):
        self.fname = file_name
        self.ttff_obj = tttff("NULL", 0)

    def acquire_average_fixed_int(self, aArray, startT, finalT):
        sumTime = 0
        counter = 0
        for times in aArray:
            counter += 1
            sumTime += times

        totalTimeOfFileRecordings = float(self.ttff_obj.time_difference_in_seconds(startT, finalT))

        return sumTime / totalTimeOfFileRecordings

    def event_availability(self):
        f = open(self.fname, "r")

        availabilityArray = []
        trackTime = False
        firstEntry = False
        formatter = nf()
        beginTime = lastTime = finalTime = recordingStartTime = 0

        for x in f:
            entry = x.split(',')

            format = []
            format = formatter.return_indices(entry[0])

            try:
                if firstEntry == False:
                    recordingStartTime = entry[format[2]]
                    firstEntry = True

                check_entry = int(entry[format[3]])
                if entry[format[3]] == 'R' or entry[format[3]] == '4':
                    if trackTime == False:
                        trackTime = True
                        beginTime = entry[format[2]]

                    lastTime = entry[format[2]]
                else:
                    if trackTime == True:
                        if self.ttff_obj.time_difference_in_seconds(beginTime, lastTime) == 0:
                            availabilityArray.append(1)
                        else:
                            availabilityArray.append(int(self.ttff_obj.time_difference_in_seconds(beginTime, lastTime)))

                    trackTime = False
            except:
                pass

            finalTime = entry[format[2]]

        if trackTime == True:
            availabilityArray.append(int(self.ttff_obj.time_difference_in_seconds(beginTime, lastTime)))

        avg = self.acquire_average_fixed_int(availabilityArray, recordingStartTime, finalTime)
        newAvg = Decimal(avg*100).quantize(Decimal(".01"), rounding=ROUND_UP)

        return newAvg
