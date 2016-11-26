#include <ESP8266WiFi.h>
#include <WebSocketClient.h>
#include <ESP8266HTTPClient.h>
#include <EEPROM.h>
#include <ArduinoJson.h>

struct wifi_config{
  String ssid;
  String password;
};

wifi_config APconfig = {"IOT Connect","iotconnect"};

wifi_config RouterConfig;

String payload;
String hoststring;

String data;
const char *action,*head,*body;
String dataToSend;
char r;

bool set = false;
bool setup_done = false;
bool socket_setup = false;
bool hard_reset = false;

char path[] = "/";
char host[] = "192.168.0.35:8085";
String chipid = String(ESP.getChipId());

uint8_t attempts = 0;
uint8_t i;

int eepromVal;
bool eepromDone = false;

#define reset_btn D8

WiFiClient client;
WiFiClient socketClient;
WiFiServer server(80);

HTTPClient http;
WebSocketClient webSocketClient;


void setup() {
  Serial.begin(9600);
  EEPROM.begin(512);
  pinMode(reset_btn,INPUT);
  eepromVal = EEPROM.read(0);
  if(eepromVal != 255)
  {
    EEPROM.get( 0, RouterConfig);
    eepromDone = true;
    setup_done = true;
    delay(5000);
  }
  else
  {
    delay(5000);
    sendDisplay("Status","Configuring");
    delay(1000);
    WiFi.mode(WIFI_AP);
    WiFi.softAP(APconfig.ssid.c_str(), APconfig.password.c_str());
    IPAddress myIP = WiFi.softAPIP();
    char ipno2[26] ;
    sprintf(ipno2, "%d.%d.%d.%d", myIP[0], myIP[1], myIP[2], myIP[3]);
    sendDisplay(APconfig.ssid,ipno2);
  }
  //start UART and the server
  server.begin();
}

void loop() {  
  checkReset();
  if(setup_done)
  {
    if(set)
    { 
      if(socket_setup)
      {
        cont_socket();     
      }
      else
      {
        cont();
      }
    }
    else
    {
      con();
    }
  }  
  else
    set_wifi();
}

void cont()
{
  sendDisplay("Status","Connecting...");
  delay(1000);
  if (socketClient.connect("192.168.0.35", 8085)) {
    sendDisplay("Status","Connected");
  } else {
    sendDisplay("Status","Failed");
    hard_reset = false;
    mod_reset();
    return;
  }
//  socket_setup=true;
  delay(1000);
  sendDisplay("Status","Waiting For HS");
//  socket_setup=true;
  // Handshake with the server
  webSocketClient.path = path;
  webSocketClient.host = host;
  if (webSocketClient.handshake(socketClient)) {
    sendDisplay("Status","HS successful");
    socket_setup=true;
    delay(1000);
    sendDisplay("Visit","192.168.0.35");
    i=0;
  } else {
    sendDisplay("Status","HS failed.");
    hard_reset = false;
    mod_reset();
    return;
  }
}

//Socket Connect.
void cont_socket()
{
  i = 1;
  while(i!=0)
  {
    checkReset();
    data = "";
    if (socketClient.connected()) 
    {  
      webSocketClient.getData(data);
      data.trim();
      if(data != "")
      {
        StaticJsonBuffer<200> jsonBuffer;
        JsonObject& root = jsonBuffer.parseObject(data);
        if (root.success()) 
        {
          action = root["action"];
          if(comp(action,"ESPID"))
          {
            StaticJsonBuffer<200> jsonBuffer;
            JsonObject& root = jsonBuffer.createObject();
            root["action"] = "CHIPID";
            root["value"] = chipid;
            char buffer[256];
            root.printTo(buffer, sizeof(buffer));
            webSocketClient.sendData(buffer);
          }
          else
          {
            if (data.length() > 0) {
              Serial.println(data);
              delay(100);
            }
          }
        }
      }
      if(Serial.available())
      {
        webSocketClient.sendData(Serial.readStringUntil('\n'));
      }
    } 
    else {
      sendDisplay("Status","Client disconnected");
      hard_reset = false;
      mod_reset();
      return;
    }
    delay(5);
    i++;
  }
}



