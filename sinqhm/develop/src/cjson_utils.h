#ifndef _CJSON_UTILS_H_
#define _CJSON_UTILS_H_

#include "cJSON.h"

double cjson_get_double(cJSON* const root,char* const field);

int cjson_get_int(cJSON* const root,char* const field);

double cjson_get_array_double(cJSON* const root,char* const field, int entry);

int cjson_get_array_int(cJSON* const root,char* const field, int entry);

#endif
