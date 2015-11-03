#include <stdlib.h>
#include <string.h>

#include <linux/limits.h>

#include "include/capi/cef_app_capi.h"
#include "include/capi/cef_browser_process_handler_capi.h"
#include "include/capi/cef_client_capi.h"
#include "include/capi/cef_frame_capi.h"

#include "util.h"
#include "cef/base.h"
#include "cef/initializers.h"

extern struct Client* client_list;

#define LOAD_LOCAL_TEST_FILE 0

CEF_CALLBACK void browser_process_handler_on_context_initialized(struct _cef_browser_process_handler_t *self)
{
	struct Client *c;

	DEBUG_ONCE("");

	if (!(c = calloc(sizeof(struct Client), 1))) {
		eprintf("calloc failed:");
		return;
	}

	c->next = client_list;
	client_list = c;

	cef_window_info_t windowInfo;
	memset(&windowInfo, 0, sizeof(windowInfo));

#if LOAD_LOCAL_TEST_FILE
	char path[PATH_MAX] = "";
	getcwd(path, LENGTH(path));

	char url[PATH_MAX] = "file://";
	strcat(url, path);
	strcat(url, "/test.html");
#else
	char url[PATH_MAX] = "http://www.w3schools.com/";
#endif
	DEBUG_PRINT("setting url %s", url);
	
	cef_string_t cefUrl = {};
	cef_string_utf8_to_utf16(url, strlen(url), &cefUrl);

	cef_browser_settings_t browserSettings;
	memset(&browserSettings, 0, sizeof(browserSettings));
	browserSettings.size = sizeof(browserSettings);

	cef_browser_host_create_browser(&windowInfo, c->client, &cefUrl, &browserSettings, NULL);
}

CEF_CALLBACK void browser_process_handler_on_before_child_process_launch(struct _cef_browser_process_handler_t *self, struct _cef_command_line_t *command_line)
{
	DEBUG_ONCE("");
	RDEC(command_line);
}

CEF_CALLBACK void browser_process_handler_on_render_process_thread_created(struct _cef_browser_process_handler_t *self, struct _cef_list_value_t *extra_info)
{
	DEBUG_ONCE("");
	RDEC(extra_info);
}

CEF_CALLBACK struct _cef_print_handler_t *browser_process_handler_get_print_handler(struct _cef_browser_process_handler_t *self)
{
	DEBUG_ONCE("");
	return NULL;
}

struct _cef_browser_process_handler_t *init_browser_process_handler()
{
	struct _cef_browser_process_handler_t *ret = NULL;
	struct refcount *r = NULL;
	char *cp = NULL;

	DEBUG_ONCE("");
	if (!(r = calloc(sizeof(struct refcount) + sizeof(struct _cef_browser_process_handler_t), 1))) {
		DEBUG_PRINT("out of memory");
		return NULL;
	}

	cp = (char*)r;
	cp += sizeof(struct refcount);
	ret = (struct _cef_browser_process_handler_t*)cp;

	if(!init_base((cef_base_t*)ret, sizeof(struct _cef_browser_process_handler_t))) {
		free(r);
		return NULL;
	}
	ret->base.add_ref((cef_base_t*)ret);

	// callbacks
	ret->get_print_handler = &browser_process_handler_get_print_handler;
	ret->on_context_initialized = &browser_process_handler_on_context_initialized;
	ret->on_render_process_thread_created = &browser_process_handler_on_render_process_thread_created;
	ret->on_before_child_process_launch = &browser_process_handler_on_before_child_process_launch;

	return ret;
}
