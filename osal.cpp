#include "osal.h"

#ifdef _WIN32
#include <Windows.h>
extern "C" {

typedef struct {
    OsalThreadFnT pFn;
    void *pUser;
} OsalThreadData;

static DWORD WINAPI osalThread(LPVOID lpThreadParameter) {
  OsalThreadData *pData = (OsalThreadData *)lpThreadParameter;
  OsalThreadData data = *pData;
  free(pData);
  data.pFn(data.pUser);
  return 0;
}


void OSALStartThread(OsalThreadFnT const pFn, void * const pUserData) {
  OsalThreadData *pData = (OsalThreadData *)malloc(sizeof(OsalThreadData));
  pData->pFn = pFn;
  pData->pUser = pUserData;

  DWORD dwThread;
  // Start the thread...
  (void)CreateThread(
    NULL, 32768,
    osalThread, pData, 0,
    &dwThread);

}

}

#else // WIN32


#endif // WIN32
