
#include <RadioLib.h>
#include "utilities.h"
#include "boards.h"
#include "PubSubClient.h"
#include <WiFi.h>

SX1276 radio = new Module(RADIO_CS_PIN, RADIO_DIO0_PIN, RADIO_RST_PIN, RADIO_BUSY_PIN);

#define LoRa_frequency 923.0


#define PreAmbleLength 6
#define wanSync 0x34
#define nodeID 0

int Spreadf= 8;
int OutputPower = 2;
float Bandwidth = 125;
int codeRate = 5;

int oldSpreadf = Spreadf;
int oldOutputPower = OutputPower;
int oldBandwidth = Bandwidth;
int oldcoderate = codeRate;

//wifi login data
const char* ssid     = "V1";
const char* password = "123456789";

//MQTT
///// MQTT Auth data
const char* mqttServer = "test.mosquitto.org";
const int mqttPort = 1883;
const char* mqttUser = "";
const char* mqttPassword = "";
const char* clientId = "LoRaGateway1";

// MQTT client setup
WiFiClient espClient;
PubSubClient client(espClient);


unsigned long previousMillis = 0;
unsigned long interval;
bool scheduled = false;
String PrevReconfString;
bool retxSch=false;



unsigned long previousMillis_health = 0;
unsigned long interval_health;
bool scheduled_health = false;


int crcErrors=0;
int numberofpackets=0;
String str;

bool EarthQuack = false;

volatile bool actionFlag = false; // Flag to indicate that a packet was received or sent
volatile bool isTransmitting = false; // Flag to identify TX or RX
volatile bool enableInterrupt = true; // Flag to enable interrupt


// unsigned long previousTransmissionTime = 0; // Variable to store the previous transmission time

// save transmission state between loops
int transmissionState = RADIOLIB_ERR_NONE;


////////////////////////////////////////////////////////////////////////////////////////////
/**
 * Connects to a WiFi network using the provided SSID and password.
 * 
 * This function sets the WiFi mode to station mode and begins the connection process
 * with the provided SSID and password. It continuously checks the connection status
 * until the device is successfully connected to the WiFi network.
 * 
 * Parameters:
 *   - ssid: The SSID of the WiFi network to connect to.
 *   - password: The password of the WiFi network.
 * 
 * Returns:
 *   None.
 */
void wifiConnect() {
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  Serial.print("Connecting to wifi");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println('\n');
}

void wifiReconnect() {
  while (WiFi.status() != WL_CONNECTED) {
    Serial.print("Attempting to reconnect to SSID: ");
    Serial.println(ssid);
    WiFi.begin(ssid, password);
    delay(5000);
  }

//  Serial.println("Connected to wifi");
}


//publish message function to MQTT
void publishMessage(const char* topic, const char* payload) {
  client.publish(topic, payload);
}


//setup function for MQTT
void connectToMQTT() {
  client.setServer(mqttServer, mqttPort);
  while (!client.connected()) {
    Serial.println("Connecting to MQTT broker...");
    if (client.connect(clientId, mqttUser, mqttPassword)) {
      Serial.println("Connected to MQTT broker");
      client.subscribe("CGateway/reconfignew");
      client.subscribe("CGateway/healthnew");
      delay(100);
    //   LCD.clear();
    }
    else {
      Serial.print("Failed with MQTT error code: ");
      Serial.println(client.state());
      delay(100);
    //   LCD.clear();
    }
  }
}

void sendData(String payload) {

  publishMessage("CGateway/logdata", payload.c_str());
}


//mqtt reconnect function
void mqttReconnect() {
  while (!client.connected()) {
    Serial.print("Attempting MQTT connection...");
    wifiReconnect();
    if (client.connect(clientId)) {
      Serial.print(clientId);
      Serial.println(" connected");
      client.subscribe("CGateway/reconfignew");
      client.subscribe("CGateway/healthnew");
    } else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");
      delay(5000);
    }
  }
}



///////////////////////////////////////////////////////////////////////////////////////////////////////////


