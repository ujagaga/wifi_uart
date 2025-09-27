#include <ESP8266WebServer.h>
#include <EEPROM.h>
#include "config.h"

ESP8266WebServer* webServer = nullptr;
long currentBaud = 115200;

String htmlPage() {
  String page = F(
    "<!DOCTYPE html><html><head>"
    "<meta name='viewport' content='width=device-width, initial-scale=1'>"
    "<title>UART Baud Rate</title>"
    "<style>"
      "body{font-family:Arial,Helvetica,sans-serif;text-align:center;"
      "background:#f4f4f4;margin:0;padding:20px;}"
      "h2{font-size:2em;margin-bottom:20px;color:#333;}"
      "p{font-size:1.5em;margin:20px 0;color:#555;}"
      "form{margin-top:30px;}"
      "input[type=text]{font-size:1.5em;padding:10px;width:80%;"
      "max-width:300px;border:2px solid #333;border-radius:8px;}"
      "input[type=submit]{font-size:1.5em;padding:12px 24px;"
      "margin-top:20px;background:#4CAF50;color:#fff;border:none;"
      "border-radius:8px;cursor:pointer;}"
      "input[type=submit]:hover{background:#45a049;}"
    "</style>"
    "</head><body>"
    "<h2>ESP8266 UART Baud Rate</h2>"
  );
  page += "<form action=\"/set?t=" + String(millis()) + "\" method=\"GET\">";
  page += "Baud rate:<br>";
  page += "<input type=\"text\" name=\"baud\" value=\"" + String(currentBaud) + "\"><br>";
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

      // Respond with HTML that auto-redirects with mobile-friendly style
      String html = "<!DOCTYPE html><html><head>";
      html += "<meta name='viewport' content='width=device-width, initial-scale=1'>";
      html += "<meta http-equiv='refresh' content='2;url=/' />";
      html += "<title>Baud Rate Changed</title>";
      html += "<style>"
              "body{font-family:Arial,Helvetica,sans-serif;text-align:center;"
              "background:#f4f4f4;margin:0;padding:40px;}"
              "h3{font-size:2em;color:#333;}"
              "p{font-size:1.5em;color:#555;}"
              "a{font-size:1.2em;color:#2196F3;text-decoration:none;}"
              "a:hover{text-decoration:underline;}"
              "</style>";
      html += "</head><body>";
      html += "<h3>Baud rate changed to " + String(currentBaud) + "</h3>";
      html += "<p>Redirecting back to main page...</p>";
      html += "<a href='/'>Click here if not redirected</a>";
      html += "</body></html>";

      webServer->send(200, "text/html", html);
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
