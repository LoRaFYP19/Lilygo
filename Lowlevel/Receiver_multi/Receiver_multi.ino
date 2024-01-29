
#include <RadioLib.h>
#include "utilities.h"
#include "boards.h"
#include <WiFi.h>
#include <HTTPClient.h>
#include <ezTime.h>

SX1276 radio = new Module(RADIO_CS_PIN, RADIO_DIO0_PIN, RADIO_RST_PIN, RADIO_BUSY_PIN);

#define LoRa_frequency 923.0
#define SpreadF 8
#define OPower 17
#define Bandwidth 125
#define CurrentLimit 120
#define PreAmbleLength 6

// flag to indicate my node id
#define mynodeId 0

const char* ssid = "F1";
const char* password = "123456789";

volatile bool receivedFlag = false;

// disable interrupt when it's not needed
volatile bool enableInterrupt = true;
#define MAX_NODES 2 // Adjust this based on the maximum number of nodeIds

//Your Domain name with URL path or IP address with path
String serverName = "https://script.google.com/macros/s/AKfycbzmBz3kstwiNjQVGuUoN_GEmpV8hnVNwppzb4oytaz1lWB4em3UliuXozVI1Pmg2BRvNA/exec";


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
    bool txSheet;
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
        txSheet(false),  // Set to true to indicate that the sheet has not been sent yet
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
    WiFi.disconnect();
    delay(1000);
    WiFi.begin(ssid, password);
    Serial.println("Connecting");
    while(WiFi.status() != WL_CONNECTED) {
      delay(500);
      Serial.print(".");
    }

    Serial.println("");
    Serial.print("Connected to WiFi network with IP Address: ");
    Serial.println(WiFi.localIP());

    setInterval(60);
    waitForSync();
    String val_1 = UTC.dateTime("l, d-M-y H:i:s.v T");
    // int sec_1 = (val_1.substring(24,27)).toInt();
    // int msec_1 = (val_1.substring(27,30)).toInt();
    Serial.println(val_1);
    // rxNumber=0;
    rssi=0;


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
    } else {
        Serial.print(F("failed, code "));
        Serial.println(state);
        while (true);
    }
 

    // when new packet is received, interrupt
    radio.setDio0Action(setFlag, RISING);

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


void sendDataToSheet(int nodeId) {
    Serial.println("Sending the Data");

    if (WiFi.status() != WL_CONNECTED) {
        WiFi.disconnect();
        delay(1000);
        WiFi.begin(ssid, password);
    }

    if (WiFi.status() == WL_CONNECTED) {
        HTTPClient http;

        // Modify the serverPath accordingly based on the nodeId
        // String serverPath = serverName + "?SF=" + String(SpreadF) + "&txP=" + String(OPower) + "&tolRxNum=" + String(nodeStates[nodeId].rxNumber) +
        //                     "&nodeId=" + String(nodeId) + "&...";
        String serverPath = serverName + "?SF=" + String(SpreadF) + "&txP=" + String(OPower) + "&tolRxNum=" + String(nodeStates[nodeId].rxNumber) +
                    "&nodeId=" + String(mynodeId) + "&rxNodeId=" + String(nodeId) +
                    "&maxRSSI=" + String(nodeStates[nodeId].maxRSSI) + "&minRSSI=" + String(nodeStates[nodeId].minRSSI) +
                    "&totalTimeOnAir=" + String(nodeStates[nodeId].totalTimeOnAir) +
                    "&avgTimeOnAir=" + String(nodeStates[nodeId].totalTimeOnAir / nodeStates[nodeId].rxNumber) +
                    "&minTimeOnAir=" + String(nodeStates[nodeId].minTimeOnAir) +
                    "&maxTimeOnAir=" + String(nodeStates[nodeId].maxTimeOnAir) +
                    "&avgSNR=" + String(nodeStates[nodeId].totalSnr / nodeStates[nodeId].rxNumber) +
                    "&avgFrequencyError=" + String(nodeStates[nodeId].tolFrqError / nodeStates[nodeId].rxNumber);


        Serial.println("SF " + String(SpreadF) + " Output Power: " + String(OPower) +
               " Total Rx Number: " + String(nodeStates[nodeId].rxNumber) +
               " Average RSSI: " + String(nodeStates[nodeId].tolRSSI / nodeStates[nodeId].rxNumber) +
               " Max RSSI: " + String(nodeStates[nodeId].maxRSSI) +
               " Min RSSI: " + String(nodeStates[nodeId].minRSSI) +
               " Total Time on Air: " + String(nodeStates[nodeId].totalTimeOnAir) +
               " Average Time On Air: " + String(nodeStates[nodeId].totalTimeOnAir / nodeStates[nodeId].rxNumber) +
               " Min Time on Air: " + String(nodeStates[nodeId].minTimeOnAir) +
               " Max Time on Air: " + String(nodeStates[nodeId].maxTimeOnAir) +
               " Average SNR: " + String(nodeStates[nodeId].totalSnr / nodeStates[nodeId].rxNumber) +
               " Average Frequency Error: " + String(nodeStates[nodeId].tolFrqError / nodeStates[nodeId].rxNumber))+
                " NodeId: " + String(mynodeId) + " RxNodeId: " + String(nodeId);



        // Your Domain name with URL path or IP address with path
        http.begin(serverPath.c_str());

        int httpResponseCode = http.GET();
        if (httpResponseCode > 0) {
            Serial.print("HTTP Response code: ");
            Serial.println(httpResponseCode);
            String payload = http.getString();
            Serial.println(payload);
            delay(1000);

            // Reset variables for the specific nodeId
            nodeStates[nodeId].txSheet = true;
            nodeStates[nodeId].maxRSSI = -1000;
            nodeStates[nodeId].minRSSI = 1000;
            nodeStates[nodeId].totalTimeOnAir = 0;
            nodeStates[nodeId].maxTimeOnAir = 0;
            nodeStates[nodeId].tolRSSI = 0;
            nodeStates[nodeId].tolrxSize = 0;
            nodeStates[nodeId].tolFrqError = 0;
            nodeStates[nodeId].totalSnr = 0;
        } else {
            Serial.print("Error code: ");
            Serial.println(httpResponseCode);
        }

        http.end();
    } else {
        Serial.println("WiFi Disconnected");
    }

    radio.setDio0Action(setFlag, RISING);
}

