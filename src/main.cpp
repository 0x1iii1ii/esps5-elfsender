#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <LittleFS.h>
#include <uzlib.h>
#include "DNSServer.h"

ESP8266WebServer server(80);

// Function to send a chunk via HTTP with chunked encoding
void sendChunk(WiFiClient& client, const uint8_t* data, size_t len) {
    char sizeStr[10];
    sprintf(sizeStr, "%X\r\n", len);
    client.print(sizeStr);
    client.write(data, len);
    client.print("\r\n");
}

// Function to send the payload by decompressing and transmitting it
bool sendPayload(String payload) {
    // Set default IP and port
    String ip = "10.1.1.100";
    int port = 9021;

    // Construct file path using the payload argument
    String filePath = "/data/" + payload + ".elf.gz";
    File file = LittleFS.open(filePath, "r");
    if (!file) {
        Serial.println("Failed to open file: " + filePath);
        return false;
    }
    Serial.println("Sending payload: " + payload);

    // Establish connection to the target server
    WiFiClient client;
    if (!client.connect(ip, port)) {
        Serial.println("Connection to target server failed for payload: " + payload);
        file.close();
        return false;
    }

    // Send HTTP POST headers with chunked encoding
    client.print("POST / HTTP/1.1\r\n");
    client.print("Host: ");
    client.print(ip);
    client.print("\r\n");
    client.print("Transfer-Encoding: chunked\r\n");
    client.print("Content-Type: application/octet-stream\r\n");
    client.print("\r\n");

    // Initialize uzlib decompressor
    struct uzlib_uncomp d;
    memset(&d, 0, sizeof(d));

    // Read initial bytes for gzip header
    uint8_t header[10];
    size_t headerLen = file.readBytes((char*)header, sizeof(header));
    d.source = header;
    d.source_limit = header + headerLen;

    int res = uzlib_gzip_parse_header(&d);
    if (res != TINF_OK) {
        Serial.println("Failed to parse gzip header for payload: " + payload);
        client.stop();
        file.close();
        return false;
    }

    uzlib_uncompress_init(&d, NULL, 0); // No dictionary

    // Buffers for compression and decompression
    uint8_t inBuffer[256];
    uint8_t outBuffer[256];
    d.dest = outBuffer;
    d.dest_limit = outBuffer + sizeof(outBuffer);

    // Decompress and send in chunks
    while (file.available() || d.source < d.source_limit) {
        if (d.source >= d.source_limit && file.available()) {
            size_t bytesRead = file.readBytes((char*)inBuffer, sizeof(inBuffer));
            d.source = inBuffer;
            d.source_limit = inBuffer + bytesRead;
        }

        res = uzlib_uncompress(&d);
        if (res != TINF_OK && res != TINF_DONE) {
            Serial.println("Decompression error for payload: " + payload);
            client.stop();
            file.close();
            return false;
        }

        if (d.dest > outBuffer || res == TINF_DONE) {
            size_t toSend = d.dest - outBuffer;
            sendChunk(client, outBuffer, toSend);
            d.dest = outBuffer;
        }

        if (res == TINF_DONE) break;
    }

    // Send the final chunk to indicate end of data
    client.print("0\r\n\r\n");

    // Wait for response (optional, discarded here)
    while (client.connected()) {
        if (client.available()) {
            client.readStringUntil('\n');
        }
    }

    client.stop();
    file.close();
    return true;
}

