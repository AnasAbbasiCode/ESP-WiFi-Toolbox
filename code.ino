#include <WiFi.h>
#include <WebServer.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64

Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);

const char* ssid = "techiesms";
const char* password = "";

WebServer server(80);
String messages = "";
String lastDisplayedMessages = "";

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

    h1 {
      color: #00ff00;
      text-shadow: 0 0 5px #00ff00;
    }

    h3 {
      color: #33ff33;
    }

    input[type="text"] {
      width: 80%;
      padding: 10px;
      margin: 10px 0;
      background-color: #0f0f0f;
      color: #00ff00;
      border: 1px solid #00ff00;
      border-radius: 5px;
      font-size: 16px;
    }

    input[type="submit"] {
      padding: 10px 20px;
      background-color: #0f0f0f;
      color: #00ff00;
      border: 1px solid #00ff00;
      border-radius: 5px;
      font-size: 16px;
      cursor: pointer;
      box-shadow: 0 0 10px #00ff00;
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
  server.send(200, "text/html", buffer);
}

void handleSend() {
  if (server.hasArg("message")) {
    String message = server.arg("message");

    // If message is "reset", clear all stored messages
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
  server.send(200, "text/html", messages);
}

void setup() {
  Serial.begin(115200);
  WiFi.softAP(ssid, password);
  Serial.println(WiFi.softAPIP());

  server.on("/", handleRoot);
  server.on("/send", HTTP_POST, handleSend);
  server.on("/messages", HTTP_GET, handleMessages);
  server.begin();

  if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
    Serial.println(F("SSD1306 allocation failed"));
    for (;;);
  }
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(WHITE);
}

void loop() {
  server.handleClient();

  if (messages != lastDisplayedMessages) {
    display.clearDisplay();
    display.setCursor(0, 0);

    // Limit number of lines to fit screen
    int maxLines = 6;
    int start = messages.lastIndexOf("<br>", messages.length() - 1);
    int lines = 0;
    String toDisplay = messages;

    while (start != -1 && lines < maxLines) {
      start = messages.lastIndexOf("<br>", start - 1);
      lines++;
    }
    if (start != -1) {
      toDisplay = messages.substring(start + 4); // skip "<br>"
    }

    toDisplay.replace("<br>", "\n"); // format for OLED
    display.println(toDisplay);
    display.display();
    lastDisplayedMessages = messages;
  }
}
