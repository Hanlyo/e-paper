/*****************************************************************************
* | File        :   EPD_7in3f_test.c
* | Author      :   Waveshare team
* | Function    :   7.3inch e-Paper (F) Demo
* | Info        :
*----------------
* | This version:   V1.0
* | Date        :   2022-10-20
* | Info        :
* -----------------------------------------------------------------------------
#
# Permission is hereby granted, free of charge, to any person obtaining a copy
# of this software and associated documnetation files (the "Software"), to deal
# in the Software without restriction, including without limitation the rights
# to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
# copies of the Software, and to permit persons to  whom the Software is
# furished to do so, subject to the following conditions:
#
# The above copyright notice and this permission notice shall be included in
# all copies or substantial portions of the Software.
#
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
# IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
# FITNESS OR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
# AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
# LIABILITY WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
# OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
# THE SOFTWARE.
#
******************************************************************************/
#include "EPD_Test.h"
#include "EPD_7in3f.h"
#include "time.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <curl/curl.h>
#include <cjson/cJSON.h>


#define URL_FORMAT "https://api.openweathermap.org/data/2.5/weather?lat=51.511532&lon=7.093030&appid=%s&units=metric&lang=de"
#define FORECAST_URL_FORMAT "https://api.openweathermap.org/data/3.0/onecall?lat=51.511532&lon=7.093030&exclude=minutely,hourly&appid=%s&units=metric&lang=de"

struct MemoryStruct {
    char *memory;
    size_t size;
};


#define MAX_KEY_LENGTH 256  // Maximale Länge des API-Keys

char *getApiKey() {
    FILE *file = fopen("apikey.txt", "r");
    if (file == NULL) {
        perror("Fehler beim Öffnen der Datei");
        return NULL;
    }

    // API-Key aus der Datei lesen
    char *apiKey = malloc(MAX_KEY_LENGTH);
    if (fgets(apiKey, MAX_KEY_LENGTH, file) == NULL) {
        perror("Fehler beim Lesen der Datei");
        fclose(file);
        return NULL;
    }

    // Datei schließen
    fclose(file);

    // Entferne das Zeilenende (falls vorhanden)
    apiKey[strcspn(apiKey, "\n")] = 0;

    // API-Key ausgeben
    printf("API-Key: %s\n", apiKey);

    return apiKey;
}




static size_t WriteMemoryCallback(void *contents, size_t size, size_t nmemb, void *userp) {
    size_t realsize = size * nmemb;
    struct MemoryStruct *mem = (struct MemoryStruct *)userp;

    char *ptr = realloc(mem->memory, mem->size + realsize + 1);
    if (ptr == NULL) return 0;

    mem->memory = ptr;
    memcpy(&(mem->memory[mem->size]), contents, realsize);
    mem->size += realsize;
    mem->memory[mem->size] = 0;

    return realsize;
}

char *fetch_weather_data(const char *api_key) {
    CURL *curl;
    CURLcode res;
    struct MemoryStruct chunk;

    chunk.memory = malloc(1);
    chunk.size = 0;

    char url[256];
    snprintf(url, sizeof(url), URL_FORMAT, api_key);

    curl_global_init(CURL_GLOBAL_ALL);
    curl = curl_easy_init();

    if (curl) {
        curl_easy_setopt(curl, CURLOPT_URL, url);
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteMemoryCallback);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void *)&chunk);

        res = curl_easy_perform(curl);
        if (res != CURLE_OK) {
            fprintf(stderr, "cURL Fehler: %s\n", curl_easy_strerror(res));
            free(chunk.memory);
            chunk.memory = NULL;
        }

        curl_easy_cleanup(curl);
    }

    curl_global_cleanup();
    return chunk.memory;
}

double get_temperature(const char *json) {
    cJSON *root = cJSON_Parse(json);
    if (root == NULL) {
        fprintf(stderr, "Fehler beim Parsen der JSON-Daten\n");
        return -999.0;  // Fehlerwert
    }

    double temp = -999.0;
    cJSON *main = cJSON_GetObjectItem(root, "main");
    if (main) {
        cJSON *temp_json = cJSON_GetObjectItem(main, "temp");
        if (temp_json) {
            temp = temp_json->valuedouble;
        }
    }

    cJSON_Delete(root);
    return temp;
}

