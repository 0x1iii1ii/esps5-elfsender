// #ifndef TEST_H
// #define TEST_H


// <!DOCTYPE html>
// <html lang="en">
// <head>
//     <meta charset="UTF-8">
//     <meta name="viewport" content="width=device-width, initial-scale=1.0">
//     <title>ESPS5 Payload Sender</title>
//     <style>
//         body {
//             font-family: Arial, sans-serif;
//             background: linear-gradient(to bottom, #1a1a1a, #2a2a2a);
//             color: white;
//             margin: 0;
//             padding: 20px;
//         }
//         .title {
//             text-align: center;
//             font-size: 32px;
//             margin-bottom: 30px;
//             font-weight: bold;
//         }
//         .main {
//             display: grid;
//             grid-template-columns: repeat(2, 1fr);
//             gap: 20px;
//         }
//         .column {
//             display: flex;
//             flex-direction: column;
//         }
//         .payload {
//             background-color: #2c3e50; /* Dark blue background */
//             padding: 20px;
//             border-radius: 8px;
//             margin-bottom: 20px;
//             margin-left: 10px;
//             margin-right: 40px;
//             cursor: pointer;
//             transition: transform 0.2s ease, box-shadow 0.2s ease;
//             box-shadow: 0 4px 8px rgba(0, 0, 0, 0.2);
//         }
//         .payload:hover {
//             transform: scale(1.02);
//             box-shadow: 0 0 0 2px white; /* White highlight on hover */
//         }
//         h2 {
//             font-size: 18px;
//             margin: 0 0 10px 0;
//             font-weight: bold;
//         }
//         p {
//             font-size: 14px;
//             margin: 0 0 10px 0;
//             text-align: left;
//         }
//         .version {
//             color: #aaa;
//         }
//         .footer {
//             text-align: right;
//             font-size: 12px;
//             color: #aaa;
//             margin-top: 20px;
//         }
//     </style>
// </head>
// <body>
//     <div class="title">ESPS5 Payload Sender</div>
//     <div class="main">
//         <!-- Left Column -->
//         <div class="column">
//             <div class="payload" onclick="sendPayload('etahen')">
//                 <h2>etaHEN 1.1b By LM</h2>
//                 <p>Runs With 3.xxx and 4.xxx. FPKG enabler For FW 4.03 Only. <span class="version">v1 beta</span></p>
//             </div>
//             <div class="payload" onclick="sendPayload('kstuff')">
//                 <h2>K-Stuff</h2>
//                 <p>FW 4.03-4.51 ONLY. FPKG enabler <span class="version">v1.2</span></p>
//             </div>
//             <div class="payload" onclick="sendPayload('payload1')">
//                 <h2>Payload 1</h2>
//                 <p>Placeholder for additional payload. <span class="version">v1.0</span></p>
//             </div>
//         </div>
//         <!-- Right Column -->
//         <div class="column">
//             <div class="payload" onclick="sendPayload('ps5elfloader')">
//                 <h2>PS5 Payload ELF Loader</h2>
//                 <p>Uses port 9021. Persistent network ELF loader <span class="version">v0.5</span></p>
//             </div>
//             <div class="payload" onclick="sendPayload('ftps5')">
//                 <h2>FTPS5 (Persistent)</h2>
//                 <p>FTP SERVER</p>
//             </div>
//             <div class="payload" onclick="sendPayload('payload2')">
//                 <h2>Payload 2</h2>
//                 <p>Placeholder for additional payload. <span class="version">v1.0</span></p>
//             </div>
//         </div>
//     </div>
//     <div class="footer">Shows payload information</div>
//     <script>
//         function sendPayload(payload) {
//             fetch(`/send?payload=${payload}`)
//                 .then(response => response.text())
//                 .then(text => alert(text))
//                 .catch(error => alert('Error: ' + error));
//         }
//     </script>
// </body>
// </html>

// #endif