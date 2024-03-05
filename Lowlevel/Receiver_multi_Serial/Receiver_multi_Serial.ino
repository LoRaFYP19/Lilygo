
#include <RadioLib.h>
#include "utilities.h"
#include "boards.h"
// #include <WiFi.h>
// #include <HTTPClient.h>
// #include <ezTime.h>

SX1276 radio = new Module(RADIO_CS_PIN, RADIO_DIO0_PIN, RADIO_RST_PIN, RADIO_BUSY_PIN);

#define LoRa_frequency 923.0
#define SpreadF 12
#define OPower 19
#define Bandwidth 125
#define CurrentLimit 120
#define PreAmbleLength 6
#define wanSync 0x34

// flag to indicate my node id
#define mynodeId 0

volatile bool receivedFlag = false;

// disable interrupt when it's not needed
volatile bool enableInterrupt = true;
volatile unsigned long startTime;
volatile unsigned long endTime;

#define MAX_NODES 2 // Adjust this based on the maximum number of nodeIds


struct NodeState {
    int64_t maxRSSI;
    int64_t minRSSI;
    int64_t totalTimeOnAir;
    int64_t minTimeOnAir;
    int64_t maxTimeOnAir;
    int64_t tolRSSI;
    int64_t tolrxSize;
    int64_t tolFrqError;
    int64_t totalSnr;
    // bool txSheet;
    int rxNumber;

    // Default constructor to set initial values
    NodeState() :
        maxRSSI(-1000),
        minRSSI(1000),
        totalTimeOnAir(0),
        minTimeOnAir(100000),
        maxTimeOnAir(0),
        tolRSSI(0),
        tolrxSize(0),
        tolFrqError(0),
        totalSnr(0),
        // txSheet(false),  // Set to true to indicate that the sheet has not been sent yet
        rxNumber(0) {
    }
};


NodeState nodeStates[MAX_NODES];

// this function is called when a complete packet
// is received by the module
// IMPORTANT: this function MUST be 'void' type
//            and MUST NOT have any arguments!
void setFlag(void)
{
    startTime = millis();
    // check if the interrupt is enabled
    if (!enableInterrupt) {
        return;
    }

    // we got a packet, set the flag
    receivedFlag = true;
    
    
}

void setup()
{
    Serial.begin(115200);
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
        radio.setSpreadingFactor(SpreadF);
        radio.setOutputPower(OPower);
        radio.setBandwidth(Bandwidth);
        radio.setCurrentLimit(CurrentLimit);
        radio.setPreambleLength(PreAmbleLength);
        radio.setSyncWord(wanSync);
    } else {
        Serial.print(F("failed, code "));
        Serial.println(state);
        while (true);
    }
 

    // when new packet is received, interrupt
    radio.setDio0Action(setFlag, RISING);
    // set the function that will be called
    // when LoRa preamble is detected
    // radio.setDio1Action(setFlagDetected, RISING);

    // start listening for LoRa packets
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


void loop()
{
    // check if the flag is set
    // events();
    if (receivedFlag) {
        // disable the interrupt service routine while
        // processing the data
        enableInterrupt = false;

        // reset flag
        receivedFlag = false;

        // you can read received data as an Arduino String
        String str;
        int state = radio.readData(str);
        endTime = millis();
        Serial.print("Initial p rec ");
        Serial.print(str);

        // you can also read received data as byte array
        /*
          byte byteArr[8];
          int state = radio.readData(byteArr, 8);
        */

        if (state == RADIOLIB_ERR_NONE) {
            // packet was successfully received
            Serial.println(F("Received packet!"));

            // print data of the packet
            Serial.print(F("Data:\t\t"));
            Serial.println(str);
            // extract nodeid and time in separate strings
            String nodeid = str.substring(0, str.indexOf(","));
            String time = str.substring(str.indexOf(",") + 1, str.length());
            int nodeId = nodeid.toInt();

            if(strcmp(time.c_str(),"Tx Done")==0){
                Serial.println("Test is Done");
            }
            else{

                nodeStates[nodeId].rxNumber = nodeStates[nodeId].rxNumber + 1;
                int64_t rssi = radio.getRSSI();
                nodeStates[nodeId].tolRSSI += rssi;
                if (rssi > nodeStates[nodeId].maxRSSI) {
                    nodeStates[nodeId].maxRSSI = rssi;
                }
                if (rssi < nodeStates[nodeId].minRSSI) {
                    nodeStates[nodeId].minRSSI = rssi;
                }

                int64_t rxSize = str.length();
                nodeStates[nodeId].tolrxSize += rxSize;

                // Example: Update totalSnr for the specific nodeId
                int64_t Snr = radio.getSNR();
                nodeStates[nodeId].totalSnr += Snr;

                // Example: Update tolFrqError for the specific nodeId
                int64_t FrqError = radio.getFrequencyError();
                nodeStates[nodeId].tolFrqError += FrqError;
//                Serial.println("Time TX: ");
//                Serial.print(endTime);
//                Serial.print(",");
//                Serial.print(startTime);
                // serial print all the values separatd by comma
                Serial.println(str+","+String(SpreadF)+","+String(OPower)+","+String(nodeStates[nodeId].rxNumber)+","+String(rssi)+","+String(nodeStates[nodeId].maxRSSI)+","+String(nodeStates[nodeId].minRSSI)+","+String(Snr)+","+String(FrqError)+","+String(mynodeId)+","+String(nodeId));
            }

#ifdef HAS_DISPLAY
            if (u8g2) {
                u8g2->clearBuffer();
                char buf[256];
                u8g2->drawStr(0, 12, "Received OK!");
                u8g2->drawStr(5, 26, str.c_str());
                snprintf(buf, sizeof(buf), "RSSI:%.2f", radio.getRSSI());
                u8g2->drawStr(0, 40, buf);
                snprintf(buf, sizeof(buf), "SNR:%.2f", radio.getSNR());
                u8g2->drawStr(0, 54, buf);
                u8g2->sendBuffer();
            }
#endif

        } else if (state == RADIOLIB_ERR_CRC_MISMATCH) {
            // packet was received, but is malformed
            Serial.println(F("CRC error!"));

        } else {
            // some other error occurred
            Serial.print(F("Failed, code "));
            Serial.println(state);
        }

        // put module back to listen mode
        radio.startReceive();

        // we're ready to receive more packets,
        // enable interrupt service routine
        enableInterrupt = true;
    }
}
