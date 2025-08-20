#ifndef employee_H
#define employee_H

#include <Arduino.h>
#include <WiFi.h>
#include <Arduinojson.h>
#include <HTTPClient.h>
#include <send_fail.h>
#include <time.h>
#include <Adafruit_PN532.h>

String status;
int work = 0;


int sck = 18,cs =5, mosi = 23, miso = 19;


Adafruit_PN532 nfc (sck,miso,mosi,cs);
// JsonDocument doc;
bool allocate;
String Allocate_name,Allocate_id;

struct account
{
    String name, password;
}occ;

String serverUrl, id;
bool employee_allocate = false;
long last_millis = 0;

char in_time[30];
char out_time[30];

const char *ntpServer = "pool.ntp.org";
const long gmtOffset_sec = 19800;
const int daylightOffset_sec = 0;

int json(String str);
void failed(String str);
void Send();
String Read()
{
    String str;
    while (1)
    {
        if (Serial.available())
        {
            str = Serial.readStringUntil('\n');
            break;
        }
        vTaskDelay(1);
    } 
    Serial.println(str);
    return str;
}

String card_read(uint8_t *arr,uint8_t n )
{
  String id="";
  for(byte i = 0;i<n;i++)
  {
    if(arr[i]<0x10)
    id+="0";
    id += String(arr[i], HEX);
  }
  Serial.println(id);
  return id;
}

String assign()
{
  Serial.println("Please Tap a Card");
  while(1)
  {
    uint8_t arr[7],length;
    if(nfc.readPassiveTargetID(PN532_MIFARE_ISO14443A,arr,&length))
    {
    String id = card_read(arr,length);
    return id;
    }
    vTaskDelay(100);
  }
}


void convert()
{
    JsonDocument docum;
    docum["employeeId"] = Allocate_id;
    docum["machineId"] = millis()%20+1;
    docum["startingTime"] = in_time;
    docum["endingTime"] = out_time;
    // docum["Employee Name"] = employee[id.toInt()];
    // docum["Working Hours"] = values;
    String str;
    serializeJson(docum, str);
    Serial.println(str);
    int i = json(str);
    // if (json(str) != 200)
    //     failed(str);
    // else
    //     Send();
}

void start()
{
    Serial.println("Shift time started");
    struct tm timeinfo;
    if (getLocalTime(&timeinfo))
    {
        getLocalTime(&timeinfo);
        strftime(in_time, sizeof(in_time), "%Y-%m-%dT%H:%M:%S.000Z", &timeinfo);
    }
    else
    {
        Serial.println("Time Getting failed");
    }
}
int json(String str)
{
    HTTPClient http_s;
    http_s.begin("http://192.168.0.45:5000/api/Employee");
    // http_s.begin("http://192.168.1.38:8094/hms-rest-api/api/employees");
    // http://192.168.1.38:8094/hms-rest-api/api/data
    http_s.addHeader("Content-Type", "application/json");
    int httpResponseCode = http_s.POST(str);
    // Serial.println(httpResponseCode);
    http_s.end();
    return httpResponseCode;
}

void stop()
{
    Serial.println("Shift time ended");
    struct tm timeinfo;
    if (getLocalTime(&timeinfo))
    {
        strftime(out_time, sizeof(out_time), "%Y-%m-%dT%H:%M:%S.000Z", &timeinfo);
    }
    else
    {
        Serial.println("Failed to get time");
    }
    convert();
    Serial.print("In Time : ");
    Serial.println(in_time);
    Serial.print("Out Time : ");
    Serial.println(out_time);
}
bool verify()
{
  String user,password;
  EEPROM.get(50,occ);
  while(1)
  {
  Serial.print("Enter Your UserName : ");
  user = Read();
  Serial.print("Enter Your Password : ");
  password = Read();
  Serial.println(occ.name+"===>"+occ.password);
  if(occ.name == user && occ.password == password)
  {
    Serial.println("Verification Complete");
    return true;
  }
  Serial.println("User Name or Password Wrong");
  }
  return false;
}

void e_allocate()
{
    Serial.print("Enter Your Employee id : ");
    id = Read();
    Serial.println(id);
    Serial.println("Type \"in\" to Start to work\nType \"out\" to stop to work");
    employee_allocate = true;
}


