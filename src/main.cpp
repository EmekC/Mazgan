#include <Arduino.h>
// IR
#include <IRremoteESP8266.h>
#include <IRsend.h>
// Electra
#include <ir_Coolix.h>
// DHT
#include <DHT.h>
#include <DHT_U.h>
#include <Adafruit_Sensor.h>
// ColorConverter
#include <ColorConverter.h>
#include <Math.h>
// ============= BLYNK =============

#define BLYNK_TEMPLATE_ID           "TMPLx38mw3Vd"
#define BLYNK_DEVICE_NAME           "AC"
#define BLYNK_AUTH_TOKEN            "QJ_O6XQv4LF6r1Hu5Jbng6hHXls6mmRn"

// Comment this out to disable prints and save space
#define BLYNK_PRINT Serial

#include <ESP8266WiFi.h>
#include <BlynkSimpleEsp8266.h>

char auth[] = BLYNK_AUTH_TOKEN;

// Your WiFi credentials.
// Set password to "" for open networks.
char ssid[] = "Emeks_Dungeon";
char pass[] = "Emek410262";

BlynkTimer timer;

// ============= Constants =============
//IR transmitter
#define IR_GPIO 4
//DHT
#define TEMP_GPIO 10
#define DHTTYPE DHT22
DHT dht(TEMP_GPIO, DHTTYPE);
//Blynk
#define V_HUMIDITY V0
#define V_TEMP_SENSOR V1
#define V_HIC V2
#define V_TERMINAL V3
#define V_TEMP V5
#define V_INC V6
#define V_DEC V7
#define V_SWITCH V8
#define V_FAN V9
#define V_FAN_BTN V10
#define AC_MODE V11
#define MIN_TEMP 16
#define MAX_TEMP 31
#define UPDATE_INTERVAL 5000L
// enums for Blynk Virtual Button
#define FAN_LOW 0
#define FAN_MED 1
#define FAN_HIGH 2
#define FAN_AUTO 3
// colors
u_int8_t temp_r;
u_int8_t temp_g;
u_int8_t temp_b;

// LED
#define WIFI_LED_GPIO 14
// ============= Variables =============

IRCoolixAC ac(IR_GPIO);

int temp = MIN_TEMP;
int fan = kCoolixFanMed;
uint8_t mode = kCoolixCool;

#define BUTTON_COOLDOWN 1000L

// ============= funcs =============
ColorConverter clrCnvrt;

void updateColorProperty(int celcius) {
  // Linear equation where (16, 0) and (31, 1) are met.
  double percentage = 0.0666*celcius - 1.06666;

  TempColor color_blue(255, 32, 82, 235);
  TempColor color_red(255, 255, 74, 102);

  TempColor propertyColor = clrCnvrt.GradientPick(percentage, color_blue, color_red);

  char hexArray[6] = { 0 };
	sprintf(hexArray, "%02X%02X%02X", propertyColor.R, propertyColor.G, propertyColor.B);
	String hexColor = hexArray;

  Blynk.setProperty(V_TEMP, "color", hexColor);
}

void terminalBlynk(String str) {
  Blynk.virtualWrite(V_TERMINAL, str);
}

// flash the leds for 100 ms to indicate something has happened.
void flashLED() {
  digitalWrite(WIFI_LED_GPIO, LOW);
  delay(100);
  digitalWrite(WIFI_LED_GPIO, HIGH);
}

void printState() {
  // Display the settings.
  Serial.println("A/C remote is in the following state:");
  Serial.printf("  %s\n", ac.toString().c_str());
}

void readTemp() {

  float temp = dht.readTemperature();
  float humid = dht.readHumidity();
  float hic = dht.computeHeatIndex(temp, humid, false);

  // Write data to blynk
  Blynk.virtualWrite(V_TEMP_SENSOR, temp);
  Blynk.virtualWrite(V_HUMIDITY, humid);
  Blynk.virtualWrite(V_HIC, hic);  

  // flash led
  flashLED();
}

void setupAC() {
  ac.begin();
  ac.setTemp(temp);
  ac.setMode(kCoolixCool);
  ac.setSensorTemp(temp);
  ac.off();
}

void turnOnAC() {
  flashLED();

  ac.begin();
  ac.setTemp(temp);
  ac.setMode(mode);
  ac.setFan(fan);
  ac.clearSensorTemp();
  ac.on();
  ac.send();
  printState();
}

void turnOffAC() {
  flashLED();
  // ac.setPowerToggle(false);
  ac.begin();
  ac.off();
  ac.send();
  printState();
}

void setupBlynk() {
  Blynk.begin(auth, ssid, pass);
  // setup interval functions
  timer.setInterval(UPDATE_INTERVAL, readTemp);
}

void setupGPIO() {
  pinMode(WIFI_LED_GPIO, OUTPUT);
  digitalWrite(WIFI_LED_GPIO, LOW);
  pinMode(IR_GPIO, OUTPUT);
}

