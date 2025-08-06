// #include <WiFi.h>
#include <EEPROM.h>
// #include <Arduino.h>
// #include <ArduinoJson.h>
#include <PubSubClient.h>
// #include <HTTPClient.h>
#include <SPIFFS.h>
#include <send_fail.h>
#include "employee.h"


HardwareSerial NewSerial(2);

HTTPClient http;
String input;
String key, value;
String ssid;
String pwd;
String ms;
String client_name;
String topic[] = {"PICKS", "WARP", "FILL", "OTHER", "TOTAL", "SHIFT", "RUN", "EFF", "STATE", "CAUSE"};
JsonDocument doc;

String pubserverUrl;
int t;
int picks=0, warp, fill, other, total, state, cause,shift;
float eff,run;
int minutes,hours,seconds;
// const char* pubserverUrl = ""; 
unsigned long previousMillis = 0;  
const long interval = 15000;  



WiFiClient espClient;
PubSubClient client(espClient);

struct change
{
  String E_ssid, E_pwd, E_ip,E_client_name;
};

change acc;

String line()
{
  String str;
  while (1)
  {
    if (Serial.available())
    {
      str = Serial.readString();
      break;
    }
    delay(100);
  }
  return str;
}







void arrange()
{
  EEPROM.get(0, acc);
  ms = acc.E_ip;
  ssid = acc.E_ssid;
  pwd = acc.E_pwd;
  client_name=acc.E_client_name;
}

void wifi()
{
  WiFi.disconnect(true);
  Serial.print("Enter Your Hotspot name : ");
  ssid = line();
  Serial.println(ssid);
  Serial.print("Enter You Password : ");
  pwd = line();
  Serial.println(pwd);
  WiFi.begin(ssid, pwd);
}

void set()
{
  acc.E_ssid = ssid;
  acc.E_pwd = pwd;
  acc.E_ip = ms;
  acc.E_client_name=client_name;
  EEPROM.put(0, acc);
  EEPROM.commit();
}

void server()
{
  Serial.print("Enter Your Broker IP : ");
  ms = line();
  Serial.println(ms);
  Serial.print("Enter Your Name : ");
  client_name = line();
  Serial.println(client_name);
}

void EEP_value()
{

  Serial.println("-----------------------------------------------------------------------------");
  if (EEPROM.read(0) == 255)
  {
    Serial.println("EEPROM is empty please enter the all value manually");
    wifi();
    server();
    set();
    Serial.println("-----------------------------------------------------------------------------");
  }

  EEPROM.get(0, acc);

  Serial.println("Hotspot name : " + acc.E_ssid);
  Serial.println("Password : " + acc.E_pwd);
  Serial.println("Borker IP : " + acc.E_ip);
}


void cloud_upload(String str,int v_values)
{
  doc[str]=v_values;
}

void cloud_uploadf(String str,float v_values)
{
  doc[str]=v_values;
}


void pref()
{
  String choice;
  Serial.println("If you want change Value Press: 1\nDon't wanna change Press any key");
  choice = line();
  if (choice == "1")
  {
    wifi();
    server();
    set();
  }
  else
    arrange();
  return;
}

char * topic_val(String top)
{
  static char values[20];
  if(top == "PICKS")
  {
  cloud_upload(top,picks);
  
  sprintf(values,"%d",picks);
  }

  if(top== "WARP")
  {
  cloud_upload(top,warp);
  sprintf(values,"%d",warp);
  }
  if(top=="FILL")
  {
  cloud_upload(top,fill);
  sprintf(values,"%d",fill);
  }

  if(top=="OTHER")
  {
  cloud_upload(top,other);
  sprintf(values,"%d",other);
  }
  
  if(top=="TOTAL")
  {
  cloud_upload(top,total);
  sprintf(values,"%d",total);
  }
  
  if(top=="SHIFT")
  {
    // cloud_upload(top,shift);
    if(shift>=60)
    {
      seconds = shift % 60;
      minutes = shift/60;

      if(minutes>=60)
      {
        minutes = shift%3600;
        hours = hours /3600;
      }
    }
    else
    {
      seconds=shift;
    }
    sprintf(values,"%02d:%02d:%02d",hours,minutes,seconds);
    doc[top]=values;
    // dtostrf(shift,6,2,values);
  }
  
  
  if(top== "RUN")
  {
    cloud_uploadf(top,run);
    run/=3600;
    dtostrf(run,6,2,values);
  }

  
  if(top=="EFF")
{
  cloud_uploadf(top,eff);
  dtostrf(eff,6,2,values);
}
  
  if(top=="STATE")
  {
  cloud_upload(top,state);
  sprintf(values,"%d",state);
    }
  
  if(top=="CAUSE")
  {
    cloud_upload(top,cause);
    sprintf(values,"%d",cause);
  }
  return values;
}


