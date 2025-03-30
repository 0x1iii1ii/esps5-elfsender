#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <LittleFS.h>

// Forward declarations of handler functions
void handleRoot();
void handleFileTransfer();

ESP8266WebServer server(80);

// AP configuration
const char* apSSID = "ESP_ELF_SENDER";
const char* apPassword = "12345678";  // Must be at least 8 characters
IPAddress localIP(10, 1, 1, 1);
IPAddress gateway(10, 1, 1, 1);
IPAddress subnet(255, 255, 255, 0);

// Target configuration
const char* targetIP = "10.1.1.100";
const uint16_t targetPort = 9021;

// Constants
const char* VERSION = "1.2.3";
const uint16_t TIMEOUT_MS = 30000;  // Connection timeout

void setup() {
  Serial.begin(115200);
  delay(100);  // Brief delay for serial stability
  
  // Configure and start AP
  if (!WiFi.softAPConfig(localIP, gateway, subnet)) {
    Serial.println("AP Config failed");
    return;
  }
  
  if (!WiFi.softAP(apSSID, apPassword)) {
    Serial.println("AP failed to start");
    return;
  }

  Serial.println("\nAP started successfully");
  Serial.print("SSID: ");
  Serial.println(apSSID);
  Serial.print("IP address: ");
  Serial.println(WiFi.softAPIP());

  if (!LittleFS.begin()) {
    Serial.println("LittleFS mount failed");
    return;
  }

  // Register endpoints
  server.on("/index.html", HTTP_GET, handleRoot);
  server.on("/send-file", HTTP_GET, handleFileTransfer);
  server.onNotFound([]() {
    server.send(404, "text/plain", "Not Found");
  });
  
  server.begin();
  Serial.println("HTTP server started");
}

void handleRoot() {
  String html = R"rawliteral(
<!DOCTYPE html>
<html>
<head>
  <title>ESP8266 Payload Sender for PS5 6.XX, 7.XX</title>
  <meta name="viewport" content="width=device-width, initial-scale=1">
  <style>
    body {
      background: #1a1a1a;
      color: #e0e0e0;
      font-family: Arial, sans-serif;
      margin: 0;
      padding: 20px;
      min-height: 100vh;
      display: flex;
      justify-content: center;
      align-items: center;
    }
    .container {
      max-width: 1200px;
      width: 100%;
      display: grid;
      grid-template-columns: repeat(2, 1fr);
      gap: 20px;
    }
    .card {
      background: #2d2d2d;
      border-radius: 12px;
      padding: 2rem;
      cursor: pointer;
      transition: all 0.3s ease;
      border: 1px solid #404040;
      text-align: center;
    }
    .card:hover {
      background: #363636;
      transform: translateY(-2px);
      box-shadow: 0 4px 20px rgba(0,0,0,0.3);
      border-color: #fff;
    }
    .title {
      font-size: 1.8rem;
      font-weight: bold;
      margin-bottom: 1rem;
      color: #fff;
      text-align: center;
      grid-column: span 2;
    }
    .description {
      color: #fff;
      margin-bottom: 0.5rem;
    }
    .version {
      color: #aaa;
      font-size: 0.9rem;
    }
    .dev {
      color: #707070;
      font-size: 0.9rem;
    }
    .credits {
      font-size: 0.8rem;
      color: #808080;
      text-align: center;
      grid-column: span 2;
    }
    .status {
      margin-top: 1rem;
      color: #00ff00;
      display: none;
      text-align: center;
    }
  </style>
</head>
<body>
  <div class="container">
    <h1 class="title">ESP8266 Payload Sender for PS5 6.XX, 7.XX</h1>
    <!-- Card for first ELF file -->
    <div class="card" onclick="sendFile('byepervisor.elf', 'status1')">
      <div class="description">Byepervisor HEN v1.0 FW 1.XX-2.XX</div>
      <div class="version">FPKG enabler</div>
      <div class="dev">SpecterDev, ChendoChap, flatz, fail0verflow, Znullptr, kiwidog, sleirsgoevy, EchoStretch, LightningMods, BestPig, zecoxao</div>
      <div id="status1" class="status">Sending file...</div>
    </div>
    <!-- Card for second ELF file -->
    <div class="card" onclick="sendFile('kstuff.elf', 'status2')">
      <div class="description">ps5-kstuff FW 3.XX-6.XX </div>
      <div class="version">FPKG enabler</div>
      <div class="dev">sleirsgoevy, john-tornblom, EchoStretch, buzzer-re, BestPig, LightningMods, zecoxao</div>
      <div id="status2" class="status">Sending file...</div>
    </div>
    <div class="credits">
      Created by https://github.com/0x1iii1ii<br>
    </div>
  </div>
  <script>
    function sendFile(fileName, statusId) {
      const status = document.getElementById(statusId);
      status.style.display = 'block';
      status.textContent = 'Sending file...';
      status.style.color = '#00ff00';
      
      fetch('/send-file?file=' + fileName)
        .then(response => {
          if (!response.ok) throw new Error('Server error');
          return response.text();
        })
        .then(data => {
          status.textContent = 'File sent successfully!';
          setTimeout(() => status.style.display = 'none', 2000);
        })
        .catch(error => {
          status.textContent = 'Error: ' + error.message;
          status.style.color = '#ff0000';
          setTimeout(() => status.style.display = 'none', 3000);
        });
    }
  </script>
</body>
</html>
  )rawliteral";
  server.send(200, "text/html", html);
}

void handleFileTransfer() {
  // Read the "file" parameter from the query string
  String fileParam = server.arg("file");
  if (fileParam == "") {
    server.send(400, "text/plain", "File parameter missing");
    return;
  }
  // Construct file path assuming files are stored in LittleFS with a leading '/'
  String filePath = "/" + fileParam;
  
  if (!LittleFS.exists(filePath)) {
    server.send(500, "text/plain", "File not found on device: " + filePath);
    return;
  }
  
  File file = LittleFS.open(filePath, "r");
  if (!file) {
    server.send(500, "text/plain", "Failed to open file: " + filePath);
    return;
  }
  
  WiFiClient client;
  client.setTimeout(TIMEOUT_MS);
  if (!client.connect(targetIP, targetPort)) {
    file.close();
    server.send(500, "text/plain", "Failed to connect to target");
    return;
  }
  
  size_t fileSize = file.size();
  uint8_t buffer[1024];
  size_t sent = 0;
  
  while (sent < fileSize) {
    size_t chunkSize = file.read(buffer, sizeof(buffer));
    if (chunkSize == 0) break;
    if (client.write(buffer, chunkSize) != chunkSize) {
      file.close();
      client.stop();
      server.send(500, "text/plain", "Transmission error");
      return;
    }
    sent += chunkSize;
  }
  
  file.close();
  client.stop();
  server.send(200, "text/plain", "File transferred successfully (" + String(sent) + " bytes)");
}

void loop() {
  server.handleClient();
}
