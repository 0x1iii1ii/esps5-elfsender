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
<html manifest="cache.appcache">

<head>
    <title>PS5 Kernel Exploit (3.xx-4.xx)</title>
    <meta name="viewport" content="width=device-width, initial-scale=1">
    <link rel="stylesheet" href="main.css">

    <script defer src="int64.js"></script>
    <script defer src="rop.js"></script>
    <script defer src="exploit.js"></script>
    <script defer src="payload_map.js"></script>
    <script defer src="custom_host_stuff.js"></script>
    <script defer src="appcache_handler.js"></script>
    <script defer src="module/utils.js"></script>
    <script defer src="module/constants.js"></script>
    <script defer src="module/int64.js"></script>
    <script defer src="module/mem.js"></script>
    <script defer src="module/memtools.js"></script>
    <script defer src="module/rw.js"></script>
    <script defer src="webkit_psfree.js"></script>
    <script defer src="webkit_fontface.js"></script>
    <script>
        addEventListener('unhandledrejection', (event) => {
            const reason = event.reason;
            // We log the line and column numbers as well since some exceptions (like
            // SyntaxError) do not show it in the stack trace.
            // alert(
            //     `${reason}\n`
            //     + `${reason.sourceURL}:${reason.line}:${reason.column}\n`
            //     + `${reason.stack}`
            // );
            alert(reason);
            throw reason;
        });
        window.addEventListener('load', function () {
            onload_setup();
        }, false);
    </script>
</head>