/**
 * Selects and returns the appropriate parameter based on the given character and index.
 *
 * @param c The character used to determine the parameter value.
 * @param index The index of the parameter to select.
 *
 * @return The selected parameter value.
 *
 * @throws None
 */
float select_params(char c, int index) {
    if (index == 2) { // for spreading factor
        int val = c - '0';
        return val+6;
    }

    if (index == 3) // Bandwidth
    {
        switch(c) {
            case '1': return 10.4;
            case '2': return 15.6;
            case '3': return 20.8;
            case '4': return 31.25;
            case '5': return 41.7;
            case '6': return 62.5;
            case '7': return 125;
            case '8': return 250;
            case '9': return 500;
            default: return 125;
        }
    }
    if (index == 4) // code rate
    {
        return c - '0';
    }

    if (index == 5) // power
    {
        int val = c > '9' ? (c | 0x20) - 'a' + 10 : c - '0';
        return val + 2;
    }

}


/**
 * Set LoRa parameters based on the input string.
 *
 * @param str the input string containing parameters
 *
 * @return void
 *
 * @throws ErrorType if there is an error in setting the parameters
 */
void set_params_lora(String str){
    // for loop for chars from 2 onward
    oldSpreadf = Spreadf;
    oldOutputPower = OutputPower;
    oldBandwidth = Bandwidth;
    oldcoderate = codeRate;
    for (int i = 2; i < str.length(); i++) {
        // store old values
        
        
        // select_params
        float val = select_params(str.charAt(i), i);
        if (i == 2)
        {
            Spreadf = (int)val;
        }
        else if (i==3)
        {
            Bandwidth = val;
        }
        else if (i==4)
        {
            codeRate = (int)val;
        }
        else if (i==5)
        {
            OutputPower = (int)val;
        }

        //for resending parameters
        PrevReconfString = str;
        scheduled=true;
        
    }
    
}



// MQTT callback function
void mqttCallback(char* topic, byte* message, unsigned int length) {
  Serial.print("Message arrived on topic: ");
  Serial.print(topic);
  Serial.print(". Message: ");
  String payload;
  for (int i = 0; i < length; i++) {
    Serial.print((char)message[i]);
    payload += (char)message[i];
  }

  // update alarm data
  if (String(topic) == "CGateway/reconfignew") {
    Serial.print("Recived Data MQTT..");
    set_params_lora(payload);
    setup_lora();
    generateRandomInterval();
    previousMillis = millis(); // this will start countdown for the scheduled re-transmission
    // updateDataFromPayload(payload.c_str()); // initiate the
    sleep(2);
    // LCD.clear();
  }
  if (String(topic) == "CGateway/healthnew") {
    Serial.print("Recived Data MQTT..");
    generateRandomInterval_health();
    previousMillis_health = millis();
    scheduled_health = true;
    sleep(2);
  }
}


// Interrupt handler for both transmit and receive
void handleInterrupt() {
    if (!enableInterrupt) {
        return;
    }

    actionFlag = true;
}

/**
 * Function to generate a random interval between two hardcoded values.
 */
void generateRandomInterval() {
  
  interval = random(5000, 10001);
  
}


/**
 * Function to generate a random interval between two hardcoded values.
 */
void generateRandomInterval_health() {
  
  interval_health = random(8000, 12001);
  
}



/**
 * Converts a Decimal value to a hexadecimal string with optional node ID and padding.
 *
 * @param value the 64-bit integer value to convert
 * @param buffer the buffer to store the resulting hexadecimal string
 * @param size the size of the buffer
 *
 * @return void
 *
 * @throws None
 */
void int64ToHexString(int64_t value, char* buffer, size_t size) {
    // snprintf(buffer, size, "%llx", static_cast<unsigned long long>(value)); // for default payload
    
    snprintf(buffer, size, "%d,%07llx", nodeID, static_cast<unsigned long long>(value)); // payload with node ID and padded value
}


/**
 * Function to start transmission when a earthquack is detected.
 *
 * @param None
 *
 * @return None
 *
 * @throws None
 */
