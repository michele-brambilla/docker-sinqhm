

////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////// C API /////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////


typedef struct 
{ 	
  long			accessErrors;			// Access violations 
	long			activeRequests;			// Currently active requests
	long			maxActiveRequests;		// Currently active requests
	long			errors;					// General errors 
	long			keptAlive;				// Requests service on keep-alive
	long			requests;				// Total requests
	long			redirects;				// Redirections 
	long			timeouts;				// Request timeouts
	long			copyDown;				// Times buffer had to copy down
} hostStats_type;

#ifdef USE_CAPI
  MprVar *maGetVariables(MaRequest *rq);
  char *maGetDocumentRoot(MaServer *srv);
  char *maGetServerRoot(MaServer *srv);
  void maGetHostStats(MaServer *srv, hostStats_type *hstats);
#endif
