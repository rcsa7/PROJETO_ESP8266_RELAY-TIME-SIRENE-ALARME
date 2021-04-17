/*********
 * ESP8266 
 * SIRENE ALARME
 * TIMER OFF
 * IP FIXO
 * WIFIMANAGER
 * OTA
 * 
 * 
  Rui Santos
  Complete project details at https://RandomNerdTutorials.com/esp32-esp8266-web-server-timer-pulse/
  
  The above copyright notice and this permission notice shall be included in all
  copies or substantial portions of the Software.
*********/

// Import required libraries
#ifdef ESP32
  #include <WiFi.h>
  #include <AsyncTCP.h>
#else
  #include <ESP8266WiFi.h>
  #include <ESPAsyncTCP.h>
  #include <ESP8266mDNS.h>
  #include <WiFiUdp.h>
  #include <ArduinoOTA.h>
  #include "ESP8266WiFi.h"
  #include "ESPAsyncWebServer.h"
#endif
#include <ESPAsyncWebServer.h>
#include <ESPAsyncWiFiManager.h>         //https://github.com/tzapu/WiFiManager


//---- Replace with your network credentials
////const char* ssid = "2.4GHz_Acher";
////const char* password = "cadeafaca999";

//IP do ESP (para voce acessar pelo browser)
IPAddress ip(192, 168, 1, 177);

//IP do roteador da sua rede wifi
IPAddress gateway(192, 168, 1, 1);

//Mascara de rede da sua rede wifi
IPAddress subnet(255, 255, 255, 0);

const char* PARAM_INPUT_1 = "state";
const char* PARAM_INPUT_2 = "value";

const int output = 12;// GPIO12 D6// liga rele duplo
const int output1 = 13;// GPIO13 D7// liga rele duplo

// modifique aqui o timer rele
String timerSliderValue = "40";//10--- tempo espera para desligar rele

//------ Create AsyncWebServer object on port 80
AsyncWebServer server(80);
DNSServer dns;
AsyncWiFiManager wifiManager(&server,&dns);// wifimanager esp8266 Asinc