void check(String RF_id)
{
  
  File root = SPIFFS.open("/");
  File file = root.openNextFile();
  while (file) 
  {
    JsonDocument doc;
    if (!file.isDirectory()) 
    {
      doc.clear();
      deserializeJson(doc,file);
      if(doc["id"].as<String>() == RF_id)
      {
        Allocate_name =  doc["name"].as <String>();
        Allocate_id = doc["id"].as <String>();
        Serial.println("Employee Name : " + Allocate_name);  
        start();
        break;
      }
    }
    file = root.openNextFile();
  }
  if(!file)
    Serial.println("UnAuthorised Card");
  else
    allocate = !allocate;
  file.close();
}



void Add_new(String RF_id, String E_name)
{
  JsonDocument doc;
  doc.clear();
  // String RF_id= assign();
  doc["id"] = RF_id;
  // String E_name ;
  // Serial.print("Enter the Name ");
  // E_name = Read();
  doc["name"] = E_name;
  File file = SPIFFS.open("/"+RF_id+".json",FILE_WRITE);
  if(serializeJson(doc,file))
  Serial.println("File Stored");
  Serial.println(file.name());
  file.close();
}

String pwd_set()
{
  String password,temp = "Hi";
  while(temp!= password)
  { 
  Serial.print("Create a Password : ");
  password = Read();
  Serial.print("re Enter your Password : ");
  temp = Read();
  }
  return password;
}

void files()
{
  File root = SPIFFS.open("/");
  File file = root.openNextFile();
  while(file)
  {
    // Serial.println(file.name());
    String rm =  file.name();
    if(file.isDirectory())
    Serial.println(file.name());
    rm = "/"+ rm ;
    Serial.println(rm);
    // Serial.println(file.readString());
    
    file.close();
    // SPIFFS.remove(rm);
    file = root.openNextFile();
  }
  Serial.println("If You need Delete file Press 1");
  if(Read() == "1")
  {
    Serial.println("Enter the File Name");
    SPIFFS.remove(Read());
    files();
  }
  //  if (SPIFFS.format()) {
  //   Serial.println("All files deleted (SPIFFS formatted)");
  // } else {
  //   Serial.println("Format failed!");
  // }

}

void choice()
{
  while(1)
  {

    Serial.println("1==>Change Admin Password ");
    Serial.println("2==>List Files");
    Serial.println("3==>Continue");
    String val;
    val=Read();
    if(val == "1")
    {
      if(verify())
      {  
      occ.password=pwd_set();
      EEPROM.put(50,occ);
      EEPROM.commit();
      }
    }
    if(val == "2")
    {
      files();
    }
    if(val == "3")
    break;
    // if(val == "4")
    // files();
    
  }

}


void initial()
{
  Serial.print("Enter Your Name : ");
  occ.name = Read();
  occ.password=pwd_set();
  EEPROM.put(50,occ);
  EEPROM.commit();
}



void admin()
{
    // EEPROM.write(50,255);
    // EEPROM.commit();
    if(EEPROM.read(50)==255)
    {
      Serial.println("Welcome to Our Page");
      initial();
    }
    choice();
}


void attendance(void *para)
{

    nfc.begin();
    nfc.SAMConfig();   
    SPIFFS.begin(true);
    admin();

    configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);

    while (1)
    {
        uint8_t uid[7];
        uint8_t uidLength;
        if (!nfc.readPassiveTargetID(PN532_MIFARE_ISO14443A, uid, &uidLength))
            return;
        if (!allocate)
        {
            String id = card_read(uid, uidLength);
            check(id);
            last_millis = millis();
        }
        else if (allocate && card_read(uid, uidLength) == Allocate_id )
        {
            if(millis()-last_millis>5000)
            {
            Serial.println(Allocate_name + " Shift time is over");
            allocate = !allocate;
            stop();
            last_millis=millis();
            }
        }
        else
            Serial.println("Already Employee " + Allocate_name + " Assigned");
        vTaskDelay(1000 / portTICK_PERIOD_MS);
    }
}

void create_task()
{
    xTaskCreate(
        attendance,
        "attendance",
        5000,
        NULL,
        1,
        NULL);
}
#endif