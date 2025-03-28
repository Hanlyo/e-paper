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


// Funktion zum Extrahieren der Min- und Max-Temperatur aus daily->temp
char *get_daily_temp_range(const char *json_str, int day_index) {
    cJSON *root = cJSON_Parse(json_str);
    if (!root) {
        fprintf(stderr, "Fehler beim Parsen des JSON.\n");
        return NULL;
    }

    cJSON *daily_array = cJSON_GetObjectItem(root, "daily");
    if (!cJSON_IsArray(daily_array)) {
        fprintf(stderr, "daily ist kein Array.\n");
        cJSON_Delete(root);
        return NULL;
    }

    cJSON *day_entry = cJSON_GetArrayItem(daily_array, day_index);
    if (!cJSON_IsObject(day_entry)) {
        fprintf(stderr, "Kein Eintrag für Index %d.\n", day_index);
        cJSON_Delete(root);
        return NULL;
    }

    cJSON *temp_obj = cJSON_GetObjectItem(day_entry, "temp");
    if (!cJSON_IsObject(temp_obj)) {
        fprintf(stderr, "Kein temp-Objekt für Index %d.\n", day_index);
        cJSON_Delete(root);
        return NULL;
    }

    cJSON *temp_min = cJSON_GetObjectItem(temp_obj, "min");
    cJSON *temp_max = cJSON_GetObjectItem(temp_obj, "max");

    if (!cJSON_IsNumber(temp_min) || !cJSON_IsNumber(temp_max)) {
        fprintf(stderr, "Ungültige min/max-Werte für Index %d.\n", day_index);
        cJSON_Delete(root);
        return NULL;
    }

    // Speicher für das Ausgabeformat "min°C/max°C"
    char *result = (char *)malloc(20);
    if (!result) {
        fprintf(stderr, "Speicherzuweisung fehlgeschlagen.\n");
        cJSON_Delete(root);
        return NULL;
    }
    snprintf(result, 20, "%.0f°C/%.0f°C", temp_min->valuedouble, temp_max->valuedouble);

    cJSON_Delete(root);
    return result;
}



// Funktion zum Extrahieren des Wetter-Icons aus daily->weather->icon
const char *get_daily_weather_icon(const char *json_str, int day_index) {
    cJSON *root = cJSON_Parse(json_str);
    if (!root) {
        fprintf(stderr, "Fehler beim Parsen des JSON.\n");
        return NULL;
    }

    cJSON *daily_array = cJSON_GetObjectItem(root, "daily");
    if (!cJSON_IsArray(daily_array)) {
        fprintf(stderr, "daily ist kein Array.\n");
        cJSON_Delete(root);
        return NULL;
    }

    cJSON *day_entry = cJSON_GetArrayItem(daily_array, day_index);
    if (!cJSON_IsObject(day_entry)) {
        fprintf(stderr, "Kein Eintrag für Index %d.\n", day_index);
        cJSON_Delete(root);
        return NULL;
    }

    cJSON *weather_array = cJSON_GetObjectItem(day_entry, "weather");
    if (!cJSON_IsArray(weather_array)) {
        fprintf(stderr, "Kein weather-Array für Index %d.\n", day_index);
        cJSON_Delete(root);
        return NULL;
    }

    cJSON *weather_entry = cJSON_GetArrayItem(weather_array, 0);
    if (!cJSON_IsObject(weather_entry)) {
        fprintf(stderr, "Kein gültiger weather-Eintrag für Index %d.\n", day_index);
        cJSON_Delete(root);
        return NULL;
    }

    cJSON *icon = cJSON_GetObjectItem(weather_entry, "icon");
    if (!cJSON_IsString(icon)) {
        fprintf(stderr, "Kein gültiger icon-Wert für Index %d.\n", day_index);
        cJSON_Delete(root);
        return NULL;
    }

    const char *icon_value = icon->valuestring;
    cJSON_Delete(root);
    return icon_value;
}


