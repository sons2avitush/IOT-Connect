#include <LiquidCrystal.h>
#include <ArduinoJson.h>
#include <EEPROM.h>

// initialize the library with the numbers of the interface pins
LiquidCrystal lcd(12, 11, 5, 4, 3, 2);

struct sta{
  bool rel1;
  bool rel2;
  byte bkled;
};

const char *cop, *head, *body;
String com;
bool stat;
#define relay1 7
#define relay2 8
#define led 6

byte eepromVal;
sta gStatus;
sta cStatus = {HIGH,HIGH,100};

void setup() {
  // put your setup code here, to run once:
  Serial.begin(9600);
  EEPROM.get( 0, gStatus);
  if(gStatus.bkled == 255)
  {
    EEPROM.put(0, cStatus);
  }
  else
  {
    cStatus = gStatus;
  }
  lcd.begin(16, 2);
  pinMode(relay1, OUTPUT);
  pinMode(relay2, OUTPUT);
  pinMode(led, OUTPUT);
  digitalWrite(relay1,cStatus.rel1);
  digitalWrite(relay2,cStatus.rel2);
  analogWrite(led,cStatus.bkled);
}

void loop() {
  // put your main code here, to run repeatedly:
  com = "";
  if (Serial.available()) {
     delay(100);
     com = Serial.readStringUntil('\n');
     Serial.flush();
     StaticJsonBuffer<200> jsonBuffer;
     JsonObject& root = jsonBuffer.parseObject(com);
     if (root.success()) 
     {
       cop = root["action"];
       if(comp(cop,"RELAY1"))
       {
        if(root["value"] == true)
          cStatus.rel1 = HIGH;
        else if(root["value"] == false)
          cStatus.rel1 = LOW;
        showStatus();
       }
       else if(comp(cop,"RELAY2"))
       {
          if(root["value"] == true)
            cStatus.rel2 = HIGH;
          else if(root["value"] == false)
            cStatus.rel2 = LOW;
          showStatus();
       }
       else if(comp(cop,"BKLED"))
       {
          cStatus.bkled = root["value"];
          showStatus();
       }
       else if(comp(cop,"display"))
       {
          lcd.clear();
          lcd.setCursor(0,0);
          head = root["head"];
          body = root["body"];
          lcd.print(head);
          lcd.setCursor(0,1);
          lcd.print(body);
       }
       else if(comp(cop,"STATUS"))
       {
        showStatus();
       }
       EEPROM.put(0,cStatus);
       digitalWrite(relay1,cStatus.rel1);
       digitalWrite(relay2,cStatus.rel2);
       analogWrite(led,cStatus.bkled);
     }
  }
  delay(10);
}
void showStatus()
{
  lcd.clear();
  if(cStatus.rel1 == HIGH)
  {
    lcd.setCursor(0,0);
    lcd.print("RELAY1: OFF");
  }
  if(cStatus.rel1 == LOW)
  {
    lcd.setCursor(0,0);
    lcd.print("RELAY1: ON");
  }
  if(cStatus.rel2 == HIGH)
  {
    lcd.setCursor(0,1);
    lcd.print("RELAY2: OFF");

  }
  if(cStatus.rel2 == LOW)
  {
    lcd.setCursor(0,1);
    lcd.print("RELAY2: ON");
  }
  StaticJsonBuffer<200> jsonBuffer;
  JsonObject& root = jsonBuffer.createObject();
  root["action"] = "status";
  root["relay1"] = cStatus.rel1;
  root["relay2"] = cStatus.rel2;
  root["bkled"] = cStatus.bkled;
  root.printTo(Serial);
  Serial.println();
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
