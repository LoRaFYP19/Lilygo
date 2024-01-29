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

int numberofpackets=0;
String str;

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

void startTransmission() {
    Serial.print(F("Sending back ... "));
    isTransmitting = true;
    radio.setSpreadingFactor(repeatSF);
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
        radio.setSpreadingFactor(Spreadf);
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
            numberofpackets++;
            Serial.println(str);
            Serial.println(numberofpackets);
            
            // // print RSSI, SNR and frequency offset
            // Serial.print(F("RSSI:\t\t"));
            // Serial.print(radio.getRSSI());
            // Serial.println(F(" dBm"));

            // Serial.print(F("SNR:\t\t"));
            // Serial.print(radio.getSNR());
            // Serial.println(F(" dB"));

            // Serial.print(F("Frequency error:\t"));
            // Serial.print(radio.getFrequencyError());
            // Serial.println(F(" Hz"));
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
            delay(100);
            startTransmission();
        
        }
        else if (receptionstate == RADIOLIB_ERR_CRC_MISMATCH){
            // packet was received, but is malformed
            Serial.println(F("CRC error!"));
        }
        else{
            Serial.print(F("Failed, code "));
            Serial.println(receptionstate);
        }

    }

}
 enableInterrupt = true;

}

