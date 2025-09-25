#include <ESP8266WebServer.h>
#include <EEPROM.h>
#include "config.h"

ESP8266WebServer* webServer = nullptr;
long currentBaud = 115200;

String htmlPage() {
  String page = "<!DOCTYPE html><html><head><title>UART Baud Rate</title></head><body>";
  page += "<h2>ESP8266 UART Baud Rate</h2>";
  page += "<p>Current baud rate: <b>" + String(currentBaud) + "</b></p>";
  page += "<form action=\"/set?t=" + String(millis()) + "\" method=\"GET\">";
  page += "New baud rate: <input type=\"text\" name=\"baud\" value=\"" + String(currentBaud) + "\">";
  page += "<input type=\"submit\" value=\"Set\">";
  page += "</form>";
  page += "</body></html>";
  return page;
}

static void handleRoot() {
  webServer->send(200, "text/html", htmlPage());
}

static void handleSetBaud() {
  if (webServer->hasArg("baud")) {
    String saveBaud = webServer->arg("baud");
    long newBaud = saveBaud.toInt();

    if (newBaud >= 1200 && newBaud <= 921600) {
      currentBaud = newBaud;

      // Save to EEPROM
      EEPROM.begin(EEPROM_SIZE);
      for (uint16_t i = 0; i < saveBaud.length(); i++)
        EEPROM.write(i, saveBaud[i]);
      EEPROM.write(saveBaud.length(), 0); // terminator
      EEPROM.commit();

      Serial.flush();
      Serial.begin(currentBaud);
      webServer->send(200, "text/html",
        "<html><body><h3>Baud rate changed to " + String(currentBaud) + "</h3>"
        "<a href=\"/\">Back</a></body></html>");
      return;
    }
  }
  webServer->send(400, "text/plain", "Invalid baud rate");
}

static void handleGetBaud() {
   webServer->send(200, "text/html", String(currentBaud));
}

static void showNotFound() {
  webServer->send(404, "text/html",
    "<html><body><h1>404 Not Found</h1></body></html>");
}

void HTTP_SERVER_process(void) {
  webServer->handleClient();
}

void HTTP_SERVER_init(void) {
  webServer = new ESP8266WebServer(80);

  EEPROM.begin(EEPROM_SIZE);
  String stored;
  char c;
  for (int i=0; i<EEPROM_SIZE && (c = EEPROM.read(i)); i++)
      stored += c;
  if (stored.length()) {
      currentBaud = stored.toInt();
      Serial.begin(currentBaud);
  }

  webServer->on("/", handleRoot);
  webServer->on("/get", handleGetBaud);
  webServer->on("/set", handleSetBaud);
  webServer->onNotFound(showNotFound);

  webServer->begin();
}