int EPD_7in3f_test(void)
{
    printf("EPD_7IN3F_test Demo\r\n");
    if(DEV_Module_Init()!=0){
        return -1;
    }

    printf("e-Paper Init and Clear...\r\n");
    EPD_7IN3F_Init();

    struct timespec start={0,0}, finish={0,0}; 
    clock_gettime(CLOCK_REALTIME, &start);
    EPD_7IN3F_Clear(EPD_7IN3F_WHITE); // WHITE
    clock_gettime(CLOCK_REALTIME, &finish);
    printf("%ld S\r\n", finish.tv_sec-start.tv_sec);    
    DEV_Delay_ms(1000);

    //Create a new image cache
    UBYTE *BlackImage;
    UDOUBLE Imagesize = ((EPD_7IN3F_WIDTH % 2 == 0)? (EPD_7IN3F_WIDTH / 2 ): (EPD_7IN3F_WIDTH / 2 + 1)) * EPD_7IN3F_HEIGHT;
    if((BlackImage = (UBYTE *)malloc(Imagesize)) == NULL) {
        printf("Failed to apply for black memory...\r\n");
        return -1;
    }
    printf("Paint_NewImage\r\n");
    Paint_NewImage(BlackImage, EPD_7IN3F_WIDTH, EPD_7IN3F_HEIGHT, 0, EPD_7IN3F_WHITE);
    Paint_SetScale(7);

    Paint_SetRotate(ROTATE_180);

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
#if 1   // Drawing on the image

    // 1.Fetching weather data
    const char *apiKey = getApiKey();
    printf("apiKey: %s\n", apiKey);
    char *json = fetch_forecast_data(apiKey);
    printf("forecast json: %s\n", json);



    char *temp0 = get_daily_temp_range(json, 0);
    printf("temp: %s\n", temp0);

    char *temp1 = get_daily_temp_range(json, 1);
    printf("temp: %s\n", temp1);

    char *temp2 = get_daily_temp_range(json, 2);
    printf("temp: %s\n", temp2);

    char *temp3 = get_daily_temp_range(json, 3);
    printf("temp: %s\n", temp3);



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
    // Debug("Draw horizontal line\r\n");
    // Paint_DrawLine(000, 120, 148, 120, EPD_7IN3F_RED, DOT_PIXEL_1X1, LINE_STYLE_SOLID);
    // Paint_DrawLine(000, 240, 148, 240, EPD_7IN3F_BLUE, DOT_PIXEL_1X1, LINE_STYLE_SOLID);
    // Paint_DrawLine(000, 360, 148, 360, EPD_7IN3F_GREEN, DOT_PIXEL_1X1, LINE_STYLE_SOLID);

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
    Paint_DrawString_EN(21, 90, temp0, &Font16, EPD_7IN3F_WHITE, EPD_7IN3F_BLUE);
    Paint_DrawRectangle(40, 130, 110, 200, EPD_7IN3F_RED, DOT_PIXEL_1X1, DRAW_FILL_FULL);
    Paint_DrawString_EN(21, 210, temp1, &Font16, EPD_7IN3F_WHITE, EPD_7IN3F_BLUE);
    Paint_DrawRectangle(40, 250, 110, 320, EPD_7IN3F_ORANGE, DOT_PIXEL_1X1, DRAW_FILL_FULL);
    Paint_DrawString_EN(21, 330, temp2, &Font16, EPD_7IN3F_WHITE, EPD_7IN3F_RED);
    Paint_DrawRectangle(40, 370, 110, 440, EPD_7IN3F_BLUE, DOT_PIXEL_1X1, DRAW_FILL_FULL);
    Paint_DrawString_EN(21, 450, temp3, &Font16, EPD_7IN3F_WHITE, EPD_7IN3F_RED);

    // horizontale Linie unten
    Debug("Draw horizontal line at the bottom\r\n");
    Paint_DrawLine(150, 332, 800, 332, EPD_7IN3F_ORANGE, DOT_PIXEL_2X2, LINE_STYLE_SOLID);
    
    // font20 -> 14 Pixel breit
    // 800 - 150 = 650 Pixel stehen zur verfügung
    // 650 / 14 = 45 Zeichen
    Debug("Draw text at the bottom\r\n");
    int yStart = 360; // 340
    int yTextOffset = 20;
    Paint_DrawString_EN(160, yStart, "Tomorrow is the biggest lie we tell ourselves. Only today exists. And today is when you either keep the streak alive - or let it die.", &Font20, EPD_7IN3F_WHITE, EPD_7IN3F_YELLOW);
    // Paint_DrawString_EN(160, yStart += yTextOffset, "Because tomorrow never actually comes.", &Font20, EPD_7IN3F_WHITE, EPD_7IN3F_ORANGE);
    // Paint_DrawString_EN(160, 380, "Only today exists. And today is when you", &Font20, EPD_7IN3F_WHITE, EPD_7IN3F_BLUE);
    // Paint_DrawString_EN(160, 400, "either keep the streak alive - or let it die.", &Font20, EPD_7IN3F_WHITE, EPD_7IN3F_RED);
    // Paint_DrawString_EN(160, 420, "test test test test test test test test test", &Font20, EPD_7IN3F_WHITE, EPD_7IN3F_RED);
    // Paint_DrawString_EN(160, 440, "test test test test test test test test test", &Font20, EPD_7IN3F_WHITE, EPD_7IN3F_RED);
    // Paint_DrawString_EN(160, 460, "test test test test test test test test test", &Font20, EPD_7IN3F_WHITE, EPD_7IN3F_RED);

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
            // Paint_DrawCircle(x+17, y+11, 26, EPD_7IN3F_ORANGE, DOT_PIXEL_1X1, DRAW_FILL_EMPTY);
            Paint_DrawString_EN(x, y, numStr, &Font24, EPD_7IN3F_WHITE, EPD_7IN3F_ORANGE);
        } else {
            Paint_DrawString_EN(x, y, numStr, &Font24, EPD_7IN3F_WHITE, EPD_7IN3F_BLACK);
        }
    }

    
    // free everything
    free(temp0);
    free(temp1);
    free(temp2);
    free(temp3);
    free(apiKey);
    free(json);
    
    
    printf("EPD_Display\r\n");
    EPD_7IN3F_Display(BlackImage);
    DEV_Delay_ms(3000);
