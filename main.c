#include <stdio.h>
#include <stdlib.h>
#include "lib/sqlite3.h"
#include "weather.h"

int main(void){

    latLongFromAdress("5%20rue%20Emile%20Masson", "Nantes", "FR");
    GPS gps = getLatLong();
    forecast( gps);
    return 0;
}

/*int  curl(char *url){
    CURL *curl;
    CURLcode res;

    curl = curl_easy_init();

    if(curl) {
        curl_easy_setopt(curl, CURLOPT_URL, url);
        *//* example.com is redirected, so we tell libcurl to follow redirection *//*
        curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);

        curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 0);
        curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0);
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, ecrire);

        *//* Perform the request, res will get the return code *//*
        res = curl_easy_perform(curl);

        if(res != CURLE_OK)
            fprintf(stderr, "curl_easy_perform() failed: %s\n",
                    curl_easy_strerror(res));



        curl_easy_cleanup(curl);
    }
}*/
