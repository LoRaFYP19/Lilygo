
// #include <RadioLib.h>
// #include "utilities.h"
// #include "boards.h"
// #include <WiFi.h>
// #include <HTTPClient.h>
// #include <ezTime.h>

// SX1276 radio = new Module(RADIO_CS_PIN, RADIO_DIO0_PIN, RADIO_RST_PIN, RADIO_BUSY_PIN);

// #define LoRa_frequency 923.0
// #define SpreadF 7
// #define OPower 17
// #define Bandwidth 125
// #define CurrentLimit 120
// #define PreAmbleLength 6

// #define repeatSF 8

// // flag to indicate that a packet was received
// #define nodeId 0

// const char* ssid = "F1";
// const char* password = "123456789";

// // save transmission state between loops
// int transmissionState = RADIOLIB_ERR_NONE;

// volatile bool receivedFlag = false;

// // disable interrupt when it's not needed
// volatile bool enableInterrupt = true;

// //Your Domain name with URL path or IP address with path
// String serverName = "https://script.google.com/macros/s/AKfycbzmBz3kstwiNjQVGuUoN_GEmpV8hnVNwppzb4oytaz1lWB4em3UliuXozVI1Pmg2BRvNA/exec";

// bool txSheet = false;
// int rxNumber;

// String str;

// // this function is called when a complete packet
// // is received by the module
// // IMPORTANT: this function MUST be 'void' type
// //            and MUST NOT have any arguments!
// void setFlag(void)
// {
//     // check if the interrupt is enabled
//     if (!enableInterrupt) {
//         return;
//     }

//     // we got a packet, set the flag
//     receivedFlag = true;
// }


// void setup()
// {
//     Serial.begin(115200);
//     initBoard();
//     // When the power is turned on, a delay is required.
//     delay(1500);
//     WiFi.disconnect();
//     delay(1000);
//     WiFi.begin(ssid, password);
//     Serial.println("Connecting");
//     while(WiFi.status() != WL_CONNECTED) {
//       delay(500);
//       Serial.print(".");
//     }

//     Serial.println("");
//     Serial.print("Connected to WiFi network with IP Address: ");
//     Serial.println(WiFi.localIP());

//     setInterval(60);
//     waitForSync();
//     String val_1 = UTC.dateTime("l, d-M-y H:i:s.v T");
//     // int sec_1 = (val_1.substring(24,27)).toInt();
//     // int msec_1 = (val_1.substring(27,30)).toInt();
//     Serial.println(val_1);
//     rxNumber=0;
//     rssi=0;


//     // initialize SX1276 with default settings
//     Serial.print(F("Initializing ... "));
// #ifndef LoRa_frequency
//     int state = radio.begin(923.0);
// #else
//     int state = radio.begin(LoRa_frequency);
// #endif
//     if (state == RADIOLIB_ERR_NONE) {
//         Serial.println(F("success!"));
//         radio.setSpreadingFactor(SpreadF);
//         radio.setOutputPower(OPower);
//         radio.setBandwidth(Bandwidth);
//         radio.setCurrentLimit(CurrentLimit);
//         radio.setPreambleLength(PreAmbleLength);
//     } else {
//         Serial.print(F("failed, code "));
//         Serial.println(state);
//         while (true);
//     }
 

//     // when new packet is received, interrupt
//     radio.setDio0Action(setFlag, RISING);

//     // start listening for LoRa packets
//     Serial.print(F("Starting to listen ... "));
//     state = radio.startReceive();
    
// #ifdef HAS_DISPLAY
//     if (u8g2) {
//         if (state != RADIOLIB_ERR_NONE) {
//             u8g2->clearBuffer();
//             u8g2->drawStr(0, 12, "Initializing: FAIL!");
//             u8g2->sendBuffer();
//         }
//     }
// #endif

//     if (state == RADIOLIB_ERR_NONE) {
//         Serial.println(F("success!"));
//     } else {
//         Serial.print(F("failed, code "));
//         Serial.println(state);
//         while (true);
//     }

// }

// void resendRepater(){
//     radio.setSpreadingFactor(repeatSF);
//     transmissionState = radio.startTransmit(str);
// }


// void loop()
// {
//     // check if the flag is set
//     events();
//     if (receivedFlag) {
//         // disable the interrupt service routine while
//         // processing the data
//         enableInterrupt = false;

//         // reset flag
//         receivedFlag = false;

//         // you can read received data as an Arduino String
        
//         int state = radio.readData(str);
//         Serial.print("Initial p rec ");
//         Serial.print(str);

//         // you can also read received data as byte array
//         /*
//           byte byteArr[8];
//           int state = radio.readData(byteArr, 8);
//         */

//         if (state == RADIOLIB_ERR_NONE) {
//             // packet was successfully received
//             Serial.println(F("Received packet!"));
//             int64_t nowMili = (UTC.dateTime("sv")).toInt() + (UTC.dateTime("i")).toInt() * 60000 + (UTC.dateTime("H")).toInt() * 3600000;

