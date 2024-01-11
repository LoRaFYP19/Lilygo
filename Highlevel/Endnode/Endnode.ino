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

Adafruit_SSD1306 display(128, 64, &Wire, OLED_RST);

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

  // Set LoRa parameters (optional)
  LoRa.setSpreadingFactor(SF);
  LoRa.setCodingRate4(CR);
  LoRa.setSignalBandwidth(BW);  // Set bandwidth if needed
  LoRa.setPreambleLength(8);      // Set preamble length if needed
}

void loop() {
  // Send a message
  String message = "Hello LoRa World!";
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
  display.display();

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

  delay(5000);  // Wait 5 seconds
}