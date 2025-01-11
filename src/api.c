#include "../include/api.h"

size_t WriteMemoryCallback(void *contents, size_t size, size_t nmemb, struct MemoryStruct *userp)
{
    size_t realsize = size * nmemb;
    userp->memory = realloc(userp->memory, userp->size + realsize + 1);
    if (userp->memory == NULL)
    {
        printf("Not enough memory!\n");
        return 0;  // Out of memory!
    }
    memcpy(&(userp->memory[userp->size]), contents, realsize);
    userp->size += realsize;
    userp->memory[userp->size] = 0;  // Null-terminate
    return realsize;
}

// Function to download the image
void download_image(const char *url)
{
    CURL *curl;
    CURLcode res;
    FILE *fp;

    curl_global_init(CURL_GLOBAL_DEFAULT);
    curl = curl_easy_init();

    if (curl)
    {
        fp = fopen(LOCAL_IMAGE_FILE, "wb");
        if (fp == NULL)
        {
            fprintf(stderr, "Could not open file for writing\n");
            return;
        }

        curl_easy_setopt(curl, CURLOPT_URL, url);
        curl_easy_setopt(curl, CURLOPT_VERBOSE, 1L);
        curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 1L);
        curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 2L);
        curl_easy_setopt(curl, CURLOPT_CAINFO, "./lib/cacert.pem");
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, NULL); // 保持为 NULL
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, fp); // 将数据写入文件指针
        res = curl_easy_perform(curl);

        if (res != CURLE_OK)
        {
            fprintf(stderr, "curl_easy_perform() failed: %s\n", curl_easy_strerror(res));
        }

        fclose(fp);
        curl_easy_cleanup(curl);
    }

    curl_global_cleanup();
}

// Function to extract image URL using simple string operations
const char* extract_image_url(const char *html)
{
    const char *start = strstr(html, "https://www.bing.com/th?id=OHR.");
    if (start)
    {
        const char *end = strstr(start, "&amp;rf=");
        if (end)
        {
            size_t length = end - start;
            char *url = (char *)malloc(length + 1);
            if (url)
            {
                strncpy(url, start, length);
                url[length] = '\0';  // Null-terminate
                return url;
            }
        }
    }
    return NULL;
}

void api(void)
{
    CURL *curl;
    CURLcode res;
    struct MemoryStruct chunk =
    {NULL, 0};

    curl_global_init(CURL_GLOBAL_DEFAULT);
    curl = curl_easy_init();

    if (curl)
    {
        // SSL CA证书
        curl_easy_setopt(curl, CURLOPT_URL, HTML_URL);
        curl_easy_setopt(curl, CURLOPT_VERBOSE, 1L);
        curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 1L); // 验证服务器证书有效性
        curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 2L); // 检验证书中的主机名和你访问的主机名是否一致
        curl_easy_setopt(curl, CURLOPT_CAINFO, "./lib/cacert.pem");  // 设置证书路径
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteMemoryCallback);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void *)&chunk);
        res = curl_easy_perform(curl);

        if (res != CURLE_OK)
        {
            fprintf(stderr, "curl_easy_perform() failed: %s\n", curl_easy_strerror(res));
        }
        else
        {
            const char *image_url = extract_image_url(chunk.memory);
            if (image_url)
            {
                printf("Found image URL: %s\n", image_url);
                download_image(image_url);
                free((void *)image_url);  // Free the duplicated URL
            }
            else
            {
                fprintf(stderr, "Image URL not found.\n");
            }
        }

        free(chunk.memory);  // Free the memory allocated for HTML
        curl_easy_cleanup(curl);
    }

    curl_global_cleanup();
}