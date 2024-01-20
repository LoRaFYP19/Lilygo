
#include <RadioLib.h>
#include "utilities.h"
#include "boards.h"

SX1276 radio = new Module(RADIO_CS_PIN, RADIO_DIO0_PIN, RADIO_RST_PIN, RADIO_BUSY_PIN);

#define LoRa_frequency 923.0
// flag to indicate that a packet was received

const char* ssid = "F1";
const char* password = "123456789";

volatile bool receivedFlag = false;

// disable interrupt when it's not needed
volatile bool enableInterrupt = true;

//Your Domain name with URL path or IP address with path
String serverName = "https://script.google.com/macros/s/AKfycby1U29__LQSKpwApoWNd__NgkccmfbXG_3M7x0h7sirvTr-11KwPZMqW-rmLzdkymFgkA/exec";

bool txSheet = false;
int64_t rxNumber;

int64_t maxRSSI=-1000;
int64_t minRSSI=1000;
int64_t totalTimeOnAir;
int64_t minTimeOnAir=100000;
int64_t maxTimeOnAir;
int64_t tolRSSI;
int64_t tolrxSize;
int64_t rxMili,nowMili,defMili;
int64_t tmiliMin,tmiliH;
int64_t rssi,rxSize;

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
    txNumber=0;
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
        radio.setOutputPower(17);
        radio.setBandwidth(125);
        radio.setCurrentLimit(120);
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


    // if needed, 'listen' mode can be disabled by calling
    // any of the following methods:
    //
    // radio.standby()
    // radio.sleep()
    // radio.transmit();
    // radio.receive();
    // radio.readData();
    // radio.scanChannel();
}

void sendDataToSheet(){
   if(WiFi.status()== WL_CONNECTED){
     HTTPClient http;
// var SF = Number(e.parameter.SF);
// var CR = Number(e.parameter.CR);
// var txP= Number(e.parameter.txP);
// var tolRxNum = Number(e.parameter.tolRxNum);
// var avgRssi = Number(e.parameter.avgRssi);
// var maxRssi = Number(e.parameter.maxRssi);
// var minRssi = Number(e.parameter.minRssi);
// var avgtoa = Number(e.parameter.avgtoa);
// var minton = Number(e.parameter.minton);
// var maxton = Number(e.parameter.maxton);

     String serverPath = serverName + "?SF=" + String(LORA_SPREADING_FACTOR) + "&CR=" + String(LORA_CODINGRATE) + "&txP=" + String(TX_OUTPUT_POWER) + "&tolRxNum=" + String(rxNumber) + "&avgRssi=" + String(tolRSSI/rxNumber) + "&maxRssi=" + String(maxRSSI) + "&minRssi=" + String(minRSSI) + "&avgtoa=" + String(totalTimeOnAir/rxNumber) + "&minton=" + String(minTimeOnAir) + "&maxton=" + String(maxTimeOnAir);
     Serial.println("Total RSSI: " + String(tolRSSI) + " Total Rx Number: " + String(rxNumber) + " Average RSSI: " + String(tolRSSI/rxNumber) + " Max RSSI: " + String(maxRSSI) + " Min RSSI: " + String(minRSSI) + " Total Time on Air: " + String(totalTimeOnAir) + " Min Time on Air: " + String(minTimeOnAir) + " Max Time on Air: " + String(maxTimeOnAir));
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
   
 }



void loop()
{
    // check if the flag is set
    if (receivedFlag) {
        // disable the interrupt service routine while
        // processing the data
        enableInterrupt = false;

        // reset flag
        receivedFlag = false;

        // you can read received data as an Arduino String
        String str;
        int state = radio.readData(str);

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

            // print RSSI (Received Signal Strength Indicator)
            Serial.print(F("RSSI:\t\t"));
            Serial.print(radio.getRSSI());
            Serial.println(F(" dBm"));

            // print SNR (Signal-to-Noise Ratio)
            Serial.print(F("SNR:\t\t"));
            Serial.print(radio.getSNR());
            Serial.println(F(" dB"));

            // print frequency error
            Serial.print(F("Frequency error:\t"));
            Serial.print(radio.getFrequencyError());
            Serial.println(F(" Hz"));
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