// Handler for the root page
void handleRoot() {
    String html = R"=====(
<!DOCTYPE html>
<html lang="en">
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>ESPS5 Payload Sender</title>
    <style>
        body {
            font-family: Arial, sans-serif;
            background: black; /* Transparent background */
            color: white;
            margin: 0;
            padding: 0;
            display: flex;
            align-items: center;
            justify-content: center;
            height: 100vh;
        }
        .container {
            display: flex;
            flex-direction: column;
            align-items: center;
            justify-content: center;
            width: 100%;
        }
        .title {
            text-align: center;
            font-size: 32px;
            margin-bottom: 30px;
            font-weight: bold;
        }
        .main {
            display: grid;
            grid-template-columns: repeat(2, 1fr);
            gap: 20px;
            justify-content: center;
            align-items: center;
        }
        .payload {
            background-color: rgba(44, 62, 80, 0.8); /* Dark blue with transparency */
            padding: 20px;
            border-radius: 8px;
            width: 500px; /* Fixed width */
            height: 80px; /* Fixed height */
            display: flex;
            flex-direction: column;
            justify-content: center;
            align-items: center;
            text-align: center;
            cursor: pointer;
            transition: transform 0.2s ease, box-shadow 0.2s ease;
            box-shadow: 0 4px 8px rgba(0, 0, 0, 0.2);
        }
        .payload:hover {
            transform: scale(1.02);
            box-shadow: 0 0 0 2px white;
        }
        h2 {
            font-size: 18px;
            margin: 0 0 10px 0;
            font-weight: bold;
        }
        p {
            font-size: 14px;
            margin: 0;
        }
        .version {
            color: #aaa;
        }
        .footer {
            text-align: center;
            font-size: 12px;
            color: #aaa;
            margin-top: 20px;
        }
    </style>
</head>
<body>
    <div class="container">
        <div class="title">ESPS5 Payload Sender</div>
        <div class="main">
            <div class="payload" onclick="sendPayload('etahen')">
                <h2>etaHEN 1.1b By LM</h2>
                <p>FPKG enabler for FW 4.03. <span class="version">v1 beta</span></p>
            </div>
            <div class="payload" onclick="sendPayload('kstuff')">
                <h2>K-Stuff</h2>
                <p>FW 4.03-4.51 ONLY. <span class="version">v1.2</span></p>
            </div>
            <div class="payload" onclick="sendPayload('payload1')">
                <h2>Payload 1</h2>
                <p>Additional payload. <span class="version">v1.0</span></p>
            </div>
            <div class="payload" onclick="sendPayload('ps5elfloader')">
                <h2>PS5 Payload ELF Loader</h2>
                <p>Persistent network ELF loader. <span class="version">v0.5</span></p>
            </div>
            <div class="payload" onclick="sendPayload('ftps5')">
                <h2>FTPS5 (Persistent)</h2>
                <p>FTP SERVER</p>
            </div>
            <div class="payload" onclick="sendPayload('payload2')">
                <h2>Payload 2</h2>
                <p>Additional payload. <span class="version">v1.0</span></p>
            </div>
        </div>
    </div>
    <script>
        function sendPayload(payload) {
            fetch(`/send?payload=${payload}`)
                .then(response => response.text())
                .then(text => alert(text))
                .catch(error => alert('Error: ' + error));
        }
    </script>
</body>
</html>
    )=====";
    server.send(200, "text/html", html);
}

// Handler for the send action
void handleSend() {
    String payload = server.arg("payload");

    if (payload.length() == 0) {
        server.send(400, "text/plain", "Missing payload parameter");
        return;
    }

    bool success = sendPayload(payload);
    if (success) {
        server.send(200, "text/plain", "Payload sent successfully");
    } else {
        server.send(500, "text/plain", "Failed to send payload");
    }
}

void setup() {
    Serial.begin(115200);

    // Set up Access Point
    //   const char* ssid = "ESPS5_ELF_SENDER"; 
    const char* ssid = "ESP_SENDER";
    const char* password = "12345678";
    IPAddress local_IP(10, 1, 1, 1);      // Static IP for ESP8266
    IPAddress gateway(10, 1, 1, 1);       // Gateway (same as local IP for AP)
    IPAddress subnet(255, 255, 255, 0);   // Subnet mask

    WiFi.softAPConfig(local_IP, gateway, subnet);
    WiFi.softAP(ssid, password);

    Serial.println("Access Point started");
    Serial.print("IP Address: ");
    Serial.println(WiFi.softAPIP());      // Should print 10.1.1.1

    // Initialize LittleFS
    if (!LittleFS.begin()) {
        Serial.println("Failed to mount LittleFS");
        return;
    }

    // Set up web server routes
    server.on("/index.html", handleRoot);
    server.on("/send", handleSend);
    server.begin();
    Serial.println("Web server started");
}

void loop() {
    server.handleClient();
}