#include <ESP8266WiFi.h>
#include <ESPAsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <EEPROM.h>
extern "C" {
#include "user_interface.h"
}
const char* ssid = "ESPTESTER";
const char* password = "12345678";
const int led1Pin = 5;  // Red
const int led2Pin = 4;  // Green
const int led3Pin = 0;  // Blue
int hueScrollRate = 500;
int brightnessScrollRate = 50;
int OnlineR = 0, OnlineG = 0, OnlineB = 0;
int OfflineR = 128, OfflineG = 0, OfflineB = 128;
bool isOffline = false;
float offlineHue = 0.0;
float brightnessPhase = 0.0;
float offlineLightness = 100.0;
const unsigned long updateInterval = 20;
unsigned long lastUpdate = 0;
String sliderValue1 = "0";
String sliderValue2 = "100";
String sliderValue3 = "50";
AsyncWebServer server(80);
const char index_html[] PROGMEM = R"rawliteral(
<!DOCTYPE HTML>
<html>
<head>
  <meta name="viewport" content="width=device-width, initial-scale=1">
  <title>ESP Web Server</title>
  <style>
    body {
      background-color: #121212;
      color: #f0f0f0;
      font-family: Arial, sans-serif;
      margin: 0;
      padding: 20px;
      text-align: center;
    }
    h2 {
      font-size: 2.3rem;
      margin-bottom: 1rem;
    }
    .card {
      background-color: #1e1e1e;
      padding: 20px;
      margin: 20px auto;
      border-radius: 12px;
      max-width: 500px;
      box-shadow: 0 0 10px rgba(0,0,0,0.4);
    }
    .slider {
      -webkit-appearance: none;
      appearance: none;
      width: 100%;
      height: 5px;
      background: #00f008;
      outline: none;
      border-radius: 8px;
      margin: 10px 0;
      transition: opacity .2s;
    }
    .slider::-webkit-slider-thumb {
      -webkit-appearance: none;
      appearance: none;
      width: 30px;
      height: 30px;
      background: #003249;
      border-radius: 50%;
      cursor: pointer;
    }
    .slider::-moz-range-thumb {
      width: 30px;
      height: 30px;
      background: #003249;
      border-radius: 50%;
      cursor: pointer;
    }
    button {
      background-color: #00f008;
      border: none;
      padding: 12px 24px;
      margin-top: 20px;
      color: black;
      font-size: 16px;
      border-radius: 8px;
      cursor: pointer;
      transition: background-color 0.3s ease;
    }
    button:hover {
      background-color: #00a305;
    }
    input[type="checkbox"] {
      transform: scale(1.3);
      margin-left: 10px;
    }
    label {
      font-size: 1.2rem;
    }
    @media (max-width: 600px) {
      body {
        padding: 10px;
      }
      .card {
        padding: 15px;
        margin: 10px auto;
      }
    }
    /* Credits Popup */
    #creditsPopup {
      display: none;
      position: fixed;
      top: 50%%;
      left: 50%%;
      transform: translate(-50%%, -50%%);
      z-index: 999;
    }
    #helpPopup {
      display: none;
      position: fixed;
      top: 50%%;
      left: 50%%;
      transform: translate(-50%%, -50%%);
      z-index: 999;
      width: 90%;
      max-width: 600px;
      max-height: 80vh; /* Limit height */
      overflow-y: auto; /* Scroll inside if content is too long */
    }
    #overlayHelp {
      display: none;
      position: fixed;
      top: 0; left: 0;
      width: 100%%; height: 100%%;
      background: rgba(0, 0, 0, 0.6);
      z-index: 998;
    }
    #overlayCredits {
      display: none;
      position: fixed;
      top: 0; left: 0;
      width: 100%%; height: 100%%;
      background: rgba(0, 0, 0, 0.6);
      z-index: 998;
    }
    /* Modern Custom Checkbox Switch */
    .switch {
      position: relative;
      display: inline-block;
      width: 50px;
      height: 28px;
      margin-left: 12px;
    }
    .switch input {
      opacity: 0;
      width: 0;
      height: 0;
    }
    .slider-switch {
      position: absolute;
      cursor: pointer;
      top: 0; left: 0;
      right: 0; bottom: 0;
      background-color: #555;
      transition: 0.4s;
      border-radius: 34px;
    }
    .slider-switch:before {
      position: absolute;
      content: "";
      height: 22px;
      width: 22px;
      left: 3px;
      bottom: 3px;
      background-color: #00f008;
      transition: 0.4s;
      border-radius: 50%;
    }
    .switch input:checked + .slider-switch {
      background-color: #00a305;
    }
    .switch input:checked + .slider-switch:before {
      transform: translateX(22px);
    }
    .collapsible-content {
      display: none;
      text-align: left;
      margin-top: 10px;
    }
    h2 {
      cursor: pointer;
      background: #2c2c2c;
      padding: 10px;
      border-radius: 8px;
      transition: background 0.3s ease;
    }
    h2:hover {
      background: #3a3a3a;
    }
    .manual-input {
      width: 60px;
      background: black;
      color: white;
      padding: 6px 8px;
      font-size: 14px;
      border: 1px solid #ccc;
      border-radius: 6px;
      box-shadow: inset 0 1px 3px rgba(0, 0, 0, 0.1);
      outline: none;
      transition: border 0.2s, box-shadow 0.2s;
      appearance: textfield;
    }
    .manual-input:focus {
      border-color: #3f51b5;
      box-shadow: 0 0 0 2px rgba(63, 81, 181, 0.2);
    }
    /* Remove default up/down arrows on number inputs */
    .manual-input::-webkit-outer-spin-button,
    .manual-input::-webkit-inner-spin-button {
      -webkit-appearance: none;
      margin: 0;
    }
  </style>