void set_wifi()
{ 
  client = server.available();
  if (!client) {
    return;
  }
  while(!client.available()){
    delay(1);
  }
  String req = client.readStringUntil('\r');
  client.flush();
  if (req.indexOf("/setup") != -1)
  {
    String que = req.substring(req.indexOf("/setup?")+7,req.indexOf(" HTTP"));
    for (int i = 0; i < que.length(); i++) {
      if (que.substring(i, i+1) == "&") {
        RouterConfig.ssid = que.substring(5, i);
        RouterConfig.password = que.substring(i+10);
        break;
      }
    }
    sendDisplay("Status",RouterConfig.ssid + " is configured");
    char temp[1000];
    snprintf ( temp, 1000,
"<html>\
  <head>\
  <meta name='viewport' content='width=device-width, initial-scale=1'>\
    <title>IOT Connect</title>\
    <style>\
      body { background-color: #cccccc; font-family: Arial, Helvetica, Sans-Serif; Color: #000088; }\
    </style>\
  </head>\
  <body>\
    <h1>IOT Connect</h1>\
    <p>Setup done for %s</p>\
    <form action='start' onSubmit= 'submit();window.close();'>\
      <p id='con_bt'><input type='button' value='Connect' onclick='httpGet(\"start\")'></p>\
    </form>\
  </body>\
  <script>\
    function httpGet(theUrl)\
    {   var xmlHttp = new XMLHttpRequest();\
        xmlHttp.open( 'GET', theUrl, true );\
        xmlHttp.send( null );\
        document.getElementById('con_bt').innerHTML = 'Connecting...<br /> To continue please visit <a href=\"http://samar.hostoi.com\">Raspberry PI</a> on your <b>Home Network</b>';}\
  </script>\
</html>",RouterConfig.ssid.c_str());
    client.print(temp);
    client.flush();
  }
  else if (req.indexOf("/start") != -1)
  {
        setup_done = true;
  }
  else
  {
    int n = WiFi.scanNetworks();
    String ssid_temp;
    ssid_temp = "";
    for(i=0; i<n;i++)
    {
        ssid_temp += "<option value=\""+WiFi.SSID(i)+"\">"+WiFi.SSID(i)+"</option>";
    }
    char temp[2000];
    snprintf ( temp, 2000,
"<html>\
  <head>\
    <title>IOT Connect</title>\
  <meta name='viewport' content='width=device-width, initial-scale=1'>\
    <style>\
      body { background-color: #cccccc; font-family: Arial, Helvetica, Sans-Serif; Color: #000088; }\
    </style>\
  </head>\
  <body>\
    <h1>IOT Connect</h1>\
    <form action='setup' method='get'>\
    <p>SSID: <select name='ssid'>%s</select></p>\
    <p>Password: <input type='password' name='password' /></p>\
    <p><input type='submit' value='Next' /></p>\
    </form>\
  </body>\
</html>",ssid_temp.c_str());
    client.print(temp);
    client.flush();
  }
  delay(1);
}
void con()
{
  sendDisplay("Status","Router: "+RouterConfig.ssid);
  WiFi.softAPdisconnect(true);
  WiFi.mode(WIFI_STA);
  WiFi.begin(RouterConfig.ssid.c_str(), RouterConfig.password.c_str());
  i = 0;
  while (WiFi.status() != WL_CONNECTED && i++ < 20) delay(500);
  if(WiFi.status() == WL_CONNECTED)
  {
    set=true;
    sendDisplay("Status","Connected");
    if(!eepromDone)
    {  
      sendDisplay("EEPROM","Writing");
      delay(1000);
      EEPROM.put(0, RouterConfig);
      EEPROM.commit();
      eepromDone = true;
    }
  }
  else
  {
    sendDisplay("Status","Failed");
    if(attempts == 4)
      hard_reset = true;
    else
      hard_reset = false;
    mod_reset();
    return;
  }
}
void mod_reset()
{
  if(hard_reset || WiFi.status() != WL_CONNECTED)
  {
    ESP.restart();
  }
  else
  {
    attempts++;
    socket_setup = false;
    delay(1000);
  }
}
void checkReset()
{
  if(digitalRead(reset_btn) == HIGH)
  {
    sendDisplay("Status","Resetting...");
    delay(1000);
    for(int j=0; j<512; j++)
    {
      EEPROM.write(j,255);
    }
    EEPROM.commit();
    ESP.restart();
  }
}
bool comp(const char *val1,const char *val2)
{
  bool res = true;
  if(strlen(val1) != strlen(val2))
    res = false;
  for(int y=0;y<strlen(val1);y++)
  {
    if(val1[y] != val2[y])
      res = false;
  }
  return res;
}
void sendDisplay(String head,String body)
{
    StaticJsonBuffer<200> jsonBuffer;
    JsonObject& root = jsonBuffer.createObject();
    root["action"] = "display";
    root["head"] = head;
    root["body"] = body;
    root.printTo(Serial);
    Serial.println();
}

