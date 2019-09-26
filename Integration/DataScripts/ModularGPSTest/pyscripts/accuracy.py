#!/usr/bin/python
import geopy.distance
import math
from NMEAStructure import NMEA_FORMAT as nf

class test_accuracy:
    def __init__(self, file_name, _surveyLat, _surveyLon):
        self.fname = file_name
        self.surveyLat = self.decimalDegree(_surveyLat)
        self.surveyLon = self.decimalDegree(_surveyLon)

    def decimalDegree(self, ddm):
        deg = math.floor(ddm / 100)
        min = (ddm-(deg*100)) / 60
        return(deg + min)

    def distance_between_GPS_coords(self, lat1, lon1):
        lat1 = self.decimalDegree(lat1)
        lon1 = self.decimalDegree(lon1)

        coord1 = (lat1, lon1)
        coord2 = (self.surveyLat, self.surveyLon)

        return geopy.distance.vincenty(coord1, coord2).meters

    def event_accuracy(self):
        f = open(self.fname, "r")
        formatter = nf()
        counter = 0
        sum = 0
        for x in f:
            entry = x.split(',')
            format = []
            format = formatter.return_indices(entry[0])

            try:
                if entry[format[3]] == 'R' or entry[format[3]] == '4':
                    counter += 1
                    sum += self.distance_between_GPS_coords(float(entry[format[0]]), float(entry[format[1]]))

            except:
                pass

        average = sum / counter

        return average
