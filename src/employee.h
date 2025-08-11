#ifndef employee_H
#define employee_H

#include <Arduino.h>
#include <WiFi.h>
#include <Arduinojson.h>
#include <HTTPClient.h>
#include <BluetoothSerial.h>
#include <send_fail.h>
#include <time.h>


String status;
BluetoothSerial SerialBT;
int work = 0;

String serverUrl, id;
bool employee_allocate = false;
long last_millis = 0;

String employee[] = {"Ram", "Suresh", "Ramesh", "Dinesh", "Kanish", "Ramesh", "Rajesh", "Harish", "Jithesh", "Pranesh"};
char in_time[30];
char out_time[30];



const char* ntpServer = "pool.ntp.org";
const long gmtOffset_sec = 19800;   
const int daylightOffset_sec = 0;


int json(String str);
void failed(String str);
void Send();
String Read()
{
    Serial.begin(57600);
    String str;
    while (1)
    {
        if (SerialBT.available())
        {
            str = SerialBT.readString();
            str.trim();
            break;
        }
        vTaskDelay(1);
    }
    return str;
}

void btconnect()
{
    while (1)
    {
        if (SerialBT.hasClient())
            break;
        vTaskDelay(10 / portTICK_PERIOD_MS);
    }
}

void convert()
{
    int minutes = 0, hours = 0, seconds = 0;
    char values[20];
    if (work >= 60)
    {
        seconds = work % 60;
        minutes = work / 60;

        if (minutes >= 60)
        {
            minutes = work % 3600;
            hours = hours / 3600;
        }
    }
    else
    {
        seconds = work;
    }
    JsonDocument docum;
    sprintf(values, "%02d:%02d:%02d", hours, minutes, seconds);
    docum["employeeId"] = id;
    docum["machineId"] = 5;
    docum["startingTime"] = in_time;
    docum["endingTime"] = out_time;
    // docum["Employee Name"] = employee[id.toInt()];
    // docum["Working Hours"] = values;
    String str;
    serializeJson(docum, str);
    Serial.println(str);
    SerialBT.println(json(str));
    // if (json(str) != 200)
        // failed(str);
    // else
        // Send();
}

void start()
{
    Serial.println("Shift time started");
    SerialBT.println("Shift time started");
    struct tm timeinfo;
    if (getLocalTime(&timeinfo))
    {
    getLocalTime(&timeinfo);   
    strftime(in_time, sizeof(in_time), "%Y-%m-%dT%H:%M:%S.000Z", &timeinfo);
    }
    else
    {
        SerialBT.println("Time Getting failed");
    }

}
int json(String str)
{
    HTTPClient http_s;
    // http_s.begin(serverUrl);
    http_s.begin("http://192.168.1.38:8094/hms-rest-api/api/employees");
    // http://192.168.1.38:8094/hms-rest-api/api/data
    http_s.addHeader("Content-Type", "application/json");
    int httpResponseCode = http_s.POST(str);
    SerialBT.println(httpResponseCode);
    Serial.println(httpResponseCode);
    http_s.end();
    return httpResponseCode;
}

void stop()
{  
    Serial.println("Shift time ended");
    SerialBT.println("Shift time ended");
    employee_allocate = false;
    work = 0;
      struct tm timeinfo;
    if (getLocalTime(&timeinfo)) {
         strftime(out_time, sizeof(out_time), "%Y-%m-%dT%H:%M:%S.000Z", &timeinfo);
    } else {
        Serial.println("Failed to get time");
    }
    convert();
    SerialBT.print("In Time : ");
    SerialBT.println(in_time);
    SerialBT.print("Out Time : ");
    SerialBT.println(out_time);
}

void e_allocate()
{
    SerialBT.print("Enter Your Employee id : ");
    id = Read();
    SerialBT.println(id);
    SerialBT.println("Type \"in\" to Start to work\nType \"out\" to stop to work");
    employee_allocate = true;
}

void check(String str)
{
    if (str == "in")
        start();
    if (str == "out")
        stop();
}

void attendance(void *para)
{
    SerialBT.begin("KB_415");
    // WiFi.mode(WIFI_STA);
    // WiFi.begin("DC-R&D", "India@123");
    // while (WiFi.status() != WL_CONNECTED)
    //     ;
    // Serial.print("Enter ServerUrl : ");
    // url();
    btconnect();
     configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);

    while (1)
    {
        long current_millis = millis();
        if (!employee_allocate)
        {
            e_allocate();
        }
        if (SerialBT.available())
        {
            status = Read();
            check(status);
        }
        if (current_millis - last_millis >= 1000)
        {
            if (status == "in")
                work++;
            last_millis = current_millis;
        }
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