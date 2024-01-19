#include <RadioLib.h>
#include "boards.h"
#include <ezTime.h>
#include <WiFi.h>


SX1276 radio = new Module(RADIO_CS_PIN, RADIO_DIO0_PIN, RADIO_RST_PIN, RADIO_BUSY_PIN);

#define LoRa_frequency 923.0

// save transmission state between loops
int transmissionState = RADIOLIB_ERR_NONE;
int maxNumOfPackets = 100;
const char* ssid = "V1";
const char* password = "123456789";
int txNumber=0;

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
    } else {
        Serial.print(F("failed, code "));
        Serial.println(state);
        while (true);
    }

    // when packet transmission is finished
    radio.setDio0Action(setFlag, RISING);

    // start transmitting the first packet
    Serial.print(F("Sending first packet ... "));

    // you can transmit C-string or Arduino string up to
    // 256 characters long
    transmissionState = radio.startTransmit("Hello World!");

    // you can also transmit byte array up to 256 bytes long
    /*
      byte byteArr[] = {0x01, 0x23, 0x45, 0x67,
                        0x89, 0xAB, 0xCD, 0xEF};
      state = radio.startTransmit(byteArr, 8);
    */
}

void int64ToHexString(int64_t value, char* buffer, size_t size) {
    snprintf(buffer, size, "%llx", static_cast<unsigned long long>(value));
}


void sendPacket()
{
    // send packet
    txNumber++;
     char txpacket[20];
    if (txNumber <= maxNumOfPackets)
    {
        // int64_t tMili = (UTC.dateTime("sv")).toInt();
        // int tmiliMin = (UTC.dateTime("i")).toInt();
        // int64_t tmiliH = (UTC.dateTime("H")).toInt();
        // tMili = tmiliMin * 60000 + tMili + tmiliH * 3600000;

        int64_t tMili = (UTC.dateTime("sv")).toInt() + (UTC.dateTime("i")).toInt() * 60000 + (UTC.dateTime("H")).toInt() * 3600000;

       
        int64ToHexString(tMili, txpacket, sizeof(txpacket));

    }

        else if(txNumber <= 550){
      
        sprintf(txpacket,"Tx Done");  //start a package
   

    }
    else{
      txNumber = 0; //reset the counter  
      }
    transmissionState = radio.startTransmit(tMiliHexStr);
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
            Serial.println(F("transmission finished!"));

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

        // wait a second before transmitting again
        delay(1000);

        // send another one
        Serial.print(F("Sending another packet ... "));
        sendPacket();

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
}