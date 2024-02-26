#include <RadioLib.h>
#include "boards.h"



SX1276 radio = new Module(RADIO_CS_PIN, RADIO_DIO0_PIN, RADIO_RST_PIN, RADIO_BUSY_PIN);

#define LoRa_frequency 923.0
#define nodeID 0
// save transmission state between loops
int transmissionState = RADIOLIB_ERR_NONE;
int maxNumOfPackets =100;
int txNumber=0;

unsigned long previousMillis = 0;
unsigned long interval;

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

void generateRandomInterval() {
  // Generate a random number between 5000 and 9000 milliseconds (5 to 9 seconds)
  interval = random(2000, 4001);
}


void setup()
{
    Serial.begin(115200);
    initBoard();
    // When the power is turned on, a delay is required.
    delay(1500);

    randomSeed(analogRead(0));  // Seed the random number generator
    generateRandomInterval();

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
        radio.setSpreadingFactor(8);
    } else {
        Serial.print(F("failed, code "));
        Serial.println(state);
        while (true);
    }

    // when packet transmission is finished
    radio.setDio0Action(setFlag, RISING);

}

void int64ToHexString(int64_t value, char* buffer, size_t size) {
    // snprintf(buffer, size, "%llx", static_cast<unsigned long long>(value)); // for default payload
    
    snprintf(buffer, size, "%d,%09llx", nodeID, static_cast<unsigned long long>(value)); // payload with node ID and padded value
}

void sendPacket()
{
    // send packet
    txNumber++;

    char txpacket[20];
    if (txNumber <= maxNumOfPackets)
    {
        // int64_t tMili = 888+20*60000+10*3600000; //(UTC.dateTime("sv")).toInt() + (UTC.dateTime("i")).toInt() * 60000 + (UTC.dateTime("H")).toInt() * 3600000;
        int64_t tMili = txNumber;

       
        int64ToHexString(tMili, txpacket, sizeof(txpacket));
        // add the node ID to the packet front with a comma separated
     

        transmissionState = radio.startTransmit(txpacket);
        String str = txpacket;
        Serial.println(str+","+String(txNumber)+","+nodeID);
       

    }

        else if(txNumber <= maxNumOfPackets + 10){
      
        snprintf(txpacket, sizeof(txpacket), "%d,Tx Done", nodeID);
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
    unsigned long currentMillis = millis();
//    events();
    // check if the previous transmission finished
    if (transmittedFlag) {
        // disable the interrupt service routine while
        // processing the data
        enableInterrupt = false;

        // reset flag
        transmittedFlag = false;

        if (transmissionState == RADIOLIB_ERR_NONE) {
            Serial.println(F("packet got transmission finished!"));

            // NOTE: when using interrupt-driven transmit method,
            //       it is not possible to automatically measure
            //       transmission data rate using getDataRate()
            // continue


        } else {
            Serial.print(F("failed code "));
            Serial.println(transmissionState);

        }

        // clean up after transmission is finished
        // this will ensure transmitter is disabled,
        // RF switch is powered down etc.
        radio.finishTransmit();
        

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
    

    if (currentMillis - previousMillis >= interval) {
    // Trigger your action here
    sendPacket();
    previousMillis = currentMillis;
    generateRandomInterval();
  }

}
