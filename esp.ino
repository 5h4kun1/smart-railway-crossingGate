#include <WiFi.h>
#include <WebServer.h>

// Communication pins
#define NANO_TX_PIN 1    
#define NANO_RX_PIN 3    
#define STATUS_LED 2     

// Network credentials
const char* ssid = "Railway_Crossing_AP";
const char* password = "railway123";

WebServer server(80);
HardwareSerial nanoSerial(1);

// System variables
String gateStatus = "UNKNOWN";
bool trainDetected = false;
bool manualMode = false;
unsigned long lastStatusUpdate = 0;
unsigned long lastLEDUpdate = 0;

void setup() {
  Serial.begin(115200);
  delay(1000);
  
  Serial.println("Starting Railway Crossing Control System...");
  
  // Initialize communication with Nano
  nanoSerial.begin(9600, SERIAL_8N1, NANO_RX_PIN, NANO_TX_PIN);
  
  // Initialize status LED
  pinMode(STATUS_LED, OUTPUT);
  digitalWrite(STATUS_LED, LOW);
  
  // LED startup sequence
  for(int i = 0; i < 5; i++) {
    digitalWrite(STATUS_LED, HIGH);
    delay(200);
    digitalWrite(STATUS_LED, LOW);
    delay(200);
  }
  
  // Create WiFi Access Point
  Serial.println("Creating WiFi Access Point...");
  WiFi.mode(WIFI_AP);
  bool apResult = WiFi.softAP(ssid, password);
  
  if (apResult) {
    Serial.println("WiFi AP Started Successfully");
    Serial.print("SSID: ");
    Serial.println(ssid);
    Serial.print("IP Address: ");
    Serial.println(WiFi.softAPIP());
  } else {
    Serial.println("Failed to start WiFi AP!");
  }
  
  // Setup web server routes
  server.on("/", handleRoot);
  server.on("/control", handleControl);
  server.on("/status", handleStatus);
  server.on("/style.css", handleCSS);
  server.on("/script.js", handleJS);
  
  server.onNotFound([]() {
    server.send(404, "text/plain", "Page not found");
  });
  
  server.begin();
  Serial.println("Web server started on port 80");
  
  // Initial status request
  delay(2000);
  requestStatus();
  
  Serial.println("System initialization complete!");
}

void loop() {
  server.handleClient();
  handleNanoResponse();
  updateStatusLED();
  
  // Request status every 3 seconds
  if (millis() - lastStatusUpdate > 3000) {
    requestStatus();
    lastStatusUpdate = millis();
  }
  
  delay(10);
}

void handleRoot() {
  String html = F("<!DOCTYPE html><html><head>");
  html += F("<meta charset='UTF-8'><meta name='viewport' content='width=device-width,initial-scale=1'>");
  html += F("<title>Railway Crossing Control</title>");
  html += F("<link rel='stylesheet' href='/style.css'>");
  html += F("</head><body>");
  html += F("<div class='container'>");
  html += F("<h1>🚂 Railway Crossing</h1>");
  html += F("<div class='error' id='error'></div>");
  html += F("<div class='status'>");
  html += F("<div>Gate: <span id='gate'>Loading...</span></div>");
  html += F("<div>Mode: <span id='mode'>Loading...</span></div>");
  html += F("</div>");
  html += F("<div class='train' id='train'>Checking...</div>");
  html += F("<div class='controls'>");
  html += F("<button onclick='cmd(\"open\")' class='btn-open'>Open Gate</button>");
  html += F("<button onclick='cmd(\"close\")' class='btn-close'>Close Gate</button>");
  html += F("<button onclick='cmd(\"auto\")' class='btn-auto'>Auto Mode</button>");
  html += F("</div>");
  html += F("<div class='footer'>Last Update: <span id='time'>Never</span></div>");
  html += F("</div>");
  html += F("<script src='/script.js'></script>");
  html += F("</body></html>");
  
  server.send(200, "text/html", html);
}

