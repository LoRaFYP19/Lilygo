#include <SPI.h>
#include <LoRa.h>

#include <Wire.h>  // Added for OLED
#include <Adafruit_GFX.h>  // Added for OLED
#include <Adafruit_SSD1306.h>  // Added for OLED

// Define LoRa pins
#define SCK 5
#define MISO 19
#define MOSI 27
#define SS 18
#define RST 14
#define DIO0 26

// OLED pins 
#define OLED_SDA 4
#define OLED_SCL 15
#define OLED_RST 16  // Check your board's pinout for reset pin

// Change these parameters as needed
#define BAND 923E6  // Frequency band (433E6, 868E6, 915E6, 923E6)
#define SF 7       // Spreading Factor (7-12)
#define CR 4       // Coding Rate (4-8)
#define BW 125E3   // Signal Bandwidth (125E3, 250E3, 500E3)

int spreadingfactor = 7;


Adafruit_SSD1306 display(128, 64, &Wire, OLED_RST);

void onCadDone(boolean signalDetected) { // When A CAD scan is done, this function is called
  // detect preamble
  if (signalDetected) {
    Serial.println("Signal detected");
    // put the radio into continuous receive mode
    LoRa.receive();
  } else {
    // try next activity dectection
    LoRa.channelActivityDetection();
  }
}

void onReceive(int packetSize) { // If reception trigger was detected, this function is called
  // received a packet
  Serial.print("Received packet '");
  // read packet
  for (int i = 0; i < packetSize; i++) {
    Serial.print((char)LoRa.read());
  }
  // print RSSI of packet
  Serial.print("' with RSSI ");
  Serial.println(LoRa.packetRssi());
  // put the radio into CAD mode
  LoRa.channelActivityDetection();
}


void setup() {
  Serial.begin(115200);
  while (!Serial);

  SPI.begin(SCK, MISO, MOSI, SS);
  LoRa.setPins(SS, RST, DIO0);

  Wire.begin(OLED_SDA, OLED_SCL);
  if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
    Serial.println("SSD1306 allocation failed");
    for (;;);
  }
  display.clearDisplay();
  display.display();

  if (!LoRa.begin(BAND)) {
    Serial.println("Starting LoRa failed!");
    while (1);
  }

  // Set LoRa parameters (optional innitialization)
  LoRa.setSpreadingFactor(SF);
  LoRa.setCodingRate4(CR);
  LoRa.setSignalBandwidth(BW);  // Set bandwidth if needed
  LoRa.setPreambleLength(8);      // Set preamble length if needed

  // register the channel activity dectection callback
  LoRa.onCadDone(onCadDone);
  // register the receive callback
  LoRa.onReceive(onReceive);
  // put the radio into CAD mode
  LoRa.channelActivityDetection();

}

void sendLoRaMessage(String message) {
  LoRa.beginPacket();
  LoRa.print(message);
  LoRa.endPacket();

  Serial.println("Message sent: " + message);
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(WHITE);
  display.setCursor(0, 0);
  display.println("Message sent:");
  display.println(message);
  display.display();   // Update the last packet time
}

void receiveLoRaMessage() {
  // Receive a message
  int packetSize = LoRa.parsePacket();
  if (packetSize) {
    String receivedMessage = "";
    while (LoRa.available()) {
      receivedMessage += (char)LoRa.read();
    }
    Serial.println("Message received: " + receivedMessage);
    display.clearDisplay();
    display.setTextSize(1);
    display.setTextColor(WHITE);
    display.setCursor(0, 0);
    display.println("Message received:");
    display.println(receivedMessage);
    display.display();
  }
}
void loop() {
    LoRa.setSpreadingFactor(spreadingfactor);
    if spreadingfactor < 12 {
        spreadingfactor++;
    }
    else {
        spreadingfactor = 7;
    }
      // register the channel activity dectection callback
    LoRa.onCadDone(onCadDone);
    // register the receive callback
    LoRa.onReceive(onReceive);
    // put the radio into CAD mode
    LoRa.channelActivityDetection();
    
    
}
