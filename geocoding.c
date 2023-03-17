//
// Created by POEI on 17/03/2023.
//

#include "geocoding.h"

int getLatLong(){
    struct json_tree json_tree = { 0 };
    FILE *f = fopen("geocode.json","r");

    char c;
    int l = 1;
    char *str = (char *) calloc(l, sizeof(char));

    do{
        c = (char) fgetc(f);
        str[l-1] = c;
        str = (char *) realloc(str, (l+1) *  sizeof(char));
        l+=1;

        fprintf(stdout, "%s", str);

    }while(c != EOF);

    puts(str);

    rjson(str, &json_tree); /* rjson reads json into a tree */
    puts(to_string_pointer(&json_tree, query(&json_tree, "/lon")));
    puts(to_string_pointer(&json_tree, query(&json_tree, "/lat")));


    return 0;

}

static size_t ecrire_geocode(char *contents, size_t size, size_t nmemb, void *userp)
{
    FILE *f = fopen("geocode.json","w");
    if (contents != NULL && f!=NULL){
        fputs(contents, f);
    }

    getLatLong();


    return size * nmemb;
}

void latLongFromAdress(char *street, char *city, char *countrycodes){

    CURL *curl;
    CURLcode res;
    char *basic = "https://forward-reverse-geocoding.p.rapidapi.com/v1/forward?format=json&street=&city=&countrycodes=&polygon_threshold=0.0";
    char *url = (char *) calloc(strlen(basic), sizeof(char));
    url = (char *) realloc(url,(strlen(basic)+strlen(street)+strlen(city)+strlen(countrycodes)+1) * sizeof(char));

    sprintf(url, "https://forward-reverse-geocoding.p.rapidapi.com/v1/forward?format=json&street=%s&city=%s&countrycodes=%s&polygon_threshold=0.0",street, city, countrycodes);

    printf("%s", url);


    curl = curl_easy_init();

    if(curl) {

        curl_easy_setopt(curl, CURLOPT_CUSTOMREQUEST, "GET");

        curl_easy_setopt(curl, CURLOPT_URL, url);
        /* example.com is redirected, so we tell libcurl to follow redirection */
        //curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);

        struct curl_slist *headers = NULL;
        headers = curl_slist_append(headers, "X-RapidAPI-Key: 9a9df01b8fmsh58d79bff35e8fc1p1328f9jsnafa4ff9fb89e");
        headers = curl_slist_append(headers, "X-RapidAPI-Host: forward-reverse-geocoding.p.rapidapi.com");

        curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);

        curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 0);
        curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0);
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, ecrire_geocode);

        /* Perform the request, res will get the return code */
        res = curl_easy_perform(curl);

        if(res != CURLE_OK)
            fprintf(stderr, "curl_easy_perform() failed: %s\n",
                    curl_easy_strerror(res));



        curl_easy_cleanup(curl);
    }

}