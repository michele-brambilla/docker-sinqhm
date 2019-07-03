
#include	<http.h>
#include	"appWrapper.h"

////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////// C API /////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

extern "C" 
{

  MprVar *maGetVariables(MaRequest *rq) 
  {
	  return rq->getVariables();
  }

  char *maGetDocumentRoot(MaServer *srv)
  {
	  MaHost *hst;
    hst = srv->getDefaultHost();

	  return hst->getDocumentRoot();
  }

#ifdef APPWEB_VER_2_0_4
  char *maGetServerRoot(MaServer *srv)
  {
    return srv->getServerRoot();
  }
#endif

  void maGetHostStats(MaServer *srv, hostStats_type *hstats)
  {
	  MaHost *hst;
    hst = srv->getDefaultHost();

    hstats->accessErrors      = hst->stats.accessErrors;
    hstats->activeRequests    = hst->stats.activeRequests;
    hstats->maxActiveRequests = hst->stats.maxActiveRequests;
    hstats->errors            = hst->stats.errors;
    hstats->keptAlive         = hst->stats.keptAlive;
    hstats->requests          = hst->stats.requests;
    hstats->redirects         = hst->stats.redirects;
    hstats->timeouts          = hst->stats.timeouts;
    hstats->copyDown          = hst->stats.copyDown;
  }
}