void connect() {
  while (!client.connected())
  client.connect(client_name.c_str(), "bharani", "1234");
  // for(int i=0;i<10;i++)
  // client.subscribe(topic[i].c_str());
  Serial.println("Client connected");
}

void callback(char* topic, byte* payload, unsigned int length) {
  Serial.print("Message arrived [");
  Serial.print(topic);
  Serial.print("] ");
  for (int i = 0; i < length; i++) {
    Serial.print((char)payload[i]);
  }
  Serial.println();
}




void store(String data)
{
  int sep1, sep2;
  int startIndex = 0;
  String key, value;

  while (true)
  {
    sep1 = data.indexOf(':', startIndex);
    sep2 = data.indexOf(',', startIndex);
    if (sep2 == -1)
      sep2 = data.length();

    key = data.substring(startIndex, sep1);
    value = data.substring(sep1 + 1, sep2);
   

    if (key == "PICKS")
      picks = value.toInt();
    else if (key == "WARP")
      warp = value.toInt();
    else if (key == "FILL")
      fill = value.toInt();
    else if (key == "OTHER")
      other = value.toInt();
    else if (key == "TOTAL")
      total = value.toInt();
    else if (key == "SHIFT")
      shift = value.toFloat();
    else if (key == "RUN")
      run = value.toFloat();
    else if (key == "EFF")
      eff = value.toFloat();
    else if (key == "STATE")
      state = value.toInt();
    else if (key == "CAUSE")
      cause = value.toInt();

    if (sep2 >= data.length())
      break;

    startIndex = sep2 + 1;
  }
}









void setup() 
{
  Serial.begin(57600);
  create_task();
  NewSerial.begin(115200,SERIAL_8N1,16,17);
  SPIFFS.begin();
  EEPROM.begin(512);
  WiFi.mode(WIFI_STA);
  EEP_value();
  pref();
  EEPROM.get(0,acc);
  WiFi.begin(acc.E_ssid.c_str(),acc.E_pwd.c_str());
  while (WiFi.status() != WL_CONNECTED) 
  {
    Serial.print(".");
    delay(2000);
    t++;
    if (t == 5) 
    {
      Serial.println();
      wifi();
      t = 0;
    }
  }

  Serial.println("Connected");
  Serial.print("IP address : ");
  Serial.println(WiFi.localIP());
  set();

  // Serial.print("\nPlease Enter the Server URL : ");
  
  // pubserverUrl = line();
  // Serial.println(pubserverUrl);
  

  // // http.end();



  // Serial.println();
  
  client.setServer(ms.c_str(), 1883);
  client.setCallback(callback);
}



void loop()
{
    if (!client.connected())
    connect();
    if(NewSerial.available())
    {
    String input;
    while(1)
    {
      input =NewSerial.readStringUntil('\n');
      if(input.length()>0)
      break;
    }
    store(input);
    }
  String val;
 
  for(int i =0;i<10;i++)
  {
    val=topic_val(topic[i]);
    client.publish(topic[i].c_str(), val.c_str());
  }
  if(millis()-previousMillis >= interval)
  {
      previousMillis=millis();
      String jsonString;
      serializeJson(doc, jsonString);
      int response = json(jsonString);
      if(response!=200)
      {
        failed(jsonString);
      }
      else
        Send();
  }

  client.loop();
  delay(1000);
}
