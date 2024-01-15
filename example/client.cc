#include <curl/curl.h>


char* g_errinfo[CURL_ERROR_SIZE];
const char* url = "localhost:12220";

int main()
{
    CURLM* conn = NULL;
    CURLcode code;

	conn = curl_multi_init();

    code = curl_easy_setopt(conn, CURLOPT_ERRORBUFFER, g_errinfo);
    // CURLM_
    if (code != CURLE_OK) {
        fprintf(stderr, "Set error buffer failed! [%d]\n", code);
        return -1;
    }
    
    code = curl_easy_setopt(conn, CURLOPT_URL, url);
    if (code != CURLE_OK) {
        
    }

}