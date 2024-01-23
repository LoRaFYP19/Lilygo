
#include <RadioLib.h>
#include "utilities.h"
#include "boards.h"
#include <WiFi.h>
#include <HTTPClient.h>
#include <ezTime.h>

SX1276 radio = new Module(RADIO_CS_PIN, RADIO_DIO0_PIN, RADIO_RST_PIN, RADIO_BUSY_PIN);

#define LoRa_frequency 923.0
#define SpreadF 7
#define OPower 17
#define Bandwidth 125
#define CurrentLimit 120
#define PreAmbleLength 6

// flag to indicate that a packet was received
#define nodeId 0

const char* ssid = "F1";
const char* password = "123456789";

volatile bool receivedFlag = false;

// disable interrupt when it's not needed
volatile bool enableInterrupt = true;

//Your Domain name with URL path or IP address with path
String serverName = "https://script.google.com/macros/s/AKfycbzmBz3kstwiNjQVGuUoN_GEmpV8hnVNwppzb4oytaz1lWB4em3UliuXozVI1Pmg2BRvNA/exec";

bool txSheet = false;
int rxNumber;

int64_t maxRSSI=-1000;
int64_t minRSSI=1000;
int64_t totalTimeOnAir;
int64_t minTimeOnAir=100000;
int64_t maxTimeOnAir;
int64_t tolRSSI;
int64_t tolrxSize=0;
int64_t defMili;
int64_t rssi,rxSize;
int64_t totalSnr=0;
int64_t Snr;
int64_t tolFrqError=0;
int64_t FrqError;

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
    rxNumber=0;
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

void sendDataToSheet(){
  Serial.println("Sending the Data");
  if(WIFI.status() != WL_CONNECTED){
    WiFi.disconnect();
    delay(1000);
    WiFi.begin(ssid, password);
  }
   if(WiFi.status()== WL_CONNECTED){

     HTTPClient http;
     
     String serverPath = serverName + "?SF=" + String(SpreadF) + "&txP=" + String(OPower) + "&tolRxNum=" + String(rxNumber) + "&avgRssi=" + String(tolRSSI/rxNumber) + "&maxRssi=" + String(maxRSSI) + "&minRssi=" + String(minRSSI) +"&avgRxSize="+String(tolrxSize/rxNumber) +"&avgtoa=" + String(totalTimeOnAir/rxNumber) + "&minton=" + String(minTimeOnAir) + "&maxton=" + String(maxTimeOnAir)+"&nodeId="+String(nodeId)+"&snr="+String(totalSnr/rxNumber)+"&frqError="+String(tolFrqError/rxNumber);
     
     Serial.println("SF "+String(SpreadF)+" Output Power: "+String(OPower) +" Total Rx Number: " + String(rxNumber) + " Average RSSI: " + String(tolRSSI/rxNumber) + " Max RSSI: " + String(maxRSSI) + " Min RSSI: " + String(minRSSI) + " Total Time on Air: " + String(totalTimeOnAir)+ " Average Time On Air: "+ String(totalTimeOnAir/rxNumber) + " Min Time on Air: " + String(minTimeOnAir) + " Max Time on Air: " + String(maxTimeOnAir)+" Average SNR: "+String(totalSnr/rxNumber)+" Average Frequency Error: "+String(tolFrqError/rxNumber));
     // Your Domain name with URL path or IP address with path
     http.begin(serverPath.c_str());
     // If you need Node-RED/server authentication, insert user and password below
     //http.setAuthorization("REPLACE_WITH_SERVER_USERNAME", "REPLACE_WITH_SERVER_PASSWORD");
     // Send HTTP GET request
     int httpResponseCode = http.GET();
     if (httpResponseCode>0) {
       Serial.print("HTTP Response code: ");
       Serial.println(httpResponseCode);
       String payload = http.getString();
       Serial.println(payload);
       delay(1000);
       txSheet = true;
       rxNumber = 0;
       tolRSSI = 0;
       maxRSSI = -1000;
       minRSSI = 1000;
       totalTimeOnAir =0;
       maxTimeOnAir = 0;
       tolrxSize = 0;
       minTimeOnAir=100000;
       totalSnr = 0;
       tolFrqError = 0;
       
     }
     else {
       Serial.print("Error code: ");
       Serial.println(httpResponseCode);
     }
     // Free resources
     http.end();
   }
   else {
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
            
            if(strcmp(str.c_str(),"Tx Done")==0){
                radio.clearDio0Action()
                Serial.println("Test is Done");
                Serial.println("SF "+String(SpreadF)+" Output Power: "+String(OPower) +" Total Rx Number: " + String(rxNumber) + " Average RSSI: " + String(tolRSSI/rxNumber) + " Max RSSI: " + String(maxRSSI) + " Min RSSI: " + String(minRSSI) + " Total Time on Air: " + String(totalTimeOnAir)+ " Average Time On Air: "+ String(totalTimeOnAir/rxNumber) + " Min Time on Air: " + String(minTimeOnAir) + " Max Time on Air: " + String(maxTimeOnAir)+" Average SNR: "+String(totalSnr/rxNumber)+" Average Frequency Error: "+String(tolFrqError/rxNumber));
                if (txSheet == false){
                  sendDataToSheet();

                }
                else{
                  Serial.println("Data is already sent to sheet");
                }
            }
            else{

                int64_t rxMili = 0;
                Serial.println(rxNumber);
                for (int i = 0; i < str.length(); i++) {
                    char hex_digit = str[i];
                    int digit_value = (hex_digit >= '0' && hex_digit <= '9') ? hex_digit - '0' : hex_digit - 'A' + 10;
                    rxMili = (rxMili << 4) | digit_value;
                }
                Serial.println(rxMili);

                txSheet = false;
                rxNumber++;
                rssi = radio.getRSSI();
                tolRSSI += rssi;
                if(rssi>maxRSSI){
                  maxRSSI = rssi;
                }
                if(rssi<minRSSI){
                  minRSSI = rssi;
                }
                rxSize = str.length();
                tolrxSize += rxSize;

                int64_t defMili = nowMili - rxMili;
                totalTimeOnAir += defMili;

                if(defMili>maxTimeOnAir){
                  maxTimeOnAir = defMili;
                }
                if(defMili<minTimeOnAir){
                  minTimeOnAir = defMili;
                }

                // get snr
                Snr = radio.getSNR();
                totalSnr += Snr;
                // get frequency error
                FrqError = radio.getFrequencyError();
                tolFrqError += FrqError;

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
