/* 
 Created 2022
  originally by Peter Chodyra
  heavily modified to meet case by Poespas
 */

#include <ArduinoJson.h>
#include "WiFiManager.h"          //https://github.com/tzapu/WiFiManager

#include <Stepper.h>
#include <ESP_EEPROM.h>
#include <ESP8266WiFi.h>          //https://github.com/esp8266/Arduino

//needed for library
#include <DNSServer.h>
#include <ESP8266WebServer.h>

//#define DEBUG

//IMPORTANT Connect ULN2003 PINs to NodeMCU D pins in order PIN1 -> D1, PIN2 -> D5, PIN3 -> PIN1,  PIN4 -> D5
#define PIN1 D2 
#define PIN2 D3
#define PIN3 D6
#define PIN4 D7

// ------------------------------------------------------------------------
// ############################# Global Definitions #########################
// ------------------------------------------------------------------------
int currentPosition = 0;
int targetPosition = 0;

// ------------------------------------------------------------------------
// ############################# EEPROM Definitions #########################
// ------------------------------------------------------------------------
int addr = 0; //EEPROM Address memory space for the current_position of the blinds
struct EEPROMStruct {
  int currentPosition;
} eepromVar;

//Web server setup
ESP8266WebServer server(80);   //Web server object. Will be listening in port 80 (default for HTTP)

// ------------------------------------------------------------------------
// ############################# jsonOutput() #############################
// ------------------------------------------------------------------------

String jsonOutput(String jName,int jValue){  
  String output;
  
  StaticJsonBuffer<200> jsonBuffer;
  JsonObject& root = jsonBuffer.createObject();
  root[jName] = jValue;
  root.printTo(output);
  return output;
}

// ------------------------------------------------------------------------
// ############################# Web callbacks #############################
// ------------------------------------------------------------------------
void onWebNotFound() {
  String message = "File Not Found\n\n";
  message += "URI: ";
  message += server.uri();
  message += "\nMethod: ";
  message += (server.method() == HTTP_GET) ? "GET" : "POST";
  message += "\nArguments: ";
  message += server.args();
  message += "\n";
  for (uint8_t i = 0; i < server.args(); i++) {
    message += " " + server.argName(i) + ": " + server.arg(i) + "\n";
  }
  server.send(404, "text/plain", message);
}

/**
  * Performs move, sets targetPosition int
  **/
void onWebGetMove() {
  String message = "";
  int moveSteps = 0; //Ths value can either be positive -> clockwise rotation or negative -> counetrclockwise rotation
  
  if (server.arg("move") == ""){     //Parameter not found
    message = "Move Argument not found";
  }else{     //Parameter found
    message = "Manual input trigered Open Argument = ";
    message += server.arg("move");     //Gets the value of the query parameter
    //Move the blinds
    targetPosition = atoi(server.arg("move").c_str());
  }
}

// ------------------------------------------------------------------------
// ############################# Business Logic #############################
// ------------------------------------------------------------------------

void handlePositionTick() {
  if (currentPosition > targetPosition) {
    moveOneTickClockwise();
    currentPosition--;
  }
  else if (currentPosition < targetPosition) {
    moveOneTickCounterClockwise();
    currentPosition++;
  }
}

void moveOneTickClockwise() {
  Serial.println("Moving clockwise..");
  digitalWrite(PIN1, HIGH);
  delay(2);
  digitalWrite(PIN1, LOW);
  digitalWrite(PIN2, HIGH);
  delay(2);
  digitalWrite(PIN2, LOW);
  digitalWrite(PIN3, HIGH);
  delay(2);
  digitalWrite(PIN3, LOW);
  digitalWrite(PIN4, HIGH);
  delay(2);
  digitalWrite(PIN4, LOW);
}

void moveOneTickCounterClockwise() {
  Serial.println("Moving counterclockwise..");
  digitalWrite(PIN4, HIGH);
  delay(2);
  digitalWrite(PIN4, LOW);
  digitalWrite(PIN3, HIGH);
  delay(2);
  digitalWrite(PIN3, LOW);
  digitalWrite(PIN2, HIGH);
  delay(2);
  digitalWrite(PIN2, LOW);
  digitalWrite(PIN1, HIGH);
  delay(2);
  digitalWrite(PIN1, LOW);
}
// ------------------------------------------------------------------------
// ############################# setup() #############################
// ------------------------------------------------------------------------

void setup() {
  pinMode(D2, OUTPUT);
  pinMode(D3, OUTPUT);
  pinMode(D6, OUTPUT);
  pinMode(D7, OUTPUT);

  // initialize the serial port:
  Serial.begin(115200);
  Serial.println();

  //Setup WiFiManager
  WiFiManager wfManager;
  //Switchoff debug mode
  wfManager.setDebugOutput(false);

  //exit after config instead of connecting
  wfManager.setBreakAfterConfig(true);
  
  if (!wfManager.autoConnect()) {
    Serial.println("failed to connect, resetting and attempting to reconnect");
    delay(3000);
    ESP.reset();
    delay(5000);
  }

  //if you get here you have connected to the WiFi
  Serial.println("connected... ");

  server.on("/", [](){
    String message = "<ul>";
    message += "<li><a href=\"/api/move?move=2\">/api/move?move=2</a> - Move motor to target position</li>";
    message += "<li><a href=\"/api/position\">/api/position</a> - Get current position</li>";
    message += "</ul>";
    server.send(200, "text/html", message);
  });

  server.on("/api/move", []() {
    onWebGetMove();
    server.send(200, "application/json", jsonOutput("success", 1));
  });

  server.on("/api/position", []() {
    server.send(200, "application/json", jsonOutput("position", currentPosition));
  });
  
  server.onNotFound(onWebNotFound);

  server.begin();
  Serial.println("HTTP server started");
  Serial.print("local ip: ");
  Serial.println(WiFi.localIP());
  Serial.print("MAC: ");
  Serial.println(WiFi.macAddress());
  
  EEPROM.begin(sizeof(EEPROMStruct));

  EEPROM.get(addr, eepromVar); //read the eeprom structure

  //Power off the motor
  digitalWrite(PIN1,LOW);
  digitalWrite(PIN2,LOW);
  digitalWrite(PIN3,LOW);
  digitalWrite(PIN4,LOW);
}

// ------------------------------------------------------------------------
// ############################# loop() #############################
// ------------------------------------------------------------------------
void loop() {

  if (targetPosition != currentPosition) {
    handlePositionTick();
  }

  //Handle server requests
  server.handleClient();

}
