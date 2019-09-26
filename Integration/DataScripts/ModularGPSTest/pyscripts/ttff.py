#!/usr/bin/python
import math
from datetime import datetime
from NMEAStructure import NMEA_FORMAT as nf

class test_ttff:
    def __init__(self, file_name, eTime):
        self.fname = file_name
        self.comparativeElapsedTime = eTime

    def split_time_string(self, _time):
        temp = _time.split('.')

        return temp[0]

    def convert_to_time_string(self, _time):
        time_ = self.split_time_string(_time)
        FMT = '%H%M%S'

        return datetime.strptime(time_, FMT)

    def time_difference_in_seconds(self, t1, t2):
        time1 = self.convert_to_time_string(t1)
        time2 = self.convert_to_time_string(t2)

        timeDifference = time2 - time1
        return timeDifference.total_seconds()

    def event_ttff(self):
        f = open(self.fname, "r")
        formatter = nf()

        trackTime = False
        firstEntry = False
        beginTimer = currentTime = recordingStartTime = 0
        ttfftime = count = 0
        for x in f:
            count += 1
            entry = x.split(',')
            format = formatter.return_indices(entry[0])
            try:
                if firstEntry == False:
                    recordingStartTime = entry[format[2]]
                    firstEntry = True
                
                if entry[format[3]] == 'R' or entry[format[3]] == '4':
                    if trackTime == False:
                        beginTime = entry[format[2]]
                        trackTime = True

                    elapsedTimeForFix = float(self.time_difference_in_seconds(beginTime, entry[format[2]]))

                    if elapsedTimeForFix > self.comparativeElapsedTime: #if there was a fix for 20 seconds
                        ttfftime = self.time_difference_in_seconds(recordingStartTime, entry[format[2]])
                        break

                #There was a break in continuity of a fixed integer solution so reset trackTime
                else:
                    trackTime = False

            except:
                pass

        return ttfftime