void startTransmission() {
    Serial.print(F("Sending Warning ... "));
    isTransmitting = true;
    char txpacket[20];
    int64_t tMili = 1888+20*60000+10*3600000; // placeholder timestamp
    int64ToHexString(tMili, txpacket, sizeof(txpacket));
    transmissionState = radio.startTransmit(txpacket); // this is non-blocking action, meaning the radio is transmitting, the execution of other tasks are not on hold
}


/**
 * Transmits the previous configuration parameters incase some nodes missed the transmission.
 * @param None
 *
 * @return None
 *
 * @throws None
 */
void startReconfigTransmission() {
    Serial.print(F("Re-sending Warning ... "));
    isTransmitting = true;
    char txpacket[20];
    sprintf(txpacket, "%s", PrevReconfString.c_str());
    transmissionState = radio.startTransmit(txpacket); // this is non-blocking action, meaning the radio is transmitting, the execution of other tasks are not on hold
}

void startHealthTransmission() {
    Serial.print(F("Sending Health Ping ... "));
    isTransmitting = true;
    String myString = "%%";
    myString += String(nodeID);
    char txpacket[20];
    sprintf(txpacket, "%s", myString.c_str());
    transmissionState = radio.startTransmit(txpacket); // this is non-blocking action, meaning the radio is transmitting, the execution of other tasks are not on hold
}

/**
 * Sets up the LoRa module with the old configuration parameters.
 * @param None
 *
 * @return None
 *
 * @throws None
 */
void setup_old_lora(){
        radio.setSpreadingFactor(oldSpreadf);
        radio.setOutputPower(oldOutputPower);
        radio.setBandwidth(oldBandwidth);
        radio.setCodingRate(oldcoderate);
}

/**
 * Sets up the LoRa radio module with the specified parameters. Will call after reconfiguring the parameters
 *
 * @param None
 *
 * @return None
 *
 * @throws None
 */
void setup_lora(){
        radio.setSpreadingFactor(Spreadf);
        radio.setOutputPower(OutputPower);
        radio.setBandwidth(Bandwidth);
        radio.setCodingRate(codeRate);
}

void setup()
{
    initBoard();
    // When the power is turned on, a delay is required.
    delay(1500);

    randomSeed(analogRead(0));  // Seed the random number generator
    generateRandomInterval();
    generateRandomInterval_health();

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

    wifiConnect(); //connect to WIFI 
    client.setCallback(mqttCallback);
    connectToMQTT(); // Iniitate MQTT connection


}

void loop(){

    unsigned long currentMillis = millis(); // for schaduler

    if (EarthQuack == true){
        EarthQuack = false;
        startTransmission();
    }

    if ((scheduled == true) && (currentMillis - previousMillis >= interval)) { // if scheduled and time is reached
        setup_old_lora();
        startReconfigTransmission();
        scheduled = false;
        retxSch = true;

    }

    if ((scheduled_health == true) && (currentMillis - previousMillis >= interval_health)) { // if scheduled and time is reached
        startHealthTransmission();
        scheduled_health = false;
    }


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

            // if retransmission is done, setup the lora again to original parameters
            if (retxSch == true) { 
                retxSch = false;
                setup_lora();
            }

            // clean up after transmission is finished
            // this will ensure transmitter is disabled,
            // RF switch is powered down etc.
            
            radio.finishTransmit();

            delay(100);
            #ifdef HAS_DISPLAY
            if (u8g2) {
                char buf[256];
                u8g2->clearBuffer();
                u8g2->drawStr(0, 12, "Transmitting: OK!");
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
            // if first 2 chars in str String is ##, call dummy() function
            
            Serial.println(F("Received packet!"));
            numberofpackets++;
            Serial.println(str);
            int64_t rssi = radio.getRSSI(); 
            int64_t snr = radio.getSNR();
            int64_t FrqError = radio.getFrequencyError();
            String temp = str+","+String(numberofpackets)+","+String(rssi)+","+String(snr)+","+String(FrqError);
            sendData(temp);
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
 client.loop();
 wifiReconnect();

  if (!client.connected()) {
    mqttReconnect();
    }

}