<body>
    <svg style="display: none" version="2.0">
        <defs>
            <symbol id="delete-icon" viewBox="0 -960 960 960">
                <path
                    d="m376-300 104-104 104 104 56-56-104-104 104-104-56-56-104 104-104-104-56 56 104 104-104 104 56 56Zm-96 180q-33 0-56.5-23.5T200-200v-520h-40v-80h200v-40h240v40h200v80h-40v520q0 33-23.5 56.5T680-120H280Zm400-600H280v520h400v-520Zm-400 0v520-520Z" />
            </symbol>
            <symbol id="forward-arrow-icon" viewBox="0 -960 960 960" >
                <path d="M647-440H160v-80h487L423-744l57-56 320 320-320 320-57-56 224-224Z"/>
            </symbol>
            
        </defs>
    </svg>

    <div id="toast-container"></div>
    
    <div id="redirector-view"
        style="min-width: 100%; max-width: 100%; position: absolute; top: 0; left: -100%; min-height: 100%; max-height: 100%;">
        <div style="overflow-y: scroll; width: 100%;max-height: 100vh;min-height: 100vh;" id="redirector-view-inner">
            <h3 style="text-align: center;">Redirector</h3>
            <div style="padding: 25px;padding-top: 0; display: flex;">
                <input id="redirector-input" type="text" value="http://"
                    style="width: 100%; border-radius: 10px; padding: 15px; background-color: #223; color: #fff; border-color: #334; font-size: 20px;" />
                <a class="btn icon-btn" style="width: 100px;" tabindex="0" onclick="redirectorGo()">
                    <svg width="32px" height="32px" fill="#ddd">
                        <use href="#forward-arrow-icon" />
                    </svg>
                </a>
            </div>

            <div style="display: flex; flex-direction: row; max-width: 100%; padding-left: 50px; padding-right: 50px;">
                <div style="width: 60%; margin-right: 35px; " id="redirector-pinned"></div>
                <div style="width: 1px; background-color: #557; margin-top: 10px;"></div>
                <div style="width: 40%; margin-left: 35px; " id="redirector-history"></div>
            </div>
        </div>
    </div>

    <div id="center-view" style="position: absolute; top: 0; left: 0; width: 100%; min-height: 100%;">

        <div style="margin-top: 15px; position: absolute; width: 100%;">
            <h1 style="text-align: center;">PS5 Jailbreak 3.xx-4.xx</h1>
            <p id="current-fw" class="text-secondary"
                style="text-align: center; margin-bottom: 8px; padding-bottom: 0;">
            </p>

            <p id="listening-ip" class="text-secondary" style="text-align: center; margin-top: 0; padding-top: 0;">
                <br />
            </p>
        </div>
        <div style="position: absolute; margin: 20px; margin-right:28px; bottom: 0; right: 0px;" class="opacity-transition" id="l2-redirect">
            <div style="display: grid; grid-template-columns: 1fr 1fr; gap: 35px;">
                <div >
                    <span style="vertical-align: middle;display: inline-block;                ">
                        <svg viewbox="0 0 79 61" xmlns="http://www.w3.org/2000/svg" style="width: 32px; height:32px;">
                            <rect x="0" y="0" width="79" height="61" rx="13" ry="13" fill="#bdc0c2" />
                            <text x="50%" y="50%" dy="17.5px" text-anchor="middle" fill="#4e4e4e" font-size="50"
                                font-weight="bold">L2</text>
                        </svg>
                    </span>
                    <span
                        style="font-size: 18px; vertical-align: middle;display: inline-block; margin-left: 8px; font-weight: bold; color: #fff;">Redirect</span>
                </div>
                <div >
                    <span style="vertical-align: middle;display: inline-block;                ">
                        <svg viewbox="0 0 79 61" xmlns="http://www.w3.org/2000/svg" style="width: 32px; height:32px;">
                            <rect x="0" y="0" width="79" height="61" rx="13" ry="13" fill="#bdc0c2" />
                            <text x="50%" y="50%" dy="17.5px" text-anchor="middle" fill="#4e4e4e" font-size="50"
                                font-weight="bold">R2</text>
                        </svg>
                    </span>
                    <span
                        style="font-size: 18px; vertical-align: middle;display: inline-block; margin-left: 8px; font-weight: bold; color: #fff;">Options</span>
                </div>
            </div>
        </div>

        <div id="before-jb-view">

            <div id="run-jb-parent" class="opacity-transition"
                style="display: flex; justify-content: center; align-items: center; position: absolute; top: 0;left: 0; width: 100%; height: 100%;">
                <a tabindex="0" class="btn" style="max-width: 50%;" id="run-jb" onclick="runJailbreak()">Jailbreak</a>
            </div>

            <div id="jb-progress" class="opacity-transition"
                style="display: flex; flex-direction: column; justify-content: center; align-items: center; position: absolute; top: 0;left: 0; width: 100%; height: 100%; opacity: 0; pointer-events: none;">
                <div class="lds-ellipsis" style="margin-top: 40px;">
                    <div></div>
                    <div></div>
                    <div></div>
                    <div></div>
                </div>
                <p id="jb-progress-status-text" style="text-align: center; width: 100%; margin-top: 40px;">
                    Getting
                    ready...</p>
                <textarea id="console" class="log" style="display: none;"></textarea>
            </div>

            <div id="post-jb-view" class="opacity-transition"
                style="width: 100%;position: relative; max-height: 10px; overflow-y: hidden; margin-top: 200px;">
                <!-- it seems like sometimes display none to anything else and transforms like causing crashes after the jailbreak. idk. -->
                <div id="payloads-list" class="grid-container">
                </div>
            </div>

            <center style="position: fixed;bottom: 0;width: 100%;margin-bottom: 10px;font-size: 13px;"
                class="info opacity-transition" id="credits">
                <a href="https://github.com/idlesauce/PS5-Exploit-Host" style="padding-bottom: -20px;">
                    <svg style="margin-bottom:-5px;" width="25px" height="25px" viewBox="0 0 98 96"
                        xmlns="http://www.w3.org/2000/svg">
                        <path fill-rule="evenodd" clip-rule="evenodd"
                            d="M48.854 0C21.839 0 0 22 0 49.217c0 21.756 13.993 40.172 33.405 46.69 2.427.49 3.316-1.059 3.316-2.362 0-1.141-.08-5.052-.08-9.127-13.59 2.934-16.42-5.867-16.42-5.867-2.184-5.704-5.42-7.17-5.42-7.17-4.448-3.015.324-3.015.324-3.015 4.934.326 7.523 5.052 7.523 5.052 4.367 7.496 11.404 5.378 14.235 4.074.404-3.178 1.699-5.378 3.074-6.6-10.839-1.141-22.243-5.378-22.243-24.283 0-5.378 1.94-9.778 5.014-13.2-.485-1.222-2.184-6.275.486-13.038 0 0 4.125-1.304 13.426 5.052a46.97 46.97 0 0 1 12.214-1.63c4.125 0 8.33.571 12.213 1.63 9.302-6.356 13.427-5.052 13.427-5.052 2.67 6.763.97 11.816.485 13.038 3.155 3.422 5.015 7.822 5.015 13.2 0 18.905-11.404 23.06-22.324 24.283 1.78 1.548 3.316 4.481 3.316 9.126 0 6.6-.08 11.897-.08 13.526 0 1.304.89 2.853 3.316 2.364 19.412-6.52 33.405-24.935 33.405-46.691C97.707 22 75.788 0 48.854 0z"
                            fill="#fff" />
                    </svg>
                    Host GitHub repo
                </a>
                <b id="version"> | v1.04</b>
                <h3 style="margin-top: 0;padding-top: 0;">
                    <br />
                    <a href="https://twitter.com/theflow0">@theflow0</a>,
                    <a href="https://twitter.com/SpecterDev">@SpecterDev</a>,
                    ChendoChap,
                    <a href="https://twitter.com/Znullptr">@Znullptr</a>,
                    <a href="https://twitter.com/sleirsgoevy">@sleirsgoevy</a>,
                    <br />
                    <a href="https://twitter.com/psxdev">@psxdev</a>,
                    <a href="https://twitter.com/flat_z">@flat_z</a>,
                    <a href="https://twitter.com/notzecoxao">@notzecoxao</a>,
                    <a href="https://twitter.com/SocraticBliss">@SocraticBliss</a>,
                    laureeeeeee,
                    abc
                </h3>
            </center>
        </div>
    </div>

    <div id="menu-overlay" style="position: fixed; top: -100%; left: 0; width: 100%; min-height: 100%; height: 100%; background-color: #111111d0; opacity:0;" class="opacity-transition"></div>
    <div id="menu-bar-wrapper" style="position: absolute; top: 0; right: -500px; width: 320px; min-height: 100%; height: 100%; padding: 16px; box-sizing: border-box; transition: right 0.35s ease-in-out;">
        <div id="menu-bar" style="position: relative;top:0;left: 0; width:100%;height:100%; background-color: #161622; border-radius: 16px; padding: 15px; box-sizing: border-box;">
            <p style="margin: 10px; font-weight: bold;">Webkit exploit type:</p>
            <div style="margin: 10px;" >
                <input type="radio" id="wk-exploit-psfree" name="wk-exploit" value="PSFree" onchange="wk_expoit_type_changed(event)" >
                <label for="wk-exploit-psfree" >PSFree</label><br>
                <input type="radio" id="wk-exploit-fontface" name="wk-exploit" value="FontFace" onchange="wk_expoit_type_changed(event)">
                <label for="wk-exploit-fontface">FontFace</label>
            </div>
        </div>
    </div>
    
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
    server.on("/", handleRoot);
    server.on("/send", handleSend);
    server.begin();
    Serial.println("Web server started");
}

void loop() {
    server.handleClient();
}