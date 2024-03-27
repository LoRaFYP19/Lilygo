
#include <RadioLib.h>
#include "utilities.h"
#include "boards.h"



SX1276 radio = new Module(RADIO_CS_PIN, RADIO_DIO0_PIN, RADIO_RST_PIN, RADIO_BUSY_PIN);

#define LoRa_frequency 923.0


#define PreAmbleLength 6
#define wanSync 0x34
#define nodeID 0

int Spreadf= 8;
int OutputPower = 2;
int Bandwidth = 125;
int codeRate = 5;


int crcErrors=0;
int numberofpackets=0;
String str;

bool EarthQuack = false;

volatile bool actionFlag = false; // Flag to indicate that a packet was received or sent
volatile bool isTransmitting = false; // Flag to identify TX or RX
volatile bool enableInterrupt = true; // Flag to enable interrupt

// unsigned long previousTransmissionTime = 0; // Variable to store the previous transmission time

// save transmission state between loops
int transmissionState = RADIOLIB_ERR_NONE;

// Interrupt handler for both transmit and receive
void handleInterrupt() {
    if (!enableInterrupt) {
        return;
    }

    actionFlag = true;
}

/**
 * Converts a Decimal value to a hexadecimal string with optional node ID and padding.
 *
 * @param value the 64-bit integer value to convert
 * @param buffer the buffer to store the resulting hexadecimal string
 * @param size the size of the buffer
 *
 * @return void
 *
 * @throws None
 */
void int64ToHexString(int64_t value, char* buffer, size_t size) {
    // snprintf(buffer, size, "%llx", static_cast<unsigned long long>(value)); // for default payload
    
    snprintf(buffer, size, "%d,%07llx", nodeID, static_cast<unsigned long long>(value)); // payload with node ID and padded value
}


/**
 * Function to start transmission.
 *
 * @param None
 *
 * @return None
 *
 * @throws None
 */
void startTransmission() {
    Serial.print(F("Sending Warning ... "));
    isTransmitting = true;
    char txpacket[20];
    int64_t tMili = 1888+20*60000+10*3600000; // placeholder
    int64ToHexString(tMili, txpacket, sizeof(txpacket));
    transmissionState = radio.startTransmit(txpacket); // this is non-blocking action, meaning the radio is transmitting, the execution of other tasks are not on hold
}

/**
 * Sets up the LoRa radio module with the specified parameters.
 *
 * @param None
 *
 * @return None
 *
 * @throws None
 */
void setup_lora(){
        radio.setSpreadingFactor(Spreadf);
        radio.setOutputPower(OutputPower);
        radio.setBandwidth(Bandwidth);
        radio.setCodingRate(codeRate);
}

/**
 * Selects and returns the appropriate parameter based on the given character and index.
 *
 * @param c The character used to determine the parameter value.
 * @param index The index of the parameter to select.
 *
 * @return The selected parameter value.
 *
 * @throws None
 */
float select_params(char c, int index) {
    if (index == 2) { // for spreading factor
        int val = c - '0';
        return val+6;
    }

    if (index == 3) // Bandwidth
    {
        switch(c) {
            case '1': return 10.4;
            case '2': return 15.6;
            case '3': return 20.8;
            case '4': return 31.25;
            case '5': return 41.7;
            case '6': return 62.5;
            case '7': return 125;
            case '8': return 250;
            case '9': return 500;
            default: return 125;
        }
    }
    if (index == 4) // code rate
    {
        return c - '0';
    }

    if (index == 5) // power
    {
        int val = c > '9' ? (c | 0x20) - 'a' + 10 : c - '0';
        return val + 2;
    }

}

/**
 * Set LoRa parameters based on the input string.
 *
 * @param str the input string containing parameters
 *
 * @return void
 *
 * @throws ErrorType if there is an error in setting the parameters
 */
void set_params_lora(String str){
    // for loop for chars from 2 onward
    for (int i = 2; i < str.length(); i++) {
        // select_params
        float val = select_params(str.charAt(i), i);
        if (i == 2)
        {
            Spreadf = (int)val;
        }
        else if (i==3)
        {
            Bandwidth = val;
        }
        else if (i==4)
        {
            codeRate = (int)val;
        }
        else if (i==5)
        {
            OutputPower = (int)val;
        }

        
        
    }
    
}

