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

#NMEA-0183 Structure for STI 030 messages
#  0   1      2      3       4       5       6       7  8   9   10  11   12  13 14   15
$PSTI,030,hhmmss.sss,A,dddmm.mmmmmmm,a,dddmm.mmmmmmm,a,x.x,x.x,x.x,x.x,ddmmyy,a.x.x,x.x*hh<CR><LF>
Entry Pos   Name
    0       NMEA Format ID
    1       STI Message ID
    2       UTC Time
    3       Status
    4       Latitude
    5       N/S Indicator
    6       Longitude
    7       E/W Indicator
    8       Altitude
    9       East Velocity
    10      North Velocity
    11      Up Velocity
    12      UTC Date
    13      Mode Indicator Mode indicator
                N = Data not valid
                A = Autonomous mode
                D = Differential mode
                E = Estimated (dead reckoning) mode
                M = Manual input mode
                S = Simulator mode
                F = Float RTK. Satellite system used in RTK mode, floating
                integers
                R = Real Time Kinematic. System used in RTK mode with fixed
                integers
    14      RTK Age
    15      RTK Ratio
"""
class NMEA_FORMAT:
    def __init__(self):
        self.pstiMode = False
        self.gpggaMode = False

        #Index fields for parsing NMEA-0183 files
        self.latIndex = 0
        self.lonIndex = 0
        self.utcTime = 0
        self.qualityIndicator = 0
        self.modeIndicator = 0
        self.rtkAge = 0
        self.rtkRatio = 0

    def process_format(self, format_str):
        if format_str == "$PSTI":
            self.set_format_to_psti()
            self.pstiMode = True
            self.gpggaMode = False
        elif format_str == "$GPGGA":
            self.set_format_to_gpgga()
            self.gpggaMode = True
            self.pstiMode = False
        else:
            print("DEFAULT NO FORMAT")

    def set_format_to_psti(self):
        self.latIndex = 4
        self.lonIndex = 6
        self.utcTime = 2
        self.modeIndicator = 13
        self.rtkAge = 14
        self.rtkRatio = 15

    def set_format_to_gpgga(self):
        self.latIndex = 2
        self.lonIndex = 4
        self.utcTime = 1
        self.qualityIndicator = 6
        self.rtkAge = 11

    def return_indices(self, format_str):
        self.process_format(format_str)

        if self.is_psti() == True:
            modeIndeces = []
            modeIndeces.append(self.latIndex)
            modeIndeces.append(self.lonIndex)
            modeIndeces.append(self.utcTime)
            modeIndeces.append(self.modeIndicator)
            modeIndeces.append(self.rtkAge)
            modeIndeces.append(self.rtkRatio)
            return modeIndeces

        elif self.is_gpgga() == True:
            modeIndeces = []
            modeIndeces.append(self.latIndex)
            modeIndeces.append(self.lonIndex)
            modeIndeces.append(self.utcTime)
            modeIndeces.append(self.qualityIndicator)
            modeIndeces.append(self.rtkAge)
            return modeIndeces

        else:
            print("Uncaught No mode triggered")

    def is_psti(self):
        return self.pstiMode

    def is_gpgga(self):
        return self.gpggaMode
