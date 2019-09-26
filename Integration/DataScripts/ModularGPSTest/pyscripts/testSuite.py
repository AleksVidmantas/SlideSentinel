#!/usr/bin/python
from accuracy import test_accuracy as tAcc
from ttff import test_ttff as tttff
from availability import test_availability as tAva
from continuity import test_continuity as tc

class test_suite():
    def __init__(self, f_name, surveyLat, surveyLon, tInterval):
        self.testAccuracy = tAcc(f_name, surveyLat, surveyLon)
        self.testTTFF = tttff(f_name, tInterval)
        self.testAvailability = tAva(f_name)
        self.testContinuity = tc(f_name)

    def conduct_tests(self):

        accuracy = self.testAccuracy.event_accuracy()
        TimeTFF = self.testTTFF.event_ttff()
        availability = self.testAvailability.event_availability()
        continuity = self.testContinuity.event_continuity()

        #write stuff to file maybe for now just print to stdout
        print("Accuracy: " + str(accuracy) +"m")
        print("TTFF: " + str(TimeTFF) + "s")
        print("Availability Fixed-Int: " + str(availability) + "%")
        print("Average time for Fixed int reacquire: " + str(continuity) + "s")