</head>
<body>
     <div id="overlayCredits" onclick="toggleCredits()"></div>
  <div id="creditsPopup" class="card">
    <p><strong>Software and Product Design</strong> by the 2025 Tech Team</p>
    <p><strong>FOC Acrylic Slide Design</strong> by Mr Butler and the Marketing Team</p>
    <p>With the support of Ms Manwaring, Ms King, Ms Calverley</p>
    <p>Special thanks to the FOC team and Dr Colley</p>
    <p>In conjunction with the 2025 Arts Team</p>
    <button onclick="toggleCredits()">Close</button>
  </div>
    <div id="overlayHelp" onclick="toggleHelp()"></div>
  <div id="helpPopup" class="card">
        <h2 onclick="toggleSection(this)">Hardware Details</h2>
        <div class="collapsible-content">
            <h3>Safety Disclaimer</h3>
            <p>Ensure you have turned off the battery if you are connecting your lamp to a USB power supply. You can access the USB port using the hole on the right of the lamp. If you leave the battery on and the USB power supply connected, you will <span style="color: #ff0000;">fry the lamp</span>.&nbsp;&nbsp;</p>
            <h3>Battery Life</h3>
            <p>The lamp has a particularly short battery life, with about 6 hours run time at full brightness mode. It is recommended to run the lamp using a USB power supply if it does not need to be mobile. It runs on 3x standard AAA Batteries.</p>
            <h3>Changing the Battery</h3>
            <p>Grab a phillips head screw driver and undo the 2 M3x10mm bolts on the underside. The whole electronics assembly will come out. Press down on and slide the breadboard and electronics (white thing) off with the battery cover. Replace the 3xAAA batteries. Turn on the lamp to test. Repeat process backwards.</p>
        </div>
        <h2 onclick="toggleSection(this)">Web GUI</h2>
        <div class="collapsible-content">
            <h3>Changing the Colour</h3>
            <p>Use the Hue, Saturation, and Brightness sliders to change the colours of your acrylic lamp.&nbsp;</p>
            <h3>Scroll Mode</h3>
            <p>This special mode allows you to scroll through the Hues and make the lamp pulsate in terms of brightness. Tick the 'Colour and Brightness Scroll Mode' and the lamp will begin to automatically periodically pulsate. You can use the Scroll Rate Sliders to change the rate at which these properties change.&nbsp;&nbsp;</p>
            <h3>Super Speed Scroll Mode</h3>
            <p>Toggle this mode to make the scroll rates even faster! Stay cautious since this mode will create bright flashing lights. And, it may make parties more crazy.</p>
            <h3>Saving to EEPROM</h3>
            <p>This button will save the current colour to memory so that when the lamp is turned on, it goes to that colour.&nbsp;</p>
            <h3><span style="color: #ff0000;">Warning:</span> You can only set the colour ~100,000 times. Use your 100,000 EEPROM writes carefully ;)</h3>
        </div>
        <h2 onclick="toggleSection(this)">Need more Support...?</h2>
        <div class="collapsible-content">
        <p>Contact your QASMT Tech Team Today, and they'll be in touch with you soon.</p>
        <p>For General Enquiries, contact support at <i style="color: #00f008;">acryliclamphotline@hotmail.com</i></p>
        </div>
        <h2 onclick="toggleSection(this)">For the Curious Engineers</h2>
        <div class="collapsible-content">
        <p>You can access our code and CAD at&nbsp;<span style="color: #00f008;">github.com/combat-wombat-one/AcrylLamp</span></p>
        <p>Make sure you get your parents to sign off on accessing GitHub.</p>
        <p>Feel free to tinker away at your Acrylic lamp to improve the Web Interface, or design your own Acrylic Lamp slides.</p>
        </div>
        <p>&nbsp;</p>
        <p>&nbsp;</p>
        <p>Designed, Programmed and Tested in Australia.&nbsp;</p>
        <p>&nbsp;</p>
    <button onclick="toggleHelp()">Close</button>
  </div>
  <h2>Acrylic Lamp Control</h2>
  <!-- Hue, Saturation, and Brightness Sliders -->
  <div class="card">
    <!-- Hue -->
    <p>Hue: 
      <span id="textSliderValue1"></span> 
      <input type="number" class="manual-input" id="hueInput" min="0" max="359" value="%%SLIDERVALUE1%%" onchange="syncFromInput('hue')">
    </p>
    <!-- Hue Slider -->
    <input type="range" oninput="updateColor(); syncFromSlider('hue')" id="hueSlider" min="0" max="359" value="%SLIDERVALUE1%" step="1" class="slider">
        <!-- Saturation -->
    <p>Saturation: 
      <span id="textSliderValue2"></span> 
      <input type="number" class="manual-input" id="saturationInput" min="0" max="100" value="%%SLIDERVALUE2%%" onchange="syncFromInput('saturation')">
    </p>
        <!-- Saturation Slider -->
    <input type="range" oninput="updateColor(); syncFromSlider('saturation')" id="saturationSlider" min="0" max="100" value="%SLIDERVALUE2%" step="1" class="slider">
        <!-- Brightness -->
    <p>Brightness: 
      <span id="textSliderValue3"></span> 
      <input type="number" class="manual-input" id="lightnessInput" min="0" max="100" value="%%SLIDERVALUE3%%" onchange="syncFromInput('lightness')">
    </p>
        <!-- Brightness Slider -->
    <input type="range" oninput="updateColor(); syncFromSlider('lightness')" id="lightnessSlider" min="0" max="100" value="%SLIDERVALUE3%" step="1" class="slider">
  </div>
  <!-- Offline and Super Speed Toggles -->
