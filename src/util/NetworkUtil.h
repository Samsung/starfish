/*
    This is temporal soluation. (mh.byun)
    FROM rnpoc
*/
#ifndef NetworkingModule_h
#define NetworkingModule_h

#include <curl/curl.h>


class NetworkUtil{
public:
    struct Buffer {
        char *memory;
        size_t size;
    };

    NetworkUtil()
    {
    }
    ~NetworkUtil()
    {
    }

    // Copy from http://breckon.eu/toby/teaching/dip/rovio/imagetest.cc
    static void* CURL_realloc(void *ptr, size_t size)
    {
        /* There might be a realloc() out there that doesn't like reallocing
         *      NULL pointers, so we take care of it here */
        if(ptr)
            return realloc(ptr, size);
        else
            return malloc(size);
    }

    static size_t WriteMemoryCallback(void *ptr, size_t size, size_t nmemb, void *data)
    {
        size_t realsize = size * nmemb;
        struct Buffer *mem = (struct Buffer *)data;

        mem->memory = (char *)
            CURL_realloc(mem->memory, mem->size + realsize + 1);
        if (mem->memory) {
            memcpy(&(mem->memory[mem->size]), ptr, realsize);
            mem->size += realsize;
            mem->memory[mem->size] = 0;
        }
        return realsize;
    }

    static size_t WriteFileCallback(void *ptr, size_t size, size_t nmemb, void* userdata)
    {
        FILE* stream = (FILE*)userdata;
        if (!stream)
        {
            printf("!!! No stream\n");
            return 0;
        }

        size_t written = fwrite((FILE*)ptr, size, nmemb, stream);
        return written;
    }

    static bool writeFileFromUrl(const char* filename, const char* url)
    {
        FILE* fp = fopen(filename, "wb");
        CURL* curl = curl_easy_init();
        CURLcode res;

        if (!curl) return false;
        curl_easy_setopt(curl, CURLOPT_URL, url);
        curl_easy_setopt(curl, CURLOPT_VERBOSE, 1);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, fp);
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteFileCallback);
        curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1);

        res = curl_easy_perform(curl);

        long res_code = 0;
        curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &res_code);
        if (!((res_code == 200 || res_code == 201) && res != CURLE_ABORTED_BY_CALLBACK))
        {
            printf("!!! Response code: %ld\n", res_code);
            return false;
        }

        curl_easy_cleanup(curl);

        fclose(fp);
        return true;
    }

    static int writeMemoryFromUrl(Buffer* buffer, Buffer* header, const char* url)
    {
        CURL* curl = curl_easy_init();
        CURLcode res;

        if (!curl) return false;
        curl_easy_setopt(curl, CURLOPT_URL, url);
        curl_easy_setopt(curl, CURLOPT_VERBOSE, 1);
        curl_easy_setopt(curl, CURLOPT_HEADERDATA, (void*) header);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void*) buffer);
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteMemoryCallback);
        curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1);

        res = curl_easy_perform(curl);

        long res_code = 0;
        curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &res_code);
        if (!((res_code == 200 || res_code == 201) && res != CURLE_ABORTED_BY_CALLBACK))
        {
            printf("!!! Response code: %ld\n", res_code);
            //return res_code;
        }

        curl_easy_cleanup(curl);

        return res_code;
    }
};

#endif
