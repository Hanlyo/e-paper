#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <curl/curl.h>
#include "weather.h"

#define URL_FORMAT "http://api.openweathermap.org/data/2.5/weather?q=%s&appid=%s&units=metric"

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

void fetch_weather(const char *city, const char *api_key) {
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
        if (res == CURLE_OK) {
            printf("Wetterdaten für %s: %s\n", city, chunk.memory);
        } else {
            fprintf(stderr, "cURL Fehler: %s\n", curl_easy_strerror(res));
        }

        curl_easy_cleanup(curl);
        free(chunk.memory);
    }

    curl_global_cleanup();
}