//             // print data of the packet
//             Serial.print(F("Data:\t\t"));
//             Serial.println(str);


//             }


// #ifdef HAS_DISPLAY
//             if (u8g2) {
//                 u8g2->clearBuffer();
//                 char buf[256];
//                 u8g2->drawStr(0, 12, "Received OK!");
//                 u8g2->drawStr(5, 26, str.c_str());
//                 snprintf(buf, sizeof(buf), "RSSI:%.2f", radio.getRSSI());
//                 u8g2->drawStr(0, 40, buf);
//                 snprintf(buf, sizeof(buf), "SNR:%.2f", radio.getSNR());
//                 u8g2->drawStr(0, 54, buf);
//                 u8g2->sendBuffer();
//             }
// #endif

//         } else if (state == RADIOLIB_ERR_CRC_MISMATCH) {
//             // packet was received, but is malformed
//             Serial.println(F("CRC error!"));

//         } else {
//             // some other error occurred
//             Serial.print(F("Failed, code "));
//             Serial.println(state);
//         }

//         // put module back to listen mode
//         radio.startReceive();

//         // we're ready to receive more packets,
//         // enable interrupt service routine
//         enableInterrupt = true;
//     }
// }


/*
   RadioLib SX1276 Receive/Transmission Example
   with RX interupt/ TX interupt trigger for Lilygo LoRa 2.1_1.6V module.
*/


#include <RadioLib.h>
#include "utilities.h"
#include "boards.h"

SX1276 radio = new Module(RADIO_CS_PIN, RADIO_DIO0_PIN, RADIO_RST_PIN, RADIO_BUSY_PIN);

#define LoRa_frequency 923.0
#define LoRa_TX_PIN 34 // for TX trigger
#define Dummy_TX_Inteval 10000 // in ms
#define Spreadf 7
#define repeatSF 8

String str;

volatile bool actionFlag = false; // Flag to indicate that a packet was received or sent
volatile bool isTransmitting = false; // Flag to identify TX or RX
volatile bool enableInterrupt = true; // Flag to enable interrupt

unsigned long previousTransmissionTime = 0; // Variable to store the previous transmission time

// save transmission state between loops
int transmissionState = RADIOLIB_ERR_NONE;

// Interrupt handler for both transmit and receive
void handleInterrupt() {
    if (!enableInterrupt) {
        return;
    }

    actionFlag = true;
}

void startTransmission() {
    Serial.print(F("Sending back ... "));
    isTransmitting = true;
    radio.setSpreadingFactor(Spreadf);
    //transmissionState = radio.startTransmit("Hello World!"); // this is non-blocking action, meaning the radio is transmitting, the execution of other tasks are not on hold
    transmissionState = radio.startTransmit(str); // this is non-blocking action, meaning the radio is transmitting, the execution of other tasks are not on hold

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
        radio.setSpreadingFactor(7);
        radio.setOutputPower(17);
        radio.setBandwidth(125);
        radio.setCurrentLimit(120);
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

    // if (digitalRead(LoRa_TX_PIN) == HIGH) {
    //     startTransmission();
    // }

    // if(millis() - previousTransmissionTime >= Dummy_TX_Inteval){ // for now, just send a dummy packet every 10 seconds with random delay
    //     previousTransmissionTime = millis();
    //     // wait random amount of time between 1 and 5 seconds
    //     delay(random(1000, 5000));
    //     startTransmission();
    // }

    // check if the flag is triggerd
    if (actionFlag){
        actionFlag = false;
        enableInterrupt = false;

        if (isTransmitting) {
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
            radio.setSpreadingFactor(Spreadf);
            #ifdef HAS_DISPLAY
            if (u8g2) {
                char buf[256];
                u8g2->clearBuffer();
                u8g2->drawStr(0, 12, "Transmitting: OK!");
                snprintf(buf, sizeof(buf), "Rate:%.2f bps", radio.getDataRate());
                u8g2->drawStr(5, 30, buf);
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
            Serial.println(F("Received packet!"));
            Serial.println(str);
            startTransmission();
            // print RSSI, SNR and frequency offset
            Serial.print(F("RSSI:\t\t"));
            Serial.print(radio.getRSSI());
            Serial.println(F(" dBm"));

            Serial.print(F("SNR:\t\t"));
            Serial.print(radio.getSNR());
            Serial.println(F(" dB"));

            Serial.print(F("Frequency error:\t"));
            Serial.print(radio.getFrequencyError());
            Serial.println(F(" Hz"));
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
        
        }
        else if (receptionstate == RADIOLIB_ERR_CRC_MISMATCH){
            // packet was received, but is malformed
            Serial.println(F("CRC error!"));
        }
        else{
            Serial.print(F("Failed, code "));
            Serial.println(receptionstate);
        }
        // put module back to listen mode
        radio.startReceive();
        // enable interrupt service routine

    }



}
 enableInterrupt = true;

}

