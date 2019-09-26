#!/usr/bin/python
from testSuite import test_suite

def main():

    surveyedLatitude = 4433.9958187
    surveyedLongitude = 12317.6156572
    fileNameAssessing = "data/PSTI.TXT" #testing psti
    #fileNameAssessing = "data/GPGGA.TXT"  #testing gpgga
    timeIntervalForTTFF = 20
    tSuite = test_suite(fileNameAssessing, surveyedLatitude, surveyedLongitude, timeIntervalForTTFF)

    tSuite.conduct_tests()


    """
    #Testing TTFF time difference between times WORKS
    #Need to add: TTFF to float
    temp = tttff("GPGGA.TXT")
    #print(str(temp.convert_to_time_string("162651.000")))
    t1 = "162651.000"
    t2 = "162701.000"
    timeDif = float(temp.time_difference_in_seconds(t1, t2))

    if timeDif < 5:
        print("Time " + str(timeDif))
    temp = tttff("test.txt")

    print(str(temp.event_ttff()))
    """

    """
    #Tested availability WORKS
    test = tAva("test.txt")
    aArray = []
    aArray = test.event_availability()
    for times in aArray:
        print("Time: " + str(times))

    """
    """
    test = tAva("GPGGA.TXT")
    temp = test.event_availability()

    print("Time Avg: " + str(temp) + "%")
    """


main()
