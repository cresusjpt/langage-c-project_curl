#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include "curl/curl.h"
#include "lib/sqlite3.h"
#include "geocoding.h"

struct response {
    char *memory;
    size_t size;
};

int curl(char *url);
static size_t ecrire(char *contents, size_t size, size_t nmemb, void *userp)
{
    FILE *f = fopen("sortie.json","a+");
    if (contents != NULL && f!=NULL){
        fputs(contents, f);
    }

    return size * nmemb;
}
int curlhttps(const char *url);

int main(void){

    latLongFromAdress("5%20rue%20Emile%20Masson", "Nantes", "FR");
    return 0;
}

int curlhttps(const char *url){
    CURL *curl;
    CURLcode res;

    //curl_global_init(CURL_GLOBAL_DEFAULT);

    curl = curl_easy_init();
    //curl_easy_setopt(curl, CURLOPT_VERBOSE, 1);
    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 0);
    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0);


    if(curl) {
        curl_easy_setopt(curl, CURLOPT_URL, url);

#ifdef SKIP_PEER_VERIFICATION
        /*
     * If you want to connect to a site who is not using a certificate that is
     * signed by one of the certs in the CA bundle you have, you can skip the
     * verification of the server's certificate. This makes the connection
     * A LOT LESS SECURE.
     *
     * If you have a CA cert for the server stored someplace else than in the
     * default bundle, then the CURLOPT_CAPATH option might come handy for
     * you.
     */
    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0L);
#endif

#ifdef SKIP_HOSTNAME_VERIFICATION
        /*
     * If the site you are connecting to uses a different host name that what
     * they have mentioned in their server certificate's commonName (or
     * subjectAltName) fields, libcurl will refuse to connect. You can skip
     * this check, but this will make the connection less secure.
     */
    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 0L);
#endif

        /* Perform the request, res will get the return code */
        res = curl_easy_perform(curl);
        /* Check for errors */
        if(res != CURLE_OK)
            fprintf(stderr, "curl_easy_perform() failed: %s\n",
                    curl_easy_strerror(res));

        /* always cleanup */
        curl_easy_cleanup(curl);
    }

    //curl_global_cleanup();

    return 0;

}

int  curl(char *url){
    CURL *curl;
    CURLcode res;

    curl = curl_easy_init();

    if(curl) {
        curl_easy_setopt(curl, CURLOPT_URL, url);
        /* example.com is redirected, so we tell libcurl to follow redirection */
        curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);

        curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 0);
        curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0);
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, ecrire);

        /* Perform the request, res will get the return code */
        res = curl_easy_perform(curl);

        if(res != CURLE_OK)
            fprintf(stderr, "curl_easy_perform() failed: %s\n",
                    curl_easy_strerror(res));



        curl_easy_cleanup(curl);
    }
}