void handleCSS() {
  String css = F("*{margin:0;padding:0;box-sizing:border-box}");
  css += F("body{font-family:Arial,sans-serif;background:linear-gradient(135deg,#667eea,#764ba2);color:white;min-height:100vh;padding:20px}");
  css += F(".container{max-width:500px;margin:0 auto;background:rgba(255,255,255,0.1);border-radius:15px;backdrop-filter:blur(10px);padding:20px;box-shadow:0 8px 32px rgba(0,0,0,0.3)}");
  css += F("h1{text-align:center;margin-bottom:20px;text-shadow:2px 2px 4px rgba(0,0,0,0.3)}");
  css += F(".status{background:rgba(255,255,255,0.2);padding:15px;border-radius:10px;margin:15px 0;border-left:4px solid #fff}");
  css += F(".status div{display:flex;justify-content:space-between;margin:8px 0;font-size:16px}");
  css += F(".status span{font-weight:bold;padding:3px 10px;border-radius:15px;background:rgba(255,255,255,0.2)}");
  css += F(".train{text-align:center;font-size:20px;margin:15px 0;padding:12px;border-radius:10px;font-weight:bold}");
  css += F(".train-yes{background:#ff4444;animation:pulse 1s infinite}");
  css += F(".train-no{background:#44ff44}");
  css += F("@keyframes pulse{0%{opacity:1}50%{opacity:0.5}100%{opacity:1}}");
  css += F(".controls{display:grid;grid-template-columns:repeat(auto-fit,minmax(120px,1fr));gap:10px;margin:20px 0}");
  css += F("button{padding:12px;border:none;border-radius:8px;font-size:14px;font-weight:bold;cursor:pointer;transition:all 0.3s ease;text-transform:uppercase}");
  css += F(".btn-open{background:#4CAF50;color:white}");
  css += F(".btn-close{background:#f44336;color:white}");
  css += F(".btn-auto{background:#2196F3;color:white}");
  css += F("button:hover{transform:translateY(-2px);box-shadow:0 4px 12px rgba(0,0,0,0.3)}");
  css += F("button:active{transform:translateY(0)}");
  css += F("button:disabled{opacity:0.5;cursor:not-allowed;transform:none}");
  css += F(".footer{text-align:center;margin-top:20px;opacity:0.8;font-size:12px}");
  css += F(".error{background:#ff4444;color:white;padding:10px;border-radius:5px;margin:10px 0;display:none}");
  css += F("@media (max-width:480px){.container{padding:10px;margin:10px}.controls{grid-template-columns:1fr}}");
  
  server.send(200, "text/css", css);
}

void handleJS() {
  String js = F("let busy=false;");
  js += F("function showError(msg){");
  js += F("const e=document.getElementById('error');");
  js += F("e.textContent=msg;e.style.display='block';");
  js += F("setTimeout(()=>e.style.display='none',4000);}");
  
  js += F("function setBtns(enabled){");
  js += F("document.querySelectorAll('button').forEach(b=>b.disabled=!enabled);}");
  
  js += F("function cmd(action){");
  js += F("if(busy){showError('Please wait...');return;}");
  js += F("busy=true;setBtns(false);");
  js += F("fetch('/control?action='+action)");
  js += F(".then(r=>{if(!r.ok)throw new Error('Failed');return r.text();})");
  js += F(".then(data=>setTimeout(updateStatus,500))");
  js += F(".catch(err=>showError('Error: '+err.message))");
  js += F(".finally(()=>{busy=false;setBtns(true);});}");
  
  js += F("function updateStatus(){");
  js += F("fetch('/status')");
  js += F(".then(r=>{if(!r.ok)throw new Error('Status failed');return r.json();})");
  js += F(".then(data=>{");
  js += F("document.getElementById('gate').textContent=data.gate;");
  js += F("document.getElementById('mode').textContent=data.manual?'Manual':'Auto';");
  js += F("const train=document.getElementById('train');");
  js += F("if(data.train){train.innerHTML='🚨 TRAIN DETECTED';train.className='train train-yes';}");
  js += F("else{train.innerHTML='✅ Track Clear';train.className='train train-no';}");
  js += F("document.getElementById('time').textContent=new Date().toLocaleTimeString();");
  js += F("})");
  js += F(".catch(err=>showError('Update failed'));}");
  
  js += F("setInterval(updateStatus,2000);");
  js += F("setTimeout(updateStatus,1000);");
  
  server.send(200, "application/javascript", js);
}