void sendFanUpdate(int fan) {
  ac.begin();
  switch (fan) {
  case FAN_LOW:
    ac.setFan(kCoolixFanMin);
    break;
  
  case FAN_MED:
    ac.setFan(kCoolixFanMed);
    break;

  case FAN_HIGH:
    ac.setFan(kCoolixFanMax);
    break;

  case FAN_AUTO:
    ac.setFan(kCoolixFanAuto0);
    break;

  default:
    ac.setFan(kCoolixFanMed);
    break;
  }
  ac.send();
}

// Update temeprature value.
void sendTempUpdate(int value) {
  ac.begin();
  ac.setTemp(value);
  ac.send();
} 

// Update Air Conditioner mode.
void sendACMode(uint8_t mode) {
  ac.begin();
  ac.setMode(mode);
  ac.send();
}
// ============= READ BUTTONS FROM BLYNK =============

BLYNK_CONNECTED() {
  // Blynk.syncVirtual(V_TEMP);
  // Request server to re-send latest values for all pins
  Serial.println("==============================\nSyncing all Blynk connections.\n==============================\n");
  Blynk.syncAll();
  digitalWrite(WIFI_LED_GPIO, HIGH);
  
}

// read temp
BLYNK_WRITE(V_TEMP) {
  String value = param.asString();
  temp = value.toInt();

  Serial.print("Receiving temperature from Blynk...\nupdating ac temperature to Blynk value.");
  ac.setTemp(temp);

  Serial.println("Read temperature param: " + value);
  Serial.print("\n");

  updateColorProperty(temp);
}
// temp increase
bool pkuda = false;
unsigned long sinceLastPress;
BLYNK_WRITE(V_INC) {
  // get value as int
  String value = param.asString();

  if (value == "1") {  
    // inc temp if read temperature from server
    if (temp < MAX_TEMP) {
      // increate temp and update in server
      temp = temp + 1;
      // update temp widget color
      updateColorProperty(temp);
      Blynk.virtualWrite(V_TEMP, temp);
      // send temperature update to AC
      // sendTempUpdate(temp);
      pkuda = true;
      sinceLastPress = millis();
    }
  }
}
// temp dec
BLYNK_WRITE(V_DEC) {
  // get value as int
  String value = param.asString();

  if (value == "1") {  
    // inc temp if read temperature from server
    if (temp > MIN_TEMP) {
      // increate temp and update in server
      temp = temp - 1;
      // update temp widget color
      updateColorProperty(temp);
      Blynk.virtualWrite(V_TEMP, temp);
      // send temperature update to AC
      // sendTempUpdate(temp);
      pkuda = true;
      sinceLastPress = millis();
    }
  }

}

// toggle
BLYNK_WRITE(V_SWITCH) {

  uint8_t value = param.asInt();
  
  if (value) {
    if (!ac.getPower())
      turnOnAC();
  } else {
    if (ac.getPower())
      turnOffAC();
  }

}

// cycle fan
int pkuda_fan = false;
BLYNK_WRITE(V_FAN_BTN) {
  int value = param.asInt();
  if (value == 0) { return; }

  fan++; // increment fan
  if (fan > FAN_AUTO) {
    fan = FAN_LOW;
  }
  Blynk.virtualWrite(V_FAN, fan);
  Serial.printf("Cycling fan to: %d\n", fan);  

  // begin ac IR command.
  pkuda_fan = true;
  sinceLastPress = millis();
  // end command

}

// sync fan with Blynk
// BLYNK_WRITE(V_FAN) {
//   uint8_t value = param.asInt();

//   Serial.println("fan is currently: " + value);

//   fan = value;
// }


// Cool/Heat mode switch
BLYNK_WRITE(AC_MODE) {
  int value = param.asInt();

  // if AC is off. dont do shit
  if (!ac.getPower()) {return;}

  if (value) {
    // Cool
    mode = kCoolixCool;
    sendACMode(mode);

  } else {
    // Heat
    mode = kCoolixHeat;
    sendACMode(mode);
  }

  printState();
}

void setup() {
  Serial.begin(115200);

  // AC setup
  setupAC();
  Serial.print("Default state:");
  printState();

  setupGPIO();

  delay(200);

  // dht setup
  dht.begin();
  Serial.println("Device Started");
  Serial.println("-------------------------------------");
  Serial.println("Running DHT!");
  Serial.println("-------------------------------------");

  // BLynk
  setupBlynk();
}



void loop() {
  Blynk.run();
  timer.run();

  // if AC is turned on continue...
  if (ac.getPower()) {
    // Wait BUTTON_COOLDOWN time since last button click until sending order.
    if ((millis() - sinceLastPress) >= BUTTON_COOLDOWN && pkuda) {
      sendTempUpdate(temp);
      pkuda = false;
    }

    if (pkuda_fan && (millis() - sinceLastPress) >= BUTTON_COOLDOWN) {
      sendFanUpdate(fan);
      pkuda_fan = false;
    }
  }
}