<p>
  <label for="offlineToggle">Colour and Brightness Scroll Mode:</label>
  <label class="switch">
    <input type="checkbox" id="offlineToggle" onchange="toggleOffline()">
    <span class="slider-switch"></span>
  </label>
</p>
<p>
  <label for="superSpeedToggle">Super Speed Settings:</label>
  <label class="switch">
    <input type="checkbox" id="superSpeedToggle" onchange="toggleSuperSpeed()">
    <span class="slider-switch"></span>
  </label>
</p>
  <!-- Hue and Brightness Scroll Rate -->
  <div class="card">
    <p>Hue Scroll Rate: <span id="hueRateValue">500</span></p>
    <input type="range" id="hueRateSlider" min="0" max="700" value="500" step="1" class="slider" oninput="updateHueRate(this.value)">

    <p>Brightness Scroll Rate: <span id="brightnessRateValue">50</span></p>
    <input type="range" id="brightnessRateSlider" min="0" max="300" value="50" step="1" class="slider" oninput="updateBrightnessRate(this.value)">
  </div>
  <!-- Save and Credits Buttons -->
  <button onclick="saveColor()">Save to EEPROM</button>
  <div>
    <button onclick="toggleHelp()">Help</button>
  </div>
  <div>
    <button onclick="toggleCredits()">Credits</button>
  </div>

<script>
function updateColor() {
  var h = parseInt(document.getElementById("hueSlider").value);
  var s = document.getElementById("saturationSlider").value;
  var l = parseInt(document.getElementById("lightnessSlider").value)/2;
  if (isNaN(h)) return;
  let flippedHue = (360 - h) % 360;
  let rgb = hslToRgb(flippedHue, s, l);
  fetch(`/setColor?r=${rgb[0]}&g=${rgb[1]}&b=${rgb[2]}`);
}
function toggleOffline() {
  let off = document.getElementById("offlineToggle").checked ? 1 : 0;
  fetch(`/setColor?offline=${off}`);
}
function updateHueRate(v) {
  document.getElementById("hueRateValue").textContent = v;
  fetch(`/setScrollRates?hueRate=${v}`);
}
function updateBrightnessRate(v) {
  document.getElementById("brightnessRateValue").textContent = v;
  fetch(`/setScrollRates?brightnessRate=${v}`);
}