//-----------------------------------------------------------
const char index_html[] PROGMEM = R"rawliteral(
<!DOCTYPE HTML><html>
<head>
  <meta name="viewport" content="width=device-width, initial-scale=1">
  <title>ESP8266 Web Server </title>
  <style>
    html {font-family: Arial; display: inline-block; text-align: center;}
    h2 {font-size: 2.4rem;}
    p {font-size: 2.2rem;}
    body {max-width: 600px; margin:0px auto; padding-bottom: 25px;}
    .switch {position: relative; display: inline-block; width: 120px; height: 68px} 
    .switch input {display: none}
    .slider {position: absolute; top: 0; left: 0; right: 0; bottom: 0; background-color: #ccc; border-radius: 34px}
    .slider:before {position: absolute; content: ""; height: 52px; width: 52px; left: 8px; bottom: 8px; background-color: #fff; -webkit-transition: .4s; transition: .4s; border-radius: 68px}
    input:checked+.slider {background-color: #2196F3}
    input:checked+.slider:before {-webkit-transform: translateX(52px); -ms-transform: translateX(52px); transform: translateX(52px)}
    .slider2 { -webkit-appearance: none; margin: 14px; width: 300px; height: 20px; background: #ccc;
      outline: none; -webkit-transition: .2s; transition: opacity .2s;}
    .slider2::-webkit-slider-thumb {-webkit-appearance: none; appearance: none; width: 30px; height: 30px; background: #2f4468; cursor: pointer;}
    .slider2::-moz-range-thumb { width: 30px; height: 30px; background: #2f4468; cursor: pointer; } 
  </style>
</head>
<body>
  <h2>ESP8266 Web Server Sinena Ota-Wm</h2>
  <p><span id="timerValue">%TIMERVALUE%</span> s</p>
  <p><input type="range" onchange="updateSliderTimer(this)" id="timerSlider" min="1" max="20" value="%TIMERVALUE%" step="1" class="slider2"></p>
  %BUTTONPLACEHOLDER%
<script>
function toggleCheckbox(element) {
  var sliderValue = document.getElementById("timerSlider").value;
  var xhr = new XMLHttpRequest();
  if(element.checked){ xhr.open("GET", "/update?state=1", true); xhr.send();
    var count = sliderValue, timer = setInterval(function() {
      count--; document.getElementById("timerValue").innerHTML = count;
      if(count == 0){ clearInterval(timer); document.getElementById("timerValue").innerHTML = document.getElementById("timerSlider").value; }
    }, 1000);
    sliderValue = sliderValue*1000;
    setTimeout(function(){ xhr.open("GET", "/update?state=0", true); 
    document.getElementById(element.id).checked = false; xhr.send(); }, sliderValue);
  }
}
function updateSliderTimer(element) {
  var sliderValue = document.getElementById("timerSlider").value;
  document.getElementById("timerValue").innerHTML = sliderValue;
  var xhr = new XMLHttpRequest();
  xhr.open("GET", "/slider?value="+sliderValue, true);
  xhr.send();
}
</script>
</body>
</html>
)rawliteral";

// Replaces placeholder with button section in your web page
String processor(const String& var){
  //Serial.println(var);
  if(var == "BUTTONPLACEHOLDER"){
    String buttons = "";
    String outputStateValue = outputState();
    buttons+= "<p><label class=\"switch\"><input type=\"checkbox\" onchange=\"toggleCheckbox(this)\" id=\"output\" " + outputStateValue + "><span class=\"slider\"></span></label></p>";
    return buttons;
  }
  else if(var == "TIMERVALUE"){
    return timerSliderValue;
  }
  return String();
}

String outputState(){
  if(digitalRead(output)){
    return "checked";
  }
  else {
    return "";
  }
  return "";
}

void setup(){
  // Serial port for debugging purposes
  Serial.begin(115200);
 //---------wifimanager Ansync para esp8266-----------------------------------
  
   wifiManager.startConfigPortalModeless("Relay_Timer", "password");
   //------------------------------------------------------------------------------------
  pinMode(output, OUTPUT);// liga rele duplo
  digitalWrite(output, LOW);
  pinMode(output1, OUTPUT);// liga rele duplo
  digitalWrite(output1, LOW);

  //--- Connect to Wi-Fi
   WiFi.config(ip, gateway, subnet);
 /// WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.println("Connecting to WiFi..");
  }


//----- Port defaults to 8266
  // ArduinoOTA.setPort(8266);

  // Hostname defaults to esp8266-[ChipID]
  ArduinoOTA.setHostname("RELAY_TIMER");

  // No authentication by default
  ArduinoOTA.setPassword((const char *)"123");

  ArduinoOTA.onStart([]() {
    Serial.println("Start");
  });
  ArduinoOTA.onEnd([]() {
    Serial.println("\nEnd");
  });
  ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
    Serial.printf("Progress: %u%%\r", (progress / (total / 100)));
  });
  ArduinoOTA.onError([](ota_error_t error) {
    Serial.printf("Error[%u]: ", error);
    if (error == OTA_AUTH_ERROR) Serial.println("Auth Failed");
    else if (error == OTA_BEGIN_ERROR) Serial.println("Begin Failed");
    else if (error == OTA_CONNECT_ERROR) Serial.println("Connect Failed");
    else if (error == OTA_RECEIVE_ERROR) Serial.println("Receive Failed");
    else if (error == OTA_END_ERROR) Serial.println("End Failed");
  });
  ArduinoOTA.begin();
  
  Serial.println("Ready");
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());

//----- Print ESP Local IP Address
  Serial.println(WiFi.localIP());



//------------ Route for root / web page
  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send_P(200, "text/html", index_html, processor);
  });

  // Send a GET request to <ESP_IP>/update?state=<inputMessage>
  server.on("/update", HTTP_GET, [] (AsyncWebServerRequest *request) {
    String inputMessage;
    // GET input1 value on <ESP_IP>/update?state=<inputMessage>
    if (request->hasParam(PARAM_INPUT_1)) {
      inputMessage = request->getParam(PARAM_INPUT_1)->value();
      digitalWrite(output, inputMessage.toInt()); // liga rele duplo
      digitalWrite(output1, inputMessage.toInt()); //liga rele duplo
    }
    else {
      inputMessage = "No message sent";
    }
    Serial.println(inputMessage);
    request->send(200, "text/plain", "OK");
  });
  
  // Send a GET request to <ESP_IP>/slider?value=<inputMessage>
  server.on("/slider", HTTP_GET, [] (AsyncWebServerRequest *request) {
    String inputMessage;
    // GET input1 value on <ESP_IP>/slider?value=<inputMessage>
    if (request->hasParam(PARAM_INPUT_2)) {
      inputMessage = request->getParam(PARAM_INPUT_2)->value();
      timerSliderValue = inputMessage;
    }
    else {
      inputMessage = "No message sent";
    }
    Serial.println(inputMessage);
    request->send(200, "text/plain", "OK");
  });
  
  // Start server
  server.begin();
}
  
void loop() {
  //---------wifimanager Ansync para esp8266------------------
    wifiManager.loop(); 
//--------------------------------------------------------------------------- 
}
