#include <RadioLib.h>
#include "boards.h"
#include <ezTime.h>
#include <WiFi.h>


SX1276 radio = new Module(RADIO_CS_PIN, RADIO_DIO0_PIN, RADIO_RST_PIN, RADIO_BUSY_PIN);

#define LoRa_frequency 923.0

// save transmission state between loops
int transmissionState = RADIOLIB_ERR_NONE;
int maxNumOfPackets =100;
const char* ssid = "F1";
const char* password = "123456789";
int txNumber=0;

#define IntervalForSend 2000

// flag to indicate that a packet was sent
volatile bool transmittedFlag = false;

// disable interrupt when it's not needed
volatile bool enableInterrupt = true;

void setFlag(void)
{
    // check if the interrupt is enabled
    if (!enableInterrupt) {
        return;
    }
    // we sent a packet, set the flag
    transmittedFlag = true;
}

void setup()
{
    Serial.begin(115200);
    initBoard();
    // When the power is turned on, a delay is required.
    delay(1500);

    WiFi.begin(ssid, password);
    Serial.println("Connecting");
    while(WiFi.status() != WL_CONNECTED) {
      delay(500);
      Serial.print(".");
    }
    delay(5000);
    Serial.println("");
    Serial.print("Connected to WiFi network with IP Address: ");
    Serial.println(WiFi.localIP());
    
    setInterval(60);
    
    waitForSync();
    String val_1 = UTC.dateTime("l, d-M-y H:i:s.v T");

    Serial.println(val_1);
    
    txNumber=0;

    // initialize SX1276 with default settings
    Serial.print(F("Initializing ... "));
#ifndef LoRa_frequency
    int state = radio.begin(923.0);
#else
    int state = radio.begin(LoRa_frequency);
#endif

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
        radio.setOutputPower(17);
        radio.setBandwidth(125);
        radio.setCurrentLimit(120);
        radio.setSpreadingFactor(7);
    } else {
        Serial.print(F("failed, code "));
        Serial.println(state);
        while (true);
    }

    // when packet transmission is finished
    radio.setDio0Action(setFlag, RISING);

}

void int64ToHexString(int64_t value, char* buffer, size_t size) {
    snprintf(buffer, size, "%llx", static_cast<unsigned long long>(value));
}


void sendPacket()
{
    // send packet
    txNumber++;
    Serial.println("Trying to send the packet number: ");
    Serial.println(txNumber);
    char txpacket[20];
    if (txNumber <= maxNumOfPackets)
    {
        int64_t tMili = (UTC.dateTime("sv")).toInt() + (UTC.dateTime("i")).toInt() * 60000 + (UTC.dateTime("H")).toInt() * 3600000;
       
        int64ToHexString(tMili, txpacket, sizeof(txpacket));
        transmissionState = radio.startTransmit(txpacket);

    }

        else if(txNumber <= maxNumOfPackets + 10){
      
        sprintf(txpacket,"Tx Done");  //start a package
        Serial.println("Done Payloads sending Tx Done");
        Serial.println(txpacket);
        transmissionState = radio.startTransmit(txpacket);
        delay(3000); // additionnal delay to be sure that the receiver got the message without spaming the channel
   

    }
    else{
      txNumber = 0; //reset the counter

      }

    
}

void loop()
{
    events();
    // check if the previous transmission finished
    if (transmittedFlag) {
        // disable the interrupt service routine while
        // processing the data
        enableInterrupt = false;

        // reset flag
        transmittedFlag = false;

        if (transmissionState == RADIOLIB_ERR_NONE) {
            // packet was successfully sent
            Serial.println(txNumber);
            Serial.println(F("packet got transmission finished!"));

            // NOTE: when using interrupt-driven transmit method,
            //       it is not possible to automatically measure
            //       transmission data rate using getDataRate()

        } else {
            Serial.print(F("failed, code "));
            Serial.println(transmissionState);

        }

        // clean up after transmission is finished
        // this will ensure transmitter is disabled,
        // RF switch is powered down etc.
        radio.finishTransmit();

        // you can transmit C-string or Arduino string up to
        

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
        // we're ready to send more packets,
        // enable interrupt service routine
        enableInterrupt = true;
    }
    
    // send packet every 2 seconds using milis()
    if (millis() % IntervalForSend == 0) {
        sendPacket();
    }
}