function toggleSuperSpeed() {
  const enabled = document.getElementById("superSpeedToggle").checked;
  let hueSlider = document.getElementById("hueRateSlider");
  let brightnessSlider = document.getElementById("brightnessRateSlider");
  hueSlider.max = enabled ? 4000 : 700;
  brightnessSlider.max = enabled ? 2000 : 300;
}
function saveColor() {
  fetch('/saveColor');
}
function hslToRgb(h, s, l) {
  s /= 100; 
  l /= 100;
  let c = (1 - Math.abs(2 * l - 1)) * s;
  let x = c * (1 - Math.abs((h / 60) % 2 - 1));
  let m = l - c/2;
  let r=0,g=0,b=0;
  if (h<60)       { r=c; g=x; b=0; }
  else if (h<120) { r=x; g=c; b=0; }
  else if (h<180) { r=0; g=c; b=x; }
  else if (h<240) { r=0; g=x; b=c; }
  else if (h<300) { r=x; g=0; b=c; }
  else            { r=c; g=0; b=x; }
  return [
    Math.round((r+m)*255),
    Math.round((g+m)*255),
    Math.round((b+m)*255)
  ];
}
function toggleCredits() {
  const popup = document.getElementById("creditsPopup");
  const overlay = document.getElementById("overlayCredits");
  // Check if the popup is visible by inspecting its current display style
  const isVisible = popup.style.display === "block";
  // Toggle the visibility of the popup and overlay
  popup.style.display = isVisible ? "none" : "block";
  overlay.style.display = isVisible ? "none" : "block";
}
function toggleHelp() {
  const popup = document.getElementById("helpPopup");
  const overlay = document.getElementById("overlayHelp");
  // Check if popup is visible or not by looking at its display style
  const isVisible = popup.style.display === "block";
   // Toggle the visibility of the popup and overlay
  popup.style.display = isVisible ? "none" : "block";
  overlay.style.display = isVisible ? "none" : "block";
}
function toggleSection(header) {
  const content = header.nextElementSibling;
  if (content && content.classList.contains("collapsible-content")) {
    content.style.display = content.style.display === "block" ? "none" : "block";
  }
}
function syncFromSlider(type) {
  document.getElementById(type + "Input").value = document.getElementById(type + "Slider").value;
}
function syncFromInput(type) {
  let inputElement = document.getElementById(type + "Input");
  let sliderElement = document.getElementById(type + "Slider");
  // Check if both elements exist
  if (!inputElement || !sliderElement) {
    console.error('Element not found for type:', type);
    return;
  }
  let value = inputElement.value;
  sliderElement.value = value;
  updateColor(); // Recalculate color immediately
}
function syncScrollSlider(type) {
  document.getElementById(type + "Input").value = document.getElementById(type + "Slider").value;
}
function syncScrollInput(type) {
  let value = document.getElementById(type + "Input").value;
  document.getElementById(type + "Slider").value = value;
}
</script>
</body>
</html>
)rawliteral";

