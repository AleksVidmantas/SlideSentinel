StaticJsonDocument<RH_SERIAL_MAX_MESSAGE_LEN> mail;     // global messaging variable

enum State {
    WAKE,
    HANDSHAKE, 
    UPDATE,
    POLL,
    UPLOAD,
    SLEEP
} 

enum Verbosity{
    QUIET,
    NORMAL,
    VERBOSE,
    VERY_VERBOSE,
    DEBUG
} 

Verbosity verbosity = NORMAL;   // global variable which dictates the bevaior of Controller.status()
State state = WAKE;             // global state variable 

while(1){
    switch(state){
        case WAKE:
            FSController.init();            // re-init the SD card, sometimes it becomes uninitialized when waking from sleep
            FSController.newDir();          // create a new directory for this wake cycle
            PMCOntroller.enableRadio();     // turn on the radio 
            state = HANDSHAKE;
        case HANDSHAKE:
            IMUController.getFlag(mail);        // check if the Accelerometer woke the device 
            if(!ComController.request(mail)){   // make a request for service to the base station
                FSController.log(mail);         // if the request fails log the failure ERR
                state = SLEEP;
            }       
            state = UPDATE;
        case UPDATE:
            ConManager.update(mail);            
                                                        /* "Controller Manager" 
                                                         * Iterates over every controller, passing the config data collected from the base
                                                         * during the request. Each Controller gets this data passed to its .update() method
                                                         * which dynamically checks if there is data for the particualr controller in the packet.
                                                         * If there is data for the controller, the controller updates its internal variables 
                                                         * thus changing the behavior of the device. 
                                                         */
            case = POLL; 
        case POLL:
            PMController.enableGNSS();          // Turn on the GNSS receiver
            TimingController.start();           // start the millis() timer
            while(TimingController.isAwake()){  // while the timer is less than the wake duration...
                if(GNSSController.poll(mail))   // check if parsed data came from the receiver
                    FSController.log(mail);     // if so log the data, DAT
            }
            PMController.disableGNSS();         // turn off the receiver
            state = UPLOAD;
        case UPLOAD:
            ConManager.status(mail, verbosity);              
                                                        /* 
                                                         * Collect the message to send to the base. Iterates over every controller, 
                                                         * and checks for data calling the base class Controller.status() method. 
                                                         * For example the GNSSController.status() will append any RTK positional info 
                                                         * to the packet for the wake cycle. If the verbosity flag is in debug though
                                                         * the GNSSController may include hdop and satellites view values to the packet
                                                         * for greater information about the rover. 
                                                         * The PMController may append the battery voltage to the packet as another example.
                                                         */

            FSController.log(mail);                 // log the packet collected
            if(!ComController.upload(mail))         // attempt to upload the packet to the base
                FSController.log(mail);             // if a failure occurs mail is populated with error message, log this to SD
            state = SLEEP;
        case SLEEP:
            PMController.disableRadio();            // turn of off the radio
            TimingController.setAlarm();            // set the alarm to wake up
            PMController.sleep();                   // go to sleep
            state = WAKE;
    }
}

/* POSSIBLE MESSAGE TYPES
RTS,
CONF,
DAT, 
ERR, 
*/

/* UPDATE                   
FSController        []
IMUController       [accelerometer sensitivity]
TimingController    [wake duration, sleep duration]
ComController       [retries, timeout]  
PMController        []
GNSSController      []
ConManager          [verboisy]      <--- We could migrate the verbosity to the ConManager, instead of making it global
*/

// Ther verbosity of the rover dictates how much of this data is appended to the packet prior to sending
/* STATUS                 
FSController        [available SD card space, number of wake cycles to date]
IMUController       [accelerometer sensitivity]
TimingController    [wake duration, sleep duration]
ComController       [retries, timeout, number of dropped packets]  
PMController        [battery voltage]
GNSSController      [hdop, pdop, satellites view during the wake cycle, report resolution (maybe the user is fine with RTK float data)]
ConManager          [verboisy]      <--- We could migrate the verbosity to the ConManager, instead of making it global
*/