void setup()
{
    initBoard();
    // When the power is turned on, a delay is required.
    delay(1500);

    // initialize SX1276 with default settings
    Serial.print(F("Initializing ... "));

#ifndef LoRa_frequency
    int state = radio.begin(923.0);
#else
    int state = radio.begin(LoRa_frequency);
#endif
    if (state == RADIOLIB_ERR_NONE) {
        Serial.println(F("success!"));
        radio.setSpreadingFactor(Spreadf);
        radio.setOutputPower(2);
        radio.setBandwidth(125);
        radio.setCurrentLimit(120);
        radio.setSyncWord(wanSync);
        radio.setPreambleLength(PreAmbleLength);
    } else {
        Serial.print(F("failed, code "));
        Serial.println(state);
        while (true);
    }
    // set the function that will be called
    // when new packet is received or trasnmitted
    radio.setDio0Action(handleInterrupt, RISING); // Trigger on Either Transmisson end or reception start

    // start listening for LoRa packets at the initial setup
    Serial.print(F("Starting to listen ... "));
    state = radio.startReceive(); 
#ifdef HAS_DISPLAY
    if (u8g2) {
        if (state != RADIOLIB_ERR_NONE) {
            u8g2->clearBuffer();
            u8g2->drawStr(0, 12, "Initializing: FAIL!");
            u8g2->sendBuffer();
        }
    }
#endif
    if (state == RADIOLIB_ERR_NONE) {
        Serial.println(F("success!"));
    } else {
        Serial.print(F("failed, code "));
        Serial.println(state);
        while (true);
    }

}

void loop(){

    if (EarthQuack == true){
        EarthQuack = false;
        startTransmission();
    }
    // check if the flag is triggerd
    if (actionFlag){
        
        actionFlag = false;
        enableInterrupt = false;

        if (isTransmitting) {
            Serial.println(F("Still Transmistting."));
            if (transmissionState == RADIOLIB_ERR_NONE){
                Serial.println(F("Transmisson was success!"));
                // radio.setSpreadingFactor(Spreadf);
            }
            else {
                Serial.print(F("failed, code "));
                Serial.println(transmissionState);
                // radio.setSpreadingFactor(Spreadf);
            }
            // clean up after transmission is finished
            // this will ensure transmitter is disabled,
            // RF switch is powered down etc.
            radio.finishTransmit();

            delay(100);
            #ifdef HAS_DISPLAY
            if (u8g2) {
                char buf[256];
                u8g2->clearBuffer();
                u8g2->drawStr(0, 12, "Transmitting: OK!");
                u8g2->sendBuffer();
            }
            #endif
             // Set the flag to false after transmission is completed
            isTransmitting = false;

            // put module back to listen mode
            radio.startReceive();
    }
    else{
        // Triggered by reception
        // read received data as an Arduino String
        
        int receptionstate = radio.readData(str);
    
        if (receptionstate == RADIOLIB_ERR_NONE){
            // if first 2 chars in str String is ##, call dummy() function
            
            Serial.println(F("Received packet!"));
            numberofpackets++;
            Serial.println(str);
            int64_t rssi = radio.getRSSI(); 
            int64_t snr = radio.getSNR();
            int64_t FrqError = radio.getFrequencyError();
            Serial.println(str+","+String(numberofpackets)+","+String(rssi)+","+String(snr)+","+String(FrqError));
            #ifdef HAS_DISPLAY
                if (u8g2) {
                    char buf[256];
                    u8g2->clearBuffer();
                    u8g2->drawStr(0, 12, "Received OK!");
                    u8g2->drawStr(5, 26, str.c_str());
                    snprintf(buf, sizeof(buf), "RSSI:%.2f", radio.getRSSI());
                    u8g2->drawStr(0, 40, buf);
                    snprintf(buf, sizeof(buf), "SNR:%.2f", radio.getSNR());
                    u8g2->drawStr(0, 54, buf);
                    u8g2->sendBuffer();
                }
            #endif
            if (str[0] == '#' && str[1] == '#'){ // for reconfiguration mode
                set_params_lora(str);
                setup_lora();
                delay(1000);
                
            }

            delay(100);
        }
        else if (receptionstate == RADIOLIB_ERR_CRC_MISMATCH){
            // packet was received, but is malformed
            Serial.println(F("CRC error!"));
            crcErrors = crcErrors +1;
            Serial.println(crcErrors);
        }
        else{
            Serial.print(F("Failed, code "));
            Serial.println(receptionstate);
        }

    }

}
 enableInterrupt = true;

}