void handleControl() {
  String action = server.arg("action");
  String response = "";
  
  if (action.length() == 0) {
    server.send(400, "text/plain", "Missing action");
    return;
  }
  
  if (action == "open") {
    nanoSerial.println("MANUAL_OPEN");
    response = "Opening gate";
    manualMode = true;
    Serial.println("Manual OPEN sent");
  }
  else if (action == "close") {
    nanoSerial.println("MANUAL_CLOSE");
    response = "Closing gate";
    manualMode = true;
    Serial.println("Manual CLOSE sent");
  }
  else if (action == "auto") {
    nanoSerial.println("AUTO_MODE");
    response = "Auto mode active";
    manualMode = false;
    Serial.println("AUTO mode sent");
  }
  else {
    server.send(400, "text/plain", "Invalid command");
    return;
  }
  
  server.send(200, "text/plain", response);
}

void handleStatus() {
  String json = "{";
  json += "\"gate\":\"" + gateStatus + "\",";
  json += "\"train\":" + String(trainDetected ? "true" : "false") + ",";
  json += "\"manual\":" + String(manualMode ? "true" : "false") + ",";
  json += "\"uptime\":" + String(millis() / 1000);
  json += "}";
  
  server.send(200, "application/json", json);
}

void handleNanoResponse() {
  while (nanoSerial.available()) {
    String response = nanoSerial.readStringUntil('\n');
    response.trim();
    
    if (response.length() > 0) {
      Serial.println("Nano: " + response);
      
      if (response.startsWith("STATE:")) {
        parseStatusResponse(response);
      }
      else if (response == "GATE_CLOSED") {
        gateStatus = "CLOSED";
      }
      else if (response == "GATE_OPENED") {
        gateStatus = "OPEN";
      }
      else if (response == "AUTO_ACTIVE") {
        manualMode = false;
      }
      else if (response == "MANUAL_ACTIVE") {
        manualMode = true;
      }
      else if (response.startsWith("ERROR:")) {
        Serial.println("Nano error: " + response);
      }
    }
  }
}

void parseStatusResponse(String response) {
  int stateStart = response.indexOf("STATE:") + 6;
  int trainStart = response.indexOf("TRAIN:") + 6;
  int manualStart = response.indexOf("MANUAL:") + 7;
  
  if (stateStart > 5 && trainStart > 5 && manualStart > 6) {
    if (stateStart < response.length() && 
        trainStart < response.length() && 
        manualStart < response.length()) {
      
      int state = response.charAt(stateStart) - '0';
      trainDetected = (response.charAt(trainStart) == '1');
      manualMode = (response.charAt(manualStart) == '1');
      
      switch (state) {
        case 0: gateStatus = "OPEN"; break;
        case 1: gateStatus = "CLOSED"; break;
        case 2: gateStatus = "OPENING"; break;
        case 3: gateStatus = "CLOSING"; break;
        default: gateStatus = "UNKNOWN"; break;
      }
      
      Serial.println("Parsed - Gate: " + gateStatus + 
                    ", Train: " + String(trainDetected) + 
                    ", Manual: " + String(manualMode));
    }
  }
}

void updateStatusLED() {
  unsigned long currentTime = millis();
  
  if (currentTime - lastLEDUpdate > 100) {
    if (trainDetected) {
      digitalWrite(STATUS_LED, (currentTime / 100) % 2);
    } else if (manualMode) {
      digitalWrite(STATUS_LED, (currentTime / 1000) % 2);
    } else {
      digitalWrite(STATUS_LED, HIGH);
    }
    lastLEDUpdate = currentTime;
  }
}

void requestStatus() {
  if (nanoSerial) {
    nanoSerial.println("STATUS");
    Serial.println("Status requested");
  }
}
