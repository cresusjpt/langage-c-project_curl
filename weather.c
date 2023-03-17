//
// Created by POEI on 17/03/2023.
//

#include "weather.h"

static size_t ecrire_forecast(char *contents, size_t size, size_t nmemb, void *userp)
{
    FILE *f = fopen("forecast.json","w");
    if (contents != NULL && f!=NULL){
        fputs(contents, f);
    }

    fclose(f);
    return size * nmemb;
}

int forecast(GPS gps){
    CURL *curl;
    CURLcode res;

    char *basic = "https://weatherbit-v1-mashape.p.rapidapi.com/forecast/3hourly?lat=&lon=";
    char *url = (char *) calloc(strlen(basic), sizeof(char));
    url = (char *) realloc(url,(strlen(basic)+strlen(gps.lat)+strlen(gps.lon)+1) * sizeof(char));

    sprintf(url, "https://weatherbit-v1-mashape.p.rapidapi.com/forecast/3hourly?lat=%s&lon=%s", gps.lat, gps.lon);

    fprintf(stdout,"%s", url);
    fprintf(stdout,"LAT %s", gps.lat);
    fprintf(stdout,"LON %s", gps.lon);

    curl = curl_easy_init();

    if(curl) {


        curl_easy_setopt(curl, CURLOPT_CUSTOMREQUEST, "GET");
        curl_easy_setopt(curl, CURLOPT_URL, url);
        struct curl_slist *headers = NULL;
        headers = curl_slist_append(headers, "X-RapidAPI-Key: 9a9df01b8fmsh58d79bff35e8fc1p1328f9jsnafa4ff9fb89e");
        headers = curl_slist_append(headers, "X-RapidAPI-Host: weatherbit-v1-mashape.p.rapidapi.com");
        curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);

        curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 0);
        curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0);
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, ecrire_forecast);

        /* Perform the request, res will get the return code */
        res = curl_easy_perform(curl);

        if(res != CURLE_OK)
            fprintf(stderr, "curl_easy_perform() in forecast failed: %s\n",
                    curl_easy_strerror(res));



        curl_easy_cleanup(curl);
    }

    return 0;
}