char *get_weather_description(const char *json) {
    cJSON *root = cJSON_Parse(json);
    if (root == NULL) {
        fprintf(stderr, "Fehler beim Parsen der JSON-Daten\n");
        return NULL;
    }

    char *description = NULL;
    cJSON *weather = cJSON_GetObjectItem(root, "weather");
    if (cJSON_IsArray(weather) && cJSON_GetArraySize(weather) > 0) {
        cJSON *weather_item = cJSON_GetArrayItem(weather, 0);
        if (weather_item) {
            cJSON *desc = cJSON_GetObjectItem(weather_item, "description");
            if (desc) {
                description = strdup(desc->valuestring);
            }
        }
    }

    cJSON_Delete(root);
    return description;
}

double get_wind_speed(const char *json) {
    cJSON *root = cJSON_Parse(json);
    if (root == NULL) {
        fprintf(stderr, "Fehler beim Parsen der JSON-Daten\n");
        return -999.0;  // Fehlerwert
    }

    double speed = -999.0;
    cJSON *wind = cJSON_GetObjectItem(root, "wind");
    if (wind) {
        cJSON *speed_json = cJSON_GetObjectItem(wind, "speed");
        if (speed_json) {
            speed = speed_json->valuedouble;
        }
    }

    cJSON_Delete(root);
    return speed;
}


int getFirstWeekdayOfMonth() {
    time_t t = time(NULL);
    struct tm *now = localtime(&t);

    // Ersten Tag des Monats setzen
    now->tm_mday = 1;

    // Normalisieren (mktime berechnet die Wochentagsnummer)
    mktime(now);

    printf("%d\n", now->tm_wday);

    return now->tm_wday;
}

int getCurrentDayOfMonth() {
    time_t t = time(NULL);
    struct tm *now = localtime(&t);

    // Ersten Tag des Monats setzen
    return now->tm_mday;
}



int aei(void)
{
    // char *json = fetch_weather_data(API_KEY);
    // if (json == NULL) {
    //     fprintf(stderr, "Fehler beim Abrufen der Wetterdaten\n");
    //     return 1;
    // }

    // double temp = get_temperature(json);
    // char *description = get_weather_description(json);
    // double windSpeed = get_wind_speed(json);

    // printf("Temperatur: %.2f°C\n", temp);
    // printf("Wetter: %s\n", description ? description : "Unbekannt");
    // printf("Windgeschwindigkeit: %.2f m/s\n", windSpeed);


    // static char strTemp[52];  // Puffer für die Zeichenkette
    // sprintf(strTemp, ".2%f", temp);
    // static char strDescription[52];  // Puffer für die Zeichenkette
    // sprintf(strDescription, "%s", description);
    // static char strWindSpeed[52];  // Puffer für die Zeichenkette
    // sprintf(strWindSpeed, ".2%f", windSpeed);

    // free(json);
    // free(description);
}

///////////////////////////////////////////////////////////////////


char *fetch_forecast_data(const char *api_key) {
    CURL *curl;
    CURLcode res;
    struct MemoryStruct chunk;

    chunk.memory = malloc(1);
    chunk.size = 0;

    char url[256];
    snprintf(url, sizeof(url), FORECAST_URL_FORMAT, api_key);

    curl_global_init(CURL_GLOBAL_ALL);
    curl = curl_easy_init();

    if (curl) {
        curl_easy_setopt(curl, CURLOPT_URL, url);
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteMemoryCallback);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void *)&chunk);

        res = curl_easy_perform(curl);
        if (res != CURLE_OK) {
            fprintf(stderr, "cURL Fehler: %s\n", curl_easy_strerror(res));
            free(chunk.memory);
            chunk.memory = NULL;
        }

        curl_easy_cleanup(curl);
    }

    curl_global_cleanup();
    return chunk.memory;
}


