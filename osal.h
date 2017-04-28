#ifndef OSAL_H__
#define OSAL_H__

typedef void (*OsalThreadFnT)(void *pUserData);

#ifdef __cplusplus
extern "C" {
#endif

void OSALStartThread(OsalThreadFnT const pFn, void * const pUserData);

#ifdef __cplusplus
}
#endif


#endif
