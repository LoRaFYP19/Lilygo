/*
   RadioLib SX1276 Single SF relay node
   with RX interupt/ TX interupt trigger for Lilygo LoRa 2.1_1.6V module.
*/


#include <RadioLib.h>
#include "utilities.h"
#include "boards.h"



SX1276 radio = new Module(RADIO_CS_PIN, RADIO_DIO0_PIN, RADIO_RST_PIN, RADIO_BUSY_PIN);

#define LoRa_frequency 923.0

unsigned long previousMillis = 0;
unsigned long timeout=60000;

int Spreadf= 8;
int OutputPower = 2;
float Bandwidth = 125;
int codeRate = 5;
#define repeatSF 8
#define PreAmbleLength 6
#define wanSync 0x34

// Queue buffer
#define MAX_PACKETS 5
String packetBuffer[MAX_PACKETS];
int head = 0;
int tail = 0;


int crcErrors=0;
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
    // Check if the packet already exists in the buffer
    for (int i = 0; i < MAX_PACKETS; i++) {
        if (packetBuffer[i] == str) {
            // Skip transmission
            Serial.println("Duplicate packet, skipping transmission");
            return;
        }
    }

    Serial.print(F("Sending back ... "));
    isTransmitting = true;
    // add the str to the cache
    radio.setSpreadingFactor(repeatSF);
    //transmissionState = radio.startTransmit("Hello World!"); // this is non-blocking action, meaning the radio is transmitting, the execution of other tasks are not on hold
    transmissionState = radio.startTransmit(str); // this is non-blocking action, meaning the radio is transmitting, the execution of other tasks are not on hold
    packetBuffer[head] = str;
    head = (head + 1) % MAX_PACKETS;
    previousMillis = millis();
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
    
    unsigned long currentMillis = millis();

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
            delay(100);
            startTransmission();
        
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
 if (currentMillis- previousMillis >= timeout) {

        for (int i=0; i<MAX_PACKETS; i++) {
            packetBuffer[i] = "";
        }

    }
    }