// Funktion zum Extrahieren eines JSON-Arrays aus einem JSON-String
cJSON *extract_json_array(const char *json_str, const char *array_key) {
    cJSON *root = cJSON_Parse(json_str);
    if (root == NULL) {
        fprintf(stderr, "Fehler beim Parsen des JSON\n");
        return NULL;
    }

    printf("root: %s\n", root);
    printf("root->valuestring: %s\n", root->valuestring);
    cJSON *json_array = cJSON_GetObjectItem(root, "daily");
    printf("json_array: %s\n", json_array);
    if (!cJSON_IsArray(json_array)) {
        fprintf(stderr, "Der Schlüssel \"%s\" enthält kein Array.\n", array_key);
        cJSON_Delete(root);
        return NULL;
    }

    return json_array; // json_array darf nicht gelöscht werden, root aber schon!
}

// Funktion zum Abrufen eines Wertes aus einem JSON-Array
cJSON *get_value_from_json_array(cJSON *json_array, int index, const char *key) {
    if (!cJSON_IsArray(json_array)) {
        fprintf(stderr, "Ungültiges JSON-Array.\n");
        return NULL;
    }

    cJSON *element = cJSON_GetArrayItem(json_array, index);
    if (!cJSON_IsObject(element)) {
        fprintf(stderr, "Das Element an Index %d ist kein JSON-Objekt.\n", index);
        return NULL;
    }

    cJSON *value = cJSON_GetObjectItem(element, key);
    if (!value) {
        fprintf(stderr, "Der Schlüssel \"%s\" existiert nicht.\n", key);
        return NULL;
    }

    return value; // Gibt ein cJSON-Objekt zurück (String, Zahl, Objekt, etc.)
}

// int getInt() {
//     double speed = -999.0;
//     cJSON *wind = cJSON_GetObjectItem(root, "wind");
//     if (wind) {
//         cJSON *speed_json = cJSON_GetObjectItem(wind, "speed");
//         if (speed_json) {
//             speed = speed_json->valuedouble;
//         }
//     }
// }

// todo getString() {
//     char *description = NULL;
//     cJSON *weather = cJSON_GetObjectItem(root, "weather");
//     if (cJSON_IsArray(weather) && cJSON_GetArraySize(weather) > 0) {
//         cJSON *weather_item = cJSON_GetArrayItem(weather, 0);
//         if (weather_item) {
//             cJSON *desc = cJSON_GetObjectItem(weather_item, "description");
//             if (desc) {
//                 description = strdup(desc->valuestring);
//             }
//         }
//     }
// }

// int[] getMinMaxTemp(const char *json, int tageInZukunft) {

// }

// int getIcon(const char *json, int tageInZukunft) {

// }




