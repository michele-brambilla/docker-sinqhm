#include <stdio.h>
#include <time.h>

#include <bson.h>
#include <bcon.h>
#include <mongoc.h>

#include "mongo.h"

static int mongohm_status_flag = MONGOHM_NOT_INITIALIZED;

#ifdef USE_MONGO_LOG

/* new cluster, test */
/* const char * sinqhm_db_name = "sinqTestDB"; */
/* const char * sinqhm_collection_name = "log_test"; */
/* new cluster, production */
const char * sinqhm_db_name = "sinqProdDB01";
const char * sinqhm_collection_name = "log_test";

/* old cluster */
/* const char * sinqhm_db_name = "rita2"; */
/* const char * sinqhm_collection_name = "log_rita2_sinqhm"; */

const char * inst = "rita2";


mongoc_client_t *client = 0;
mongoc_database_t *database = 0;
mongoc_collection_t *collection = 0;

void mongohm_init(const char * addr, 
		  const char * db_name, 
		  const char * collection_name);

void log_json(char * output,
	      const int severity, 
	      const char * sub, 
	      const char * message);

void log_bson(bson_t * output,
	      const int severity, 
	      const char * sub, 
	      const char * message);

void mongohm_init(const char* addr, const char* db_name, const char* collection_name) {
  mongoc_init();
  if( !client ) {
    client = mongoc_client_new (addr);
  }
  if( !client ) {
    perror("Error: cannot connect to mongodb\n");
    mongohm_status_flag = MONGOHM_INIT_FAILURE;
    return;
  } 
  if( !database ) {
    if( !(database = mongoc_client_get_database (client, db_name) )) {
      perror("Error: cannot connect to mongodb database\n");
      mongohm_status_flag = MONGOHM_INIT_FAILURE;
      return;
    }
  }
  if( !collection ) {
    if( !(collection = mongoc_client_get_collection (client, db_name, collection_name) )) {
      perror("Error: cannot connect to mongodb collection\n");
      mongohm_status_flag = MONGOHM_INIT_FAILURE;
      return;
    }
  }

  mongohm_status_flag = MONGOHM_INITIALIZED;
  return;
}

void mongohm_cleanup() {
  if(mongohm_status_flag != MONGOHM_INITIALIZED) {
    return;
  }
  if(collection) {
    mongoc_collection_destroy (collection);
  }
  if(database) {
    mongoc_database_destroy (database);
  }
  if(client) {
    mongoc_client_destroy (client);
  }
  mongoc_cleanup ();
  return;
}

void log_json(char * output,
	      const int severity, 
	      const char * sub, 
	      const char * message) {
  
  double timestamp;
  char * timetext;
  time_t rawtime;
  struct timespec tp;
  struct tm *timeinfo;

  timetext = calloc(128,sizeof(char));

  clock_gettime(CLOCK_MONOTONIC, &tp);
  timestamp = tp.tv_sec + tp.tv_nsec *1e-9;

  time(&rawtime);
  timeinfo = localtime (&rawtime);

  strftime (timetext,128,"%FT%H:%M:%S",timeinfo);
  sprintf(timetext,"%s.%ld",timetext,tp.tv_nsec);

  sprintf(output,
	  "{\"sub\":\"%s\",\"timestamp\":%lf,\"severity\":%d,\"timetext\":\"%s\",\"message\":\"%s\"}",
	  sub, timestamp, severity, timetext, message);

  free(timetext);
  return;
}


void log_bson(bson_t * output,
	      const int severity, 
	      const char * sub, 
	      const char * message) {
  
  bson_oid_t oid;
  double timestamp;
  char * timetext;
  time_t rawtime;
  struct timespec tp;
  struct tm *timeinfo;

  timetext = calloc(128,sizeof(char));

  clock_gettime(CLOCK_MONOTONIC, &tp);
  timestamp = tp.tv_sec + tp.tv_nsec *1e-9;

  time(&rawtime);
  timeinfo = localtime (&rawtime);

  strftime (timetext,128,"%FT%H-%M-%S",timeinfo);
  sprintf(timetext,"%s.%ld",timetext,tp.tv_nsec);

  bson_oid_init (&oid, NULL);
  BSON_APPEND_OID (output, "_id", &oid);
  BSON_APPEND_DOUBLE(output,"timestamp",timestamp);
  BSON_APPEND_INT32(output,"level",severity);
  BSON_APPEND_UTF8(output,"_timetext",timetext);
  BSON_APPEND_UTF8(output,"_sub",sub);
  BSON_APPEND_UTF8(output,"short_message",message);
  BSON_APPEND_UTF8(output,"version","1.1");
  BSON_APPEND_UTF8(output,"host",inst);

  free(timetext);
  return;
}


void mongohm_log(const int level, const char* subsystem, const char * msg) {

  /* char * json; */
  bson_error_t error;
  bson_t *message;
  
  if( mongohm_status_flag == MONGOHM_INIT_FAILURE ) {
    return;
  }
  if(mongohm_status_flag == MONGOHM_NOT_INITIALIZED) {

    /* new cluster, production db */
    mongohm_init( "mongodb://sinqUserRW:8Mh7]YFUE6JP]dQ7@mgdbaas001:27017,mgdbaas002:27017,mgdbaas003:27017/sinqProdDB01?replicaSet=rs_prod-DataScience-01&connectTimeoutMS=300&authSource=sinqProdDB01",sinqhm_db_name,sinqhm_collection_name);

    /* new cluster, test db */
    /* mongohm_init( "mongodb://userRW:2yfbt4AwBw4RY7j@mgdbaas001:27017,mgdbaas002:27017,mgdbaas003:27017/testDB?replicaSet=rs_prod-DataScience-01&connectTimeoutMS=300&authSource=sinqTestDB",sinqhm_db_name,sinqhm_collection_name); */

    /* old cluster */
    /* mongohm_init("mongodb://logwriter:sinqsics@mpc1965:27017/?authSource=admin",sinqhm_db_name,sinqhm_collection_name); */

    printf("\n");
    printf("Initialise mongodb:\n");
    printf("> db\t\t: %s\n",sinqhm_db_name);
    printf("> collection\t: %s\n",sinqhm_collection_name);
    printf("> initialised\t: %d\n",
	   (mongohm_status_flag == MONGOHM_INITIALIZED));
    printf("\n");
  }

  /* json = calloc(1024,sizeof(char)); */
  /* log_json(json, level, subsystem, msg); */
  /* message = bson_new_from_json ((const uint8_t *)json, -1, &error); */
  message = bson_new();
  log_bson(message, level, subsystem, msg);
  
  
  if(!message) {
    fprintf(stderr, "%s\n", error.message);
    mongohm_status_flag = MONGOHM_MESSAGE_ERROR;
    return;
  }
  if (!mongoc_collection_insert (collection, MONGOC_INSERT_NONE, message, NULL, &error)) {
    mongohm_status_flag = MONGOHM_MESSAGE_ERROR;
    fprintf (stderr, "%s\n", error.message);
  }
  if(message) {
    bson_destroy (message);
  }
  //  free(json);

  return;
}


int mongohm_ok() {
  return mongohm_status_flag == MONGOHM_INITIALIZED;
}

#else

void mongohm_cleanup() { return; }
void mongohm_log(const int level, const char * sub, const char * msg) { mongohm_status_flag = MONGOHM_NOT_INITIALIZED; return; }
int mongohm_ok() { return 0; }

#endif