String processor(const String& var) {
  if (var == "SLIDERVALUE1") return sliderValue1;
  if (var == "SLIDERVALUE2") return sliderValue2;
  if (var == "SLIDERVALUE3") return sliderValue3;
  return String();
}
void hslToRgb(float h, float s, float l, int& r, int& g, int& b) {
  if (h > 359.0) h = 359.0;
  s /= 100.0; l /= 100.0;
  float c = (1 - fabs(2 * l - 1)) * s;
  float x = c * (1 - fabs(fmod(h / 60.0, 2) - 1));
  float m = l - c / 2.0;
  float r1=0, g1=0, b1=0;
  if      (h < 60)  { r1 = c; g1 = x; b1 = 0; }
  else if (h < 120) { r1 = x; g1 = c; b1 = 0; }
  else if (h < 180) { r1 = 0; g1 = c; b1 = x; }
  else if (h < 240) { r1 = 0; g1 = x; b1 = c; }
  else if (h < 300) { r1 = x; g1 = 0; b1 = c; }
  else              { r1 = c; g1 = 0; b1 = x; }
  r = round((r1 + m) * 255);
  g = round((g1 + m) * 255);
  b = round((b1 + m) * 255);
}
float rgbToHue(int r, int g, int b) {
  float fr = r / 255.0;
  float fg = g / 255.0;
  float fb = b / 255.0;
  float maxVal = max(fr, max(fg, fb));
  float minVal = min(fr, min(fg, fb));
  float delta = maxVal - minVal;
  float h = 0;
  if (delta == 0)
    h = 0;
  else if (maxVal == fr)
    h = fmod((60 * ((fg - fb) / delta) + 360), 360);
  else if (maxVal == fg)
    h = fmod((60 * ((fb - fr) / delta) + 120), 360);
  else if (maxVal == fb)
    h = fmod((60 * ((fr - fg) / delta) + 240), 360);
  return h;
}
void applyLEDColor() {
  analogWrite(led1Pin, isOffline ? OfflineR : OnlineR);
  analogWrite(led2Pin, isOffline ? OfflineG : OnlineG);
  analogWrite(led3Pin, isOffline ? OfflineB : OnlineB);
}
void saveColorToEEPROM(int r, int g, int b) {
  if (EEPROM.read(0) != r || EEPROM.read(1) != g || EEPROM.read(2) != b) {
    EEPROM.write(0, r);
    EEPROM.write(1, g);
    EEPROM.write(2, b);
    EEPROM.commit();
}
}
void loadColorFromEEPROM() {
  OnlineR = EEPROM.read(0);
  OnlineG = EEPROM.read(1);
  OnlineB = EEPROM.read(2);
}
void setup() {
  Serial.begin(115200);
  EEPROM.begin(512);
  analogWriteRange(255);
  analogWriteFreq(1000);
  wifi_set_sleep_type(LIGHT_SLEEP_T);
  pinMode(led1Pin, OUTPUT);
  pinMode(led2Pin, OUTPUT);
  pinMode(led3Pin, OUTPUT);
  loadColorFromEEPROM();
  applyLEDColor();
  WiFi.mode(WIFI_AP);
  WiFi.softAP(ssid, password);
  Serial.print("IP: "); Serial.println(WiFi.softAPIP());
  WiFi.setOutputPower(0);
  server.on("/", HTTP_GET, [](AsyncWebServerRequest* req) {
    req->send_P(200, "text/html", index_html, processor);
  });
  server.on("/setColor", HTTP_GET, [](AsyncWebServerRequest* req) {
    if (req->hasParam("offline")) {
      bool newOffline = req->getParam("offline")->value().toInt() == 1;
      if (!isOffline && newOffline) {
        offlineLightness = 50.0;
        brightnessPhase = 0.0;
        offlineHue = rgbToHue(OnlineR, OnlineG, OnlineB);  // Sync hue
      }
      isOffline = newOffline;
    }

    if (req->hasParam("r") && req->hasParam("g") && req->hasParam("b")) {
      OnlineR = req->getParam("r")->value().toInt();
      OnlineG = req->getParam("g")->value().toInt();
      OnlineB = req->getParam("b")->value().toInt();
    }
    applyLEDColor();
    req->send(200, "text/plain", "OK");
  });
  server.on("/setScrollRates", HTTP_GET, [](AsyncWebServerRequest* req) {
    if (req->hasParam("hueRate"))
      hueScrollRate = req->getParam("hueRate")->value().toInt();
    if (req->hasParam("brightnessRate"))
      brightnessScrollRate = req->getParam("brightnessRate")->value().toInt();
    req->send(200, "text/plain", "OK");
  });
  server.on("/saveColor", HTTP_GET, [](AsyncWebServerRequest* req) {
    saveColorToEEPROM(OnlineR, OnlineG, OnlineB);
    req->send(200, "text/plain", "Saved");
  });
  server.begin();
}
void loop() {
  unsigned long now = millis();
  if (isOffline && now - lastUpdate >= updateInterval) {
    lastUpdate = now;
    offlineHue += (hueScrollRate / 10.0) * (updateInterval / 1000.0);
    if (offlineHue >= 360.0) offlineHue -= 360.0;
    brightnessPhase += (brightnessScrollRate / 50.0) * (updateInterval / 1000.0) * PI;
    if (brightnessPhase >= TWO_PI) brightnessPhase -= TWO_PI;
    float lightness = sin(brightnessPhase) * 25.0 + 25.0;
    hslToRgb(offlineHue, 100.0, lightness, OfflineR, OfflineG, OfflineB);
    applyLEDColor();
  }
  delay(1);
}
