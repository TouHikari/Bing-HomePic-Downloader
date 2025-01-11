#ifndef _API_H_
#define _API_H_

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <curl/curl.h>
#include "../include/cJSON.h"

// Callback function to write data to a buffer
struct MemoryStruct {
    char *memory;
    size_t size;
};

// Function prototypes
void log_message(const char *message);
size_t WriteMemoryCallback(void *contents, size_t size, size_t nmemb, struct MemoryStruct *userp);
void download_image(const char *url, const char *filename);
void api(int days);
void close_log(void);

#endif