void loop()
{
    // check if the flag is set
    events();
    if (receivedFlag) {
        // disable the interrupt service routine while
        // processing the data
        enableInterrupt = false;

        // reset flag
        receivedFlag = false;

        // you can read received data as an Arduino String
        String str;
        int state = radio.readData(str);
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
            int64_t nowMili = (UTC.dateTime("sv")).toInt() + (UTC.dateTime("i")).toInt() * 60000 + (UTC.dateTime("H")).toInt() * 3600000;

            // print data of the packet
            Serial.print(F("Data:\t\t"));
            Serial.println(str);
            // extract nodeid and time in separate strings
            String nodeid = str.substring(0, str.indexOf(","));
            String time = str.substring(str.indexOf(",") + 1, str.length());
            int nodeId = nodeid.toInt();

            if(strcmp(time.c_str(),"Tx Done")==0){
                radio.clearDio0Action();
                Serial.println("Test is Done");
                Serial.println("SF "+String(SpreadF)+" Output Power: "+String(OPower) +" Total Rx Number: " + String(rxNumber) + " Average RSSI: " + String(tolRSSI/rxNumber) + " Max RSSI: " + String(maxRSSI) + " Min RSSI: " + String(minRSSI) + " Total Time on Air: " + String(totalTimeOnAir)+ " Average Time On Air: "+ String(totalTimeOnAir/rxNumber) + " Min Time on Air: " + String(minTimeOnAir) + " Max Time on Air: " + String(maxTimeOnAir)+" Average SNR: "+String(totalSnr/rxNumber)+" Average Frequency Error: "+String(tolFrqError/rxNumber));
                if (!nodeStates[nodeId].txSheet) {
                    sendDataToSheet(nodeId);
                }

                else{
                  Serial.println("Data is already sent to sheet");
                }
            }
            else{

                int64_t rxMili = 0;
                Serial.println(rxNumber);
                for (int i = 0; i < time.length(); i++) {
                    char hex_digit = time[i];
                    int digit_value = (hex_digit >= '0' && hex_digit <= '9') ? hex_digit - '0' : hex_digit - 'a' + 10;
                    rxMili = (rxMili << 4) | digit_value;
                }
                Serial.println(rxMili);

                txSheet = false;
                // rxNumber++;
                nodeStates[nodeId].rxNumber++;
                rssi = radio.getRSSI();
                // tolRSSI += rssi;
                nodeStates[nodeId].tolRSSI += rssi;

                if (rssi > nodeStates[nodeId].maxRSSI) {
                    nodeStates[nodeId].maxRSSI = rssi;
                }
                if (rssi < nodeStates[nodeId].minRSSI) {
                    nodeStates[nodeId].minRSSI = rssi;
                }

                int64_t rxSize = str.length();
                nodeStates[nodeId].tolrxSize += rxSize;

                int64_t defMili = nowMili - rxMili;
                nodeStates[nodeId].totalTimeOnAir += defMili;
                if (defMili > nodeStates[nodeId].maxTimeOnAir) {
                    nodeStates[nodeId].maxTimeOnAir = defMili;
                }

                // Example: Update minTimeOnAir for the specific nodeId
                if (defMili < nodeStates[nodeId].minTimeOnAir) {
                    nodeStates[nodeId].minTimeOnAir = defMili;
                }

                // Example: Update totalSnr for the specific nodeId
                int64_t Snr = radio.getSNR();
                nodeStates[nodeId].totalSnr += Snr;

                // Example: Update tolFrqError for the specific nodeId
                int64_t FrqError = radio.getFrequencyError();
                nodeStates[nodeId].tolFrqError += FrqError;


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