#endif


#if 0 // test 

    const char *apiKey = getApiKey();
    printf("apiKey: %s\n", apiKey);
    char *json = fetch_forecast_data(apiKey);
    printf("forecast json: %s\n", json);



    double temp = get_daily_temp_day(json, 1);
    printf("temp: %.2f\n", temp);

    const char *icon = get_daily_weather_icon(json, 1);
    printf("icon: %s\n", icon);

    static char a[52];  // Puffer für die Zeichenkette
    sprintf(a, "%s", icon);
    printf("icon: %s\n", a);

    // cJSON *json_array = extract_json_array(json, "daily");
    // if (!json_array) {
    //     return 1;
    // }

    // // String holen
    // cJSON *temp_value = get_value_from_json_array(json_array, 1, "humidity");
    // if (cJSON_IsString(temp_value)) {
    //     printf("Luftfeuchtigkeit am zweiten Tag: %s\n", temp_value->valuestring);
    // }
    // static char a[52];  // Puffer für die Zeichenkette
    // sprintf(a, "%s", temp_value->valuestring);

    // printf("temp: %s\r\n", a);
    // printf("nach humidity\r\n");

    // // JSON-Objekt holen
    // cJSON *details = get_value_from_json_array(json_array, 1, "temp");
    // if (cJSON_IsObject(details)) {
    //     cJSON *humidity = cJSON_GetObjectItem(details, "day");
    //     static char b[52];  // Puffer für die Zeichenkette
    //     sprintf(b, "%s", humidity->valuestring);
    //     if (cJSON_IsString(humidity)) {
    //         printf("Temperatur am zweiten Tag: %s\n", humidity->valuestring);
    //     }
    // }
    // printf("nach temp\r\n");


    // cJSON_Delete(json_array); // Speicherfreigabe

    

    free(apiKey);
    free(json);

#endif

    // printf("Clear...\r\n");
    // EPD_7IN3F_Clear(EPD_7IN3F_WHITE);

    printf("Goto Sleep...\r\n");
    EPD_7IN3F_Sleep();
    free(BlackImage);
    BlackImage = NULL;
    DEV_Delay_ms(2000); // important, at least 2s
    // close 5V
    printf("close 5V, Module enters 0 power consumption ...\r\n");
    DEV_Module_Exit();
    
    return 0;
}





