#include "curl/curl.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "lib/cisson.h"

//
// Created by POEI on 17/03/2023.
//

#ifndef CURL_GEOCODING_H
#define CURL_GEOCODING_H

#endif //CURL_GEOCODING_H

int getLatLong();
static size_t ecrire_geocode(char *contents, size_t size, size_t nmemb, void *userp);
void latLongFromAdress(char *street, char *city, char *countrycodes);
