#include "../include/api.h"
#include "../include/output.h"

FILE *log_file;

// 日志记录函数
void log_message(const char *message)
{
    if (log_file)
    {
        time_t now = time(NULL);
        struct tm *t = localtime(&now);
        fprintf(log_file, "[%04d-%02d-%02d %02d:%02d:%02d] %s\n",
                t->tm_year + 1900, t->tm_mon + 1, t->tm_mday,
                t->tm_hour, t->tm_min, t->tm_sec, message);
        fflush(log_file); // 立即写入日志
    }
}

size_t WriteMemoryCallback(void *contents, size_t size, size_t nmemb, struct MemoryStruct *userp)
{
    size_t realsize = size * nmemb;
    userp->memory = realloc(userp->memory, userp->size + realsize + 1);
    if (userp->memory == NULL)
    {
        printf(_RED("Not enough memory!\n"));
        return 0;  // Out of memory!
    }
    memcpy(&(userp->memory[userp->size]), contents, realsize);
    userp->size += realsize;
    userp->memory[userp->size] = 0;  // Null-terminate
    return realsize;
}

// Function to download the image
void download_image(const char *url, const char *filename)
{
    CURL *curl;
    CURLcode res;
    FILE *fp;

    curl_global_init(CURL_GLOBAL_DEFAULT);
    curl = curl_easy_init();

    if (curl)
    {
        fp = fopen(filename, "wb");
        if (fp == NULL)
        {
            fprintf(stderr, _RED("Could not open file for writing\n"));
            return;
        }

        curl_easy_setopt(curl, CURLOPT_URL, url);
        curl_easy_setopt(curl, CURLOPT_VERBOSE, 1L);
        curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 1L);
        curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 2L);
        curl_easy_setopt(curl, CURLOPT_CAINFO, "./lib/cacert.pem");

        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, NULL);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, fp);
        res = curl_easy_perform(curl);

        if (res != CURLE_OK)
        {
            fprintf(stderr, _RED("curl_easy_perform() failed: %s\n"), curl_easy_strerror(res));
        }

        fclose(fp);
        curl_easy_cleanup(curl);
    }

    curl_global_cleanup();
}

void api(int days) // 接受天数作为参数
{
    CURL *curl;
    CURLcode res;
    struct MemoryStruct chunk = {NULL, 0};
    struct curl_slist *headers = NULL;

    curl_global_init(CURL_GLOBAL_DEFAULT);
    curl = curl_easy_init();

    if (curl)
    {
        // 使用用户输入的天数构建 URL
        char url[256];
        snprintf(url, sizeof(url), "https://cn.bing.com/HPImageArchive.aspx?format=js&idx=%d&n=1", days);
        curl_easy_setopt(curl, CURLOPT_URL, url);
        curl_easy_setopt(curl, CURLOPT_VERBOSE, 1L);
        curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 1L);
        curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 2L);
        curl_easy_setopt(curl, CURLOPT_CAINFO, "./lib/cacert.pem");

        headers = curl_slist_append(headers, "Accept: application/json");
        headers = curl_slist_append(headers, "Accept-Language: zh-CN,zh;q=0.5");
        curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
        
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteMemoryCallback);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void *)&chunk);
        res = curl_easy_perform(curl);

        if (res != CURLE_OK)
        {
            log_message("curl_easy_perform() failed");
            fprintf(stderr, _RED("curl_easy_perform() failed: %s\n"), curl_easy_strerror(res));
        }
        else
        {
            log_message("Received JSON data");
            // 解析 JSON
            cJSON *json = cJSON_Parse(chunk.memory);
            if (json == NULL)
            {
                log_message("JSON parsing error");
                fprintf(stderr, _RED("JSON parsing error\n"));
                free(chunk.memory);
                curl_easy_cleanup(curl);
                return;
            }

            cJSON *images = cJSON_GetObjectItem(json, "images");
            if (cJSON_IsArray(images) && cJSON_GetArraySize(images) > 0)
            {
                cJSON *first_image = cJSON_GetArrayItem(images, 0);
                cJSON *url_item = cJSON_GetObjectItem(first_image, "url");
                cJSON *copyright_item = cJSON_GetObjectItem(first_image, "copyright");
                cJSON *title_item = cJSON_GetObjectItem(first_image, "title");

                if (url_item != NULL && cJSON_IsString(url_item) &&
                    copyright_item != NULL && cJSON_IsString(copyright_item) &&
                    title_item != NULL && cJSON_IsString(title_item))
                {
                    // 拼接完整的图片 URL
                    char full_url[256];
                    snprintf(full_url, sizeof(full_url), "https://cn.bing.com%s", url_item->valuestring);
                    log_message("Found image URL");
                    printf(_GREEN("Found image URL: %s\n"), full_url);

                    // 获取日期并保存图片
                    const char *date_string = cJSON_GetObjectItem(first_image, "startdate")->valuestring;
                    char filename[256];
                    snprintf(filename, sizeof(filename), "./bin/img/%s.jpg", date_string);
                    download_image(full_url, filename);

                    // 保存图片信息到文本文件
                    char info_filename[256];
                    snprintf(info_filename, sizeof(info_filename), "./bin/inf/%s.txt", date_string);
                    FILE *info_file = fopen(info_filename, "w");
                    if (info_file)
                    {
                        fprintf(info_file, "Title: %s\n", title_item->valuestring);
                        fprintf(info_file, "Copyright: %s\n", copyright_item->valuestring);
                        fclose(info_file);
                        log_message("Image information saved");
                    }
                    else
                    {
                        log_message("Could not open info file for writing");
                        fprintf(stderr, _RED("Could not open info file for writing\n"));
                    }
                }
                else
                {
                    log_message("Image URL or metadata not found in JSON");
                    fprintf(stderr, _RED("Image URL or metadata not found in JSON.\n"));
                }
            }
            else
            {
                log_message("No images found in JSON");
                fprintf(stderr, _RED("No images found in JSON.\n"));
            }

            cJSON_Delete(json);
        }

        free(chunk.memory);
        curl_easy_cleanup(curl);
    }

    curl_global_cleanup();
}

// 在程序结束时关闭日志文件
void close_log(void)
{
    if (log_file)
    {
        fclose(log_file);
    }
}