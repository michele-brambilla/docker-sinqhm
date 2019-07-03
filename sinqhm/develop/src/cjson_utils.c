#include "cjson_utils.h"

double cjson_get_double(cJSON* const root,char* const field) {
  cJSON* const item = cJSON_GetObjectItem(root,field);
  if( item!= NULL ) {
    return item->valuedouble;
  }
  return -1;
}

int cjson_get_int(cJSON* const root,char* const field) {
  cJSON* const item = cJSON_GetObjectItem(root,field);
  if( item!= NULL ) {
    return item->valueint;
  }
  return -1;
}

int cjson_get_array_int(cJSON* const root,char* const field, int entry) {
  cJSON *item = cJSON_GetObjectItem(root,field);
  if( !item) {
    return -1;
  }
  cJSON *subitem = cJSON_GetArrayItem(item, entry);  
  if( !subitem ) {
    return -1;//subitem;
  }
  return subitem->valueint;
}

double cjson_get_array_double(cJSON* const root,char* const field, int entry) {
  cJSON *item = cJSON_GetObjectItem(root,field);
  if( !item) {
    return -1;
  }
  cJSON *subitem = cJSON_GetArrayItem(item, entry);  
  if( !subitem ) {
    return -1;//subitem;
  }
  return subitem->valuedouble;
}