int EPD_7in3f_test(void)
{
    // printf("EPD_7IN3F_test Demo\r\n");
    // if(DEV_Module_Init()!=0){
    //     return -1;
    // }

    // printf("e-Paper Init and Clear...\r\n");
    // EPD_7IN3F_Init();

    // struct timespec start={0,0}, finish={0,0}; 
    // clock_gettime(CLOCK_REALTIME, &start);
    // EPD_7IN3F_Clear(EPD_7IN3F_WHITE); // WHITE
    // clock_gettime(CLOCK_REALTIME, &finish);
    // printf("%ld S\r\n", finish.tv_sec-start.tv_sec);    
    // DEV_Delay_ms(1000);

    // //Create a new image cache
    // UBYTE *BlackImage;
    // UDOUBLE Imagesize = ((EPD_7IN3F_WIDTH % 2 == 0)? (EPD_7IN3F_WIDTH / 2 ): (EPD_7IN3F_WIDTH / 2 + 1)) * EPD_7IN3F_HEIGHT;
    // if((BlackImage = (UBYTE *)malloc(Imagesize)) == NULL) {
    //     printf("Failed to apply for black memory...\r\n");
    //     return -1;
    // }
    // printf("Paint_NewImage\r\n");
    // Paint_NewImage(BlackImage, EPD_7IN3F_WIDTH, EPD_7IN3F_HEIGHT, 0, EPD_7IN3F_WHITE);
    // Paint_SetScale(7);

    // Paint_SetRotate(ROTATE_180);

#if 0   // show bmp
    printf("show bmp1-----------------\r\n");
    Paint_SelectImage(BlackImage);
    Paint_Clear(EPD_7IN3F_WHITE);
    GUI_ReadBmp_RGB_7Color("./myPic/initialD.bmp", 0, 0);
    EPD_7IN3F_Display(BlackImage);
    DEV_Delay_ms(3000);

    printf("show bmp2-----------------\r\n");
    Paint_SelectImage(BlackImage);
    Paint_Clear(EPD_7IN3F_WHITE);
    GUI_ReadBmp_RGB_7Color("./myPic/initialD2.bmp", 0, 0);
    EPD_7IN3F_Display(BlackImage);
    DEV_Delay_ms(3000);
    
    printf("show bmp3-----------------\r\n");
    Paint_SelectImage(BlackImage);
    Paint_Clear(EPD_7IN3F_WHITE);
    GUI_ReadBmp_RGB_7Color("./myPic/initialD3.bmp", 0, 0);
    EPD_7IN3F_Display(BlackImage);
    DEV_Delay_ms(3000);
#endif
#if 0   // Drawing on the image

    // 1.Fetching weather data
    // char *json = fetch_weather_data(API_KEY);
    // if (json == NULL) {
    //     fprintf(stderr, "Fehler beim Abrufen der Wetterdaten\n");
    //     return 1;
    // }

    // double temp = get_temperature(json);
    // char *description = get_weather_description(json);

    // static char str[52];  // Puffer für die Zeichenkette
    // sprintf(str, ".2%f°C", temp);
    // const char *temp_str = str;
    // printf("Temperatur: %s\n", temp);
    // printf("Temperatur: %s\n", str);
    // printf("Temperatur: %s\n", temp_str);

    // static char str2[52];  // Puffer für die Zeichenkette
    // sprintf(str2, "%s", description);
    // printf("Wetter: %s\n", str2 ? str2 : "Unbekannt");



    // 2.Select Image
    printf("SelectImage:BlackImage\r\n");
    Paint_SelectImage(BlackImage);
    Paint_Clear(EPD_7IN3F_WHITE);

    // 3.Drawing on the image
    printf("Drawing:BlackImage\r\n");
    
    
    // Paint_DrawString_EN(001, 0, temp_str, &Font24, EPD_7IN3F_WHITE, EPD_7IN3F_ORANGE);

    
    //Paint_DrawString_EN(400, 0, str2, &Font24, EPD_7IN3F_WHITE, EPD_7IN3F_ORANGE);

    // 4. Free resources
    // free(json);
    // free(description);


    // senkrechte Linie links
    Debug("Draw vertical line\r\n");
    Paint_DrawLine(150, 000, 150, 480, EPD_7IN3F_ORANGE, DOT_PIXEL_2X2, LINE_STYLE_SOLID);

    // horizontale Linien links
    Debug("Draw horizontal line\r\n");
    Paint_DrawLine(000, 120, 148, 120, EPD_7IN3F_RED, DOT_PIXEL_1X1, LINE_STYLE_SOLID);
    Paint_DrawLine(000, 240, 148, 240, EPD_7IN3F_BLUE, DOT_PIXEL_1X1, LINE_STYLE_SOLID);
    Paint_DrawLine(000, 360, 148, 360, EPD_7IN3F_GREEN, DOT_PIXEL_1X1, LINE_STYLE_SOLID);

    // Text und Icons links
    // 150 breit -> 75 mitte
    // font16 -> 11 Pixel breit
    // 9 Zeichen je 11 Pixel = 99 Pixel breit
    // 480 / 4 = 120 Pixel hoch
    // 70 für icon
    // 20 für text
    // 10 abstand über, unter und zwischen
    Debug("Draw icons and temperature\r\n");
    Paint_DrawRectangle(40, 10, 110, 80, EPD_7IN3F_GREEN, DOT_PIXEL_1X1, DRAW_FILL_FULL);
    Paint_DrawString_EN(21, 90, "7°C/10°C", &Font16, EPD_7IN3F_WHITE, EPD_7IN3F_BLUE);
    Paint_DrawRectangle(40, 130, 110, 200, EPD_7IN3F_RED, DOT_PIXEL_1X1, DRAW_FILL_FULL);
    Paint_DrawString_EN(21, 210, "-5°C/0°C", &Font16, EPD_7IN3F_WHITE, EPD_7IN3F_BLUE);
    Paint_DrawRectangle(40, 250, 110, 320, EPD_7IN3F_ORANGE, DOT_PIXEL_1X1, DRAW_FILL_FULL);
    Paint_DrawString_EN(21, 330, "20°C/30°C", &Font16, EPD_7IN3F_WHITE, EPD_7IN3F_RED);
    Paint_DrawRectangle(40, 370, 110, 440, EPD_7IN3F_BLUE, DOT_PIXEL_1X1, DRAW_FILL_FULL);
    Paint_DrawString_EN(21, 450, "25°C/30°C", &Font16, EPD_7IN3F_WHITE, EPD_7IN3F_RED);

    // horizontale Linie unten
    Debug("Draw horizontal line at the bottom\r\n");
    Paint_DrawLine(150, 320, 800, 320, EPD_7IN3F_ORANGE, DOT_PIXEL_2X2, LINE_STYLE_SOLID);
    
    // font20 -> 14 Pixel breit
    // 800 - 150 = 650 Pixel stehen zur verfügung
    // 650 / 14 = 45 Zeichen
    Debug("Draw text at the bottom\r\n");
    Paint_DrawString_EN(160, 340, "Tomorrow is the biggest lie we tell ourselves.", &Font20, EPD_7IN3F_WHITE, EPD_7IN3F_YELLOW);
    Paint_DrawString_EN(160, 360, "Because tomorrow never actually comes.", &Font20, EPD_7IN3F_WHITE, EPD_7IN3F_ORANGE);
    Paint_DrawString_EN(160, 380, "Only today exists. And today is when you", &Font20, EPD_7IN3F_WHITE, EPD_7IN3F_BLUE);
    Paint_DrawString_EN(160, 400, "either keep the streak alive - or let it die.", &Font20, EPD_7IN3F_WHITE, EPD_7IN3F_RED);
    Paint_DrawString_EN(160, 420, "test test test test test test test test test", &Font20, EPD_7IN3F_WHITE, EPD_7IN3F_RED);
    Paint_DrawString_EN(160, 440, "test test test test test test test test test", &Font20, EPD_7IN3F_WHITE, EPD_7IN3F_RED);
    Paint_DrawString_EN(160, 460, "test test test test test test test test test", &Font20, EPD_7IN3F_WHITE, EPD_7IN3F_RED);

    // Monat oben
    // font24 -> 17 Pixel breit
    // 800 - 150 = 650 Pixel stehen zur verfügung
    // 150 + ((800 - 150) / 2) = 475 <- Mitte
    // 475 - (2*17)
    // 475 - 34 = 441
    Debug("Draw name of month\r\n");
    Paint_DrawString_EN(441, 5, "März", &Font24, EPD_7IN3F_WHITE, EPD_7IN3F_BLACK);


    // Name der Tage anzeigen
    Debug("Draw name of days\r\n");
    Paint_DrawString_EN(195, 40, "Mo", &Font20, EPD_7IN3F_WHITE, EPD_7IN3F_GREEN);
    Paint_DrawString_EN(285, 40, "Di", &Font20, EPD_7IN3F_WHITE, EPD_7IN3F_GREEN);
    Paint_DrawString_EN(375, 40, "Mi", &Font20, EPD_7IN3F_WHITE, EPD_7IN3F_GREEN);
    Paint_DrawString_EN(465, 40, "Do", &Font20, EPD_7IN3F_WHITE, EPD_7IN3F_GREEN);
    Paint_DrawString_EN(555, 40, "Fr", &Font20, EPD_7IN3F_WHITE, EPD_7IN3F_GREEN);
    Paint_DrawString_EN(645, 40, "Sa", &Font20, EPD_7IN3F_WHITE, EPD_7IN3F_GREEN);
    Paint_DrawString_EN(735, 40, "So", &Font20, EPD_7IN3F_WHITE, EPD_7IN3F_GREEN);

    // Tage mitte // TODO vllt nur aktuelle (nächste Woche anzeigen)
    // 800 - 150 = 650 Pixel stehen zur verfügung
    // 650 / 7 = 91 Pixel pro Tag
    // 480 - 180 = 300 Pixel stehen zur verfügung
    // 300 / 6 = 50 Pixel pro Woche
    // starten zu zeichnen: 30 / 150
    int x = 0;
    int y = 0;
    int firstWeekdayOfMonth = getFirstWeekdayOfMonth();
    int currentDay = getCurrentDayOfMonth();
    int daysInMonth = 31;

    Debug("Draw days\r\n");
    int i;
    for (i=firstWeekdayOfMonth; i<daysInMonth+firstWeekdayOfMonth; i++) {
        int tagDerWoche = (i%7)==0?7:(i%7);
        int woche = ((i-1)/7)+1;
        x = 150 + (90*tagDerWoche) - 45;
        y = 30 + (50*woche) - 25;

        int temp = i-firstWeekdayOfMonth+1;
        static char numStr[52];  // Puffer für die Zeichenkette
        sprintf(numStr, "%d", temp);
        // printf("%d %d %s\n", x, y, numStr);
    
        if (temp == currentDay) {
            // aktuellen Tag hervorheben
            // font24: 24 Pixel hoch und 17 Pixel breit
            // 24 hoch / 34 breit 
            // mitte: 12 hoch / 17 breit
            Paint_DrawCircle(x+17, y+11, 26, EPD_7IN3F_ORANGE, DOT_PIXEL_8X8, DRAW_FILL_FULL);
            Paint_DrawString_EN(x, y, numStr, &Font24, EPD_7IN3F_ORANGE, EPD_7IN3F_WHITE);
        } else {
            Paint_DrawString_EN(x, y, numStr, &Font24, EPD_7IN3F_WHITE, EPD_7IN3F_BLACK);
        }
    }


    // Paint_DrawLine(001, 400, 479, 400, EPD_7IN3F_ORANGE, DOT_PIXEL_1X1, LINE_STYLE_SOLID);
    // Paint_DrawLine(001, 500, 479, 500, EPD_7IN3F_ORANGE, DOT_PIXEL_1X1, LINE_STYLE_SOLID);
    // Paint_DrawLine(001, 600, 479, 600, EPD_7IN3F_ORANGE, DOT_PIXEL_1X1, LINE_STYLE_SOLID);
    // Paint_DrawLine(001, 700, 479, 700, EPD_7IN3F_ORANGE, DOT_PIXEL_1X1, LINE_STYLE_SOLID);

    // Paint_DrawLine(50, 00, 50, 480, EPD_7IN3F_RED, DOT_PIXEL_1X1, LINE_STYLE_SOLID);
    // Paint_DrawLine(750, 00, 750, 480, EPD_7IN3F_RED, DOT_PIXEL_1X1, LINE_STYLE_SOLID);


    
    // char buffer[50];
    // time_t t = time(NULL);
    // struct tm *tm_info = localtime(&t);
    // strftime(buffer, sizeof(buffer), "%A, %d. %B %Y", tm_info);
    
    // Paint_DrawString_EN(000, 40, buffer, &Font24, EPD_7IN3F_WHITE, EPD_7IN3F_GREEN);



    
    // Paint_DrawString_EN(400, 20, mon, &Font16, EPD_7IN3F_BLACK, EPD_7IN3F_WHITE);
    // Paint_DrawString_EN(400, 40, day, &Font16, EPD_7IN3F_BLACK, EPD_7IN3F_WHITE);
    // Paint_DrawString_EN(400, 60, hour, &Font16, EPD_7IN3F_BLACK, EPD_7IN3F_WHITE);
    // Paint_DrawString_EN(400, 80, min, &Font16, EPD_7IN3F_BLACK, EPD_7IN3F_WHITE);

    // Paint_DrawLine(20, 70, 70, 120, EPD_7IN3F_ORANGE, DOT_PIXEL_1X1, LINE_STYLE_SOLID);
    // Paint_DrawLine(70, 70, 20, 120, EPD_7IN3F_ORANGE, DOT_PIXEL_1X1, LINE_STYLE_SOLID);
    // Paint_DrawRectangle(20, 70, 70, 120, EPD_7IN3F_BLACK, DOT_PIXEL_1X1, DRAW_FILL_EMPTY);
    // Paint_DrawRectangle(80, 70, 130, 120, EPD_7IN3F_BLACK, DOT_PIXEL_1X1, DRAW_FILL_FULL);
    // Paint_DrawCircle(45, 95, 20, EPD_7IN3F_BLACK, DOT_PIXEL_1X1, DRAW_FILL_EMPTY);
    // Paint_DrawCircle(105, 95, 20, EPD_7IN3F_WHITE, DOT_PIXEL_1X1, DRAW_FILL_FULL);
    // Paint_DrawLine(85, 95, 125, 95, EPD_7IN3F_YELLOW, DOT_PIXEL_1X1, LINE_STYLE_DOTTED);
    // Paint_DrawLine(105, 75, 105, 115, EPD_7IN3F_YELLOW, DOT_PIXEL_1X1, LINE_STYLE_DOTTED);
    // Paint_DrawString_CN(10, 160, "���Abc", &Font12CN, EPD_7IN3F_BLACK, EPD_7IN3F_WHITE);
    // Paint_DrawString_CN(10, 180, "΢ѩ����", &Font24CN, EPD_7IN3F_WHITE, EPD_7IN3F_BLACK);
    // Paint_DrawNum(10, 33, 123456789, &Font12, EPD_7IN3F_BLACK, EPD_7IN3F_WHITE);
    // Paint_DrawNum(10, 50, 987654321, &Font16, EPD_7IN3F_WHITE, EPD_7IN3F_BLACK);
    // Paint_DrawString_EN(400, 0, "waveshare", &Font16, EPD_7IN3F_BLACK, EPD_7IN3F_WHITE);
    // Paint_DrawString_EN(400, 20, "waveshare", &Font16, EPD_7IN3F_GREEN, EPD_7IN3F_WHITE);
    // Paint_DrawString_EN(400, 40, "waveshare", &Font16, EPD_7IN3F_BLUE, EPD_7IN3F_WHITE);
    // Paint_DrawString_EN(400, 60, "waveshare", &Font16, EPD_7IN3F_RED, EPD_7IN3F_WHITE);
    // Paint_DrawString_EN(400, 80, "waveshare", &Font16, EPD_7IN3F_YELLOW, EPD_7IN3F_WHITE);
    // Paint_DrawString_EN(400, 100, "waveshare", &Font16, EPD_7IN3F_ORANGE, EPD_7IN3F_WHITE);
    // Paint_DrawString_EN(150, 0, "hello world", &Font24, EPD_7IN3F_WHITE, EPD_7IN3F_BLACK);
    // Paint_DrawString_EN(150, 30, "hello world", &Font24, EPD_7IN3F_GREEN, EPD_7IN3F_BLACK);
    // Paint_DrawString_EN(150, 60, "hello world", &Font24, EPD_7IN3F_BLUE, EPD_7IN3F_BLACK);
    // Paint_DrawString_EN(150, 90, "hello world", &Font24, EPD_7IN3F_RED, EPD_7IN3F_BLACK);
    // Paint_DrawString_EN(150, 120, "hello world", &Font24, EPD_7IN3F_YELLOW, EPD_7IN3F_BLACK);
    // Paint_DrawString_EN(150, 150, "hello world", &Font24, EPD_7IN3F_ORANGE, EPD_7IN3F_BLACK);
    // Paint_DrawString_EN(150, 180, "hello world", &Font24, EPD_7IN3F_BLACK, EPD_7IN3F_YELLOW);

    printf("EPD_Display\r\n");
    EPD_7IN3F_Display(BlackImage);
    DEV_Delay_ms(3000);
#endif


#if 1 // test 
    // TODO
    // Wetterdaten der nächsten Tage holen
    // ich will Wetter-Icon und min/max Temperatur anzeigen

    const char *apiKey = getApiKey();
    printf("apiKey: %s\n", apiKey);
    char *json = fetch_forecast_data(apiKey);
    printf("forecast json: %s\n", json);



    cJSON *json_array = extract_json_array(json, "daily");
    if (!json_array) {
        return 1;
    }
    printf("array: %s\r\n", json_array);
    printf("array->valuestring: %s\r\n", json_array->valuestring);
    printf("array geholt\r\n");

    // String holen
    cJSON *temp_value = get_value_from_json_array(json_array, 1, "humidity");
    if (cJSON_IsString(temp_value)) {
        printf("Luftfeuchtigkeit am zweiten Tag: %s\n", temp_value->valuestring);
    }
    printf("temp: %s\r\n", temp_value);
    printf("temp->valuestring: %s\r\n", temp_value->valuestring);
    printf("nach humidity\r\n");

    // JSON-Objekt holen
    cJSON *details = get_value_from_json_array(json_array, 1, "temp");
    if (cJSON_IsObject(details)) {
        cJSON *humidity = cJSON_GetObjectItem(details, "day");
        if (cJSON_IsString(humidity)) {
            printf("Temperatur am zweiten Tag: %s\n", humidity->valuestring);
        }
    }

    printf("nach temp\r\n");


    cJSON_Delete(json_array); // Speicherfreigabe

    

    free(apiKey);
    free(json);

#endif

#if 0   // Drawing image from char arry
    // printf("show picture 1------------------------\r\n");
    // Paint_SelectImage(BlackImage);
    // Paint_Clear(EPD_7IN3F_WHITE);
    // Paint_DrawBitMap(aImage7color);
    // EPD_7IN3F_Display(BlackImage);
    // DEV_Delay_ms(3000);

    printf("show picture 2------------------------\r\n");
    Paint_SelectImage(BlackImage);
    Paint_DrawBitMap(bImage7color);
    EPD_7IN3F_Display(BlackImage);
    DEV_Delay_ms(3000);

    // printf("show picture 3------------------------\r\n");
    // Paint_SelectImage(BlackImage);
    // Paint_DrawBitMap(ThirdImage7color);
    // EPD_7IN3F_Display(BlackImage);
    // DEV_Delay_ms(3000);
    
    // printf("show picture 4------------------------\r\n");
    // Paint_SelectImage(BlackImage);
    // Paint_DrawBitMap(ForthImage7color);
    // EPD_7IN3F_Display(BlackImage);
    // DEV_Delay_ms(3000);
#endif


#if 0   // Drawing on the image
    //1.Select Image
    printf("SelectImage:BlackImage\r\n");
    Paint_SelectImage(BlackImage);
    Paint_Clear(EPD_7IN3F_WHITE);

    int hNumber, hWidth, vNumber, vWidth;
    hNumber = 10;
    hWidth = EPD_7IN3F_HEIGHT/hNumber; // 480/10=48
    vNumber = 20;
    vWidth = EPD_7IN3F_WIDTH/vNumber; // 800/20=40
    
    // 2.Drawing on the image
    printf("Drawing:BlackImage\r\n");
    for(int i=0; i<hNumber; i++) {  // horizontal
        Paint_DrawRectangle(1, 1+i*hWidth, EPD_7IN3F_WIDTH, hWidth*(1+i), EPD_7IN3F_BLACK + (i % 2), DOT_PIXEL_1X1, DRAW_FILL_FULL);
    }
    for(int i=0; i<vNumber; i++) {  // vertical
        if(i%2) {
            Paint_DrawRectangle(1+i*vWidth, 1, vWidth*(i+1), EPD_7IN3F_HEIGHT, EPD_7IN3F_GREEN + (i%5), DOT_PIXEL_1X1, DRAW_FILL_FULL);
        }
    }

    printf("EPD_Display\r\n");
    EPD_7IN3F_Display(BlackImage);
    DEV_Delay_ms(3000);
#endif

    // printf("Clear...\r\n");
    // EPD_7IN3F_Clear(EPD_7IN3F_WHITE);

    // printf("Goto Sleep...\r\n");
    // EPD_7IN3F_Sleep();
    // free(BlackImage);
    // BlackImage = NULL;
    // DEV_Delay_ms(2000); // important, at least 2s
    // // close 5V
    // printf("close 5V, Module enters 0 power consumption ...\r\n");
    // DEV_Module_Exit();
    
    return 0;
}





