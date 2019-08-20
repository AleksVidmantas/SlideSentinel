# Modular Receiver Tests
This modular testing suite will be available for testing any gps receiver with a Base and Rover setup. The Base and Rover setup used in this test is the Slide Sentinel interface which can be found in [Slide Sentinel README.md](https://github.com/OPEnSLab-OSU/SlideSentinel/blob/TheRealReadingGPSData/README.md). Whatever setup you decide to interface with your GPS Receivers, they must produce NMEA-0183 data for this testing suite. The tests will assess how well the gps receivers perform and what enviornments they are suitable for. The tests will also assess whether they can retain a sub-centimeter reading which is a necessary for the Slide Sentinel project. 

## Receivers Included in initial tests	
* S2525F8-GL-RTK
* Piksi Multi 

## Whats being tested?
### Accuracy
This will test how accurate the GPS Receiver is at collecting a gps coordinate. This will assess whether the GPS Receiver is collecting sub centimeter readings. The code for this test is written in accuracy.py.
### Continuity
This will test the average time it takes for a GPS Receiver to regain a Fixed-Integer solution after it has been lossed. The purpose of this is to test how long it takes the receiver to regain a high quality solution for a gps coordinate. This test has yet to be created but will be created in continuity.py.
### Availability
Availability tests the total amount of time the receiver stays in a Fixed-Integer solution as a percentage. This will tell how often a Fixed-Integer solution occurs throughout the whole duration of a Rovers gps coordinate gathering cycle. The code for this test is written in availability.py.
### Time To First Fix 
Time to first fix(TTFF) finds the amount of time it takes to get a Fixed-Integer solution and subsequently hold for a allotted time. In the data that is analyzed; there are occurrences of a Fixed-Integer solution that doesn't best represent an accurate reading. Thus, a time threshold for how long the first Fixed-Integer holds will most likely have a more accurate reading. The code for this test is written in ttff.py. 
## Procedure For Conducting Tests
### Getting Data
A survey grade gps must be used to compare the quality of the gps readings gathered by the receivers being tested. The GPS will be tested in three areas: open sky, minor obtrusion, major obtrusion. The three locations will represent 180&deg; sky-view, 90&deg; sky-view and 45&deg; sky-view respectively. A total of four points will be recorded by the survey gps: Central location for the Base, and the three other positions recently discussed. Then test each position with one Base and Rover keeping the Base in a static open sky location (preferably in a central location to all three test positions). Run the gps receivers within the Slide Sentinel interface hot for one hour. NMEA-0183 strings will be saved to a SD card provided in the Slide Sentinel Interface. Once all three locations are surveyed and tested. Sufficient data has been gathered to test the GPS Receiver via Testing Scripts.
### Testing Scripts
#### Setting up your scripting enviornment
If pip and python are already installed on your machine, skip to install command 4. Otherwise execute all of the following commands using a bash console. 
1. sudo apt-get install python
1. sudo apt update
1. sudo apt install python-pip
1. pip install geopy
1. git clone https://github.com/OPEnSLab-OSU/SlideSentinel.git </br>

Now move into the folder with the testing scripts via console. The path follows ~/SlideSentinel/Integration/DataScripts/ModularGPSTest/pyscripts 
#### How the testing scripts work
testEnv.py is going to be the main testing enviornment where you enter a few pieces of information. First, the data must be extracted from the Slide Sentinel Interface and uploaded into the data folder. Note: The current test scripts only evaluate GPGGA and PSTI formatted NMEA strings. The format is handled in NMEA_FORMAT.py. Within testEnv set the surveyLatitude and surveyLongitude to the gps coordinates recorded per location by the survey grade gps. Retrieve data from each position recorded, relabel the file to the location. The three locations used during testing is open sky, minor obtrusion, and major obtrusion. The testing suite currently tests one file at a time located in the testEnv.py. So configuring the target filename will be required by the user. 

To execute the scripts type python testEnv.py. 
### Setting up GPS Testing Grounds
#### Finding test locations
Preferably, look for a location that suites all three testing enviornments with an open-sky location for the Base. The Base should be placed centrally between these three locations. The diagram below shows a visual example.  

![alt text](https://github.com/OPEnSLab-OSU/SlideSentinel/blob/TheRealReadingGPSData/Documentation/images/fieldDiagram.PNG)

Testing in different enviornments tests multipath issues that all modern receivers face. Attenuating multipath can be reduced by creating a radio link between the Base and Rover. The Base can broadcast correctional information to the receiver via radio. This process is called differential gps or DGPS. 
#### Procedure for testing locations 
Before testing, ensure that you have a survey grade gps handy, and two receivers properly configured as a Base and Receiver. For precise measurements, bring a marker of some sort (wooden stake, metal rod, pvc pipe, etc..) to mark each location surveyed. The next following steps will guide the user through the data gathering process and often refer to the diagram above. 
1. Survey the location of the Base
  1. Set a Stake to mark the location of the surveyed position
  2. Pre-program the Base with the surveyed location
  3. Set the base in the location of the surveyed position 
2. Survey the location of the Rover 180&deg; sky-view location
  1. Set a Stake to mark the location of the surveyed position
  2. Record the surveyed GPS position
  3. Set the Rover in the location of the surveyed position
  4. Ensure the Base is broadcasting correctional information
  5. Activate Receiver for one hour 
  6. Extract data and save to a recognizable file name
3. Repeat Process 2 for the 90&deg and 45&deg sky-view positions.
4. Test the data with the py scripts

Important Note: All tests will conduct a one hour long test with each receiver. Depending on the quality of the gps and the enviornment it was placed. Doesn't ensure that the length of time is adequate for it to receive a quality positional fix at a sub-centimeter level. For the GPS's listed above, they should be adequate in resolving a solution within the time frame specified.

