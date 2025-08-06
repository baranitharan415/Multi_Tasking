#ifndef send_fail_H
#define send_fail_H

// #include <Arduino.h>
#include <employee.h>
#include <SPIFFS.h>
long file_count = 0;

void failed(String str)
{
    String file_name = "/" + String(file_count++) + ".json";

    File file = SPIFFS.open(file_name, FILE_WRITE);

    file.print(str);
    file.close();
    Serial.println("File stored");
}

void Send()
{

    File root = SPIFFS.open("/");
    if (!root || !root.isDirectory())
    {
        Serial.println("Failed to open SPIFFS root");
        return;
    }
    File file = root.openNextFile();
    while (file)
    {
        String file_name = "/" + String(file.name());
        String data = file.readString();
        file.close();
        if (json(data) != 200)
        {
            failed(data);
            break;
        }
        Serial.println(file_name);
        if (SPIFFS.exists(file_name))
            SPIFFS.remove(file_name);
        file = root.openNextFile();
        vTaskDelay(1000 / portTICK_PERIOD_MS);
    }
}

// void remove()
// {
//     File root = SPIFFS.open("/");
//     File file = root.openNextFile();
//     while (file)
//     {
//         String file_name = "/" + String(file.name());
//         Serial.println(file_name);
//         file.close();
//         size_t fsize = file.size();
//         if (fsize == 0)
//         {
//             if (SPIFFS.exists(file_name))
//             {
//                 if (SPIFFS.remove(file_name))
//                 Serial.println("Removed Succesfully");
//             }
//         }
//         file = root.openNextFile();
//     }
// }
#endif