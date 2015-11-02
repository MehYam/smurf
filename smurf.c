#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <string.h>

#include <linux/limits.h>

#include "include/capi/cef_app_capi.h"
#include "include/capi/cef_client_capi.h"

#include "util.h"
#include "cef/base.h"
#include "cef/initializers.h"

int main(int argc, char** argv)
{
	DEBUG_PRINT("----------------- NEW PROCESS -------------------");
	for (int i = 0; i < argc; ++i) {
		DEBUG_PRINT("%s ", argv[i]);
	}
	cef_main_args_t mainArgs = {};

	mainArgs.argc = argc;
	mainArgs.argv = argv;

	// Application handler and its callbacks.
	// cef_app_t structure must be filled. It must implement
	// reference counting. You cannot pass a structure
	// initialized with zeroes.
	struct _cef_app_t* const app = init_app();

	// Execute subprocesses.
	RINC(app);
	int code = cef_execute_process(&mainArgs, app, NULL);
	if (code >= 0) {
		DEBUG_PRINT("-------- cef_execute_process > 0, EXITING");
		return code;
	}
	DEBUG_PRINT("-------- cef_execute_process RETURNED 0, this is the BROWSER process, running CEF message loop");

	// Application settings.
	// It is mandatory to set the "size" member.
	cef_settings_t settings = {};
	memset(&settings, 0, sizeof(settings));
	settings.size = sizeof(cef_settings_t);
//	settings.no_sandbox = 1;

	char homeDir[PATH_MAX] = "";
	getHomeDir(homeDir, LENGTH(homeDir));

	char cachePath[PATH_MAX] = "";
	strncat(cachePath, homeDir, LENGTH(cachePath));
	strncat(cachePath, "/.cache/ericsson/mediafirstuc", LENGTH(cachePath));

	DEBUG_PRINT("cache dir '%s'", cachePath);
	cef_string_utf8_to_utf16(cachePath, strlen(cachePath), &settings.cache_path);

	DEBUG_PRINT("cef string length %d", settings.cache_path.length);

	// Initialize CEF.
//	printf("cef_initialize\n");
	RINC(app);
	cef_initialize(&mainArgs, &settings, app, NULL);

	// Message loop.
	DEBUG_PRINT("-------- cef_run_message_loop");
	cef_run_message_loop();

	// Shutdown CEF.
	DEBUG_PRINT("----------------- cef_shutdown -----------------");
	cef_shutdown();
	DEBUG_PRINT("----------------- cef_shutdown returned --------");
	return 0;
}
