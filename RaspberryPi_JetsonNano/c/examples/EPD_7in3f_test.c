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







#define URL_FORMAT "http://api.openweathermap.org/data/2.5/weather?q=%s&appid=%s&units=metric&units=metric&lang=de"

struct MemoryStruct {
    char *memory;
    size_t size;
};

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

char *fetch_weather_data(const char *city, const char *api_key) {
    CURL *curl;
    CURLcode res;
    struct MemoryStruct chunk;

    chunk.memory = malloc(1);
    chunk.size = 0;

    char url[256];
    snprintf(url, sizeof(url), URL_FORMAT, city, api_key);

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
    GUI_ReadBmp_RGB_7Color("./pic/7in3e.bmp", 0, 0);
    EPD_7IN3F_Display(BlackImage);
    DEV_Delay_ms(3000);

    
    // printf("show bmp2-----------------\r\n");
    // Paint_SelectImage(BlackImage);
    // Paint_Clear(EPD_7IN3F_WHITE);
    // GUI_ReadBmp_16Gray("./pic/4in2_Scale_2.bmp", 0, 0);
    // EPD_7IN3F_Display(BlackImage);
    // DEV_Delay_ms(3000);

    // printf("show bmp3------------------------\r\n");
    // Paint_SelectImage(BlackImage);
    // GUI_ReadBmp_RGB_7Color("./pic/7in3f1.bmp", 0, 0);
    // EPD_7IN3F_Display(BlackImage);
    // DEV_Delay_ms(3000);
#endif


#define API_KEY "3128d4668dffb45c2e2c08d9f2c52d82"
#define CITY "Berlin"


#if 1   // Drawing on the image


    char *json = fetch_weather_data(CITY, API_KEY);
    if (json == NULL) {
        fprintf(stderr, "Fehler beim Abrufen der Wetterdaten\n");
        return 1;
    }

    double temp = get_temperature(json);
    char *description = get_weather_description(json);

    printf("Temperatur: %.2f°C\n", temp);
    printf("Wetter: %s\n", description ? description : "Unbekannt");


    // static char str[52];  // Puffer für die Zeichenkette
    // sprintf(str, "%f", temp);
    // const char *ptr = str; // Zeiger auf die Zeichenkette
    // Paint_DrawString_EN(200, 0, str, &Font24, EPD_7IN3F_WHITE, EPD_7IN3F_ORANGE);
    Paint_DrawString_EN(400, 0, description, &Font24, EPD_7IN3F_WHITE, EPD_7IN3F_ORANGE);

    free(json);
    free(description);




    // 1.Select Image
    printf("SelectImage:BlackImage\r\n");
    Paint_SelectImage(BlackImage);
    Paint_Clear(EPD_7IN3F_WHITE);

    // 2.Drawing on the image
    printf("Drawing:BlackImage\r\n");
    Paint_DrawPoint(10, 80, EPD_7IN3F_RED, DOT_PIXEL_1X1, DOT_STYLE_DFT);
    Paint_DrawPoint(10, 90, EPD_7IN3F_BLUE, DOT_PIXEL_2X2, DOT_STYLE_DFT);
    Paint_DrawPoint(10, 100, EPD_7IN3F_GREEN, DOT_PIXEL_3X3, DOT_STYLE_DFT);

    
    char buffer[50];
    time_t t = time(NULL);
    struct tm *tm_info = localtime(&t);
    strftime(buffer, sizeof(buffer), "%A, %d. %B %Y", tm_info);
    
    Paint_DrawString_EN(200, 40, buffer, &Font24, EPD_7IN3F_WHITE, EPD_7IN3F_GREEN);
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

