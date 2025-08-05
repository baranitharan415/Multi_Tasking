#include <Arduino.h>
#include <WiFi.h>
#include <Arduinojson.h>
#include <HTTPClient.h>
#include <BluetoothSerial.h>

String status;
BluetoothSerial SerialBT;
int work = 0;

JsonDocument docum;
String serverUrl, id;
bool employee_allocate = false;
long last_millis = 0;

String employee[] = {"Ram", "Suresh", "Ramesh", "Dinesh", "Kanish", "Ramesh", "Rajesh", "Harish", "Jithesh", "Pranesh"};

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
    sprintf(values, "%02d:%02d:%02d", hours, minutes, seconds);
    docum["Loom No"] = "5";
    docum["Employee id"] = id;
    docum["Employee Name"] = employee[id.toInt()];
    docum["Working Hours"] = values;
}


void start()
{
    Serial.println("Shift time started");
    SerialBT.println("Shift time started");
}

void json()
{
    HTTPClient http_s;
    // http_s.begin(serverUrl);
    http_s.begin("http://192.168.0.45:5000/api/data");
    http_s.addHeader("Content-Type", "application/json");
    String jsonString;
    serializeJson(docum, jsonString);
    int httpResponseCode = http_s.POST(jsonString);
    SerialBT.println(httpResponseCode);
    http_s.end();
}

void stop()
{
    Serial.println("Shift time ended");
    SerialBT.println("Shift time ended");
    convert();
    json();
    employee_allocate = false;
    work = 0;
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
