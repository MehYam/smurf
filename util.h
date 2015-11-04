#ifndef _SMURF_UTIL_H
#define _SMURF_UTIL_H

#include <assert.h>
#include <sys/types.h>
#include <unistd.h>

#include "include/capi/cef_browser_capi.h"
#include "include/capi/cef_client_capi.h"
#include "include/capi/cef_life_span_handler_capi.h"

struct Client
{
	cef_browser_t *browser;
	cef_browser_host_t *host;
	cef_client_t *client;
	cef_life_span_handler_t *lsh;

	struct Client *next;
};

#define CEF_REQUIRE_UI_THREAD()       assert(cef_currently_on(TID_UI));
#define CEF_REQUIRE_IO_THREAD()       assert(cef_currently_on(TID_IO));
#define CEF_REQUIRE_FILE_THREAD()     assert(cef_currently_on(TID_FILE));
#define CEF_REQUIRE_RENDERER_THREAD() assert(cef_currently_on(TID_RENDERER));

#define INJECTED_JS_FUNCTION_NAME "nativeFunction"

void eprintf(const char *format, ...);
void die(const char *msg);
void getHomeDir(char* pszBuffer, size_t size);

#define DEBUG_ONCE(str, args...) do{ static int first_call = 1; if (first_call) { first_call = 0; eprintf("[%05d:%08x](%s:%d)%s(): "str, getpid(), pthread_self(), __FILE__, __LINE__, __func__, ##args); } }while(0)
#define DEBUG_PRINT(str, args...) eprintf("[%05d:%08x](%s:%d)%s(): "str, getpid(), pthread_self(), __FILE__, __LINE__, __func__, ##args);
#define LENGTH(x) (sizeof x / sizeof x[0])

#endif
