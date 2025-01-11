#ifndef _API_H_
#define _API_H_

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <curl/curl.h>

#define HTML_URL "https://cn.bing.com/"
#define LOCAL_IMAGE_FILE "./bin/img/background_image.jpg"

// Callback function to write data to a buffer
struct MemoryStruct {
    char *memory;
    size_t size;
};

// Function prototypes
size_t WriteMemoryCallback(void *contents, size_t size, size_t nmemb, struct MemoryStruct *userp);
void download_image(const char *url);
const char* extract_image_url(const char *html);
void api(void);

#endif