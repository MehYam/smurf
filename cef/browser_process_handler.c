#include <stdlib.h>

#include <string.h>
#include <pthread.h>
#include <unistd.h>
#include <errno.h>

#include <X11/Xlib.h>
#include <X11/Xatom.h>

#include <time.h>
#include <sys/time.h>
#include <linux/limits.h>

#include "include/capi/cef_app_capi.h"
#include "include/capi/cef_browser_process_handler_capi.h"
#include "include/capi/cef_client_capi.h"
#include "include/capi/cef_frame_capi.h"

#include "util.h"
#include "cef/base.h"
#include "cef/initializers.h"

static void configurenotify(struct Client *c, const XEvent *e);

extern struct Client* client_list;

static void (*handler[LASTEvent]) (struct Client *, const XEvent *) = {
//	[ClientMessage] = clientmessage,
	[ConfigureNotify] = configurenotify,
//	[ConfigureRequest] = configurerequest,
//	[DestroyNotify] = destroynotify,
//	[KeyPress] = keypress,
//	[PropertyNotify] = propertynotify,
};

static void configurenotify(struct Client *c, const XEvent *e)
{
	const XConfigureEvent *ev = &e->xconfigure;
	XWindowChanges changes = {0};

	DEBUG_PRINT("############ configurenotify ##################")

	if (ev->window == c->win && c->cwin && c->host && c->host->notify_move_or_resize_started) {
		c->host->notify_move_or_resize_started(c->host);
		changes.width = ev->width;
		changes.height = ev->height;
		XConfigureWindow(c->dpy, c->cwin, CWHeight | CWWidth, &changes);
	}
}

const char* XEVENT_NAMES[] =
{
"0",
"1",
"KeyPress",
"KeyRelease",
"ButtonPress",
"ButtonRelease",
"MotionNotify",
"EnterNotify",
"LeaveNotify",
"FocusIn",
"FocusOut",
"KeymapNotify",
"Expose",
"GraphicsExpose",
"NoExpose",
"VisibilityNotify",
"CreateNotify",
"DestroyNotify",
"UnmapNotify",
"MapNotify",
"MapRequest",
"ReparentNotify",
"ConfigureNotify",
"ConfigureRequest",
"GravityNotify",
"ResizeRequest",
"CirculateNotify",
"CirculateRequest",
"PropertyNotify",
"SelectionClear",
"SelectionRequest",
"SelectionNotify",
"ColormapNotify",
"ClientMessage",
"MappingNotify",
"GenericEvent"
};

#define LOAD_LOCAL_TEST_FILE 0
#define RUN_TIMER_TEST 0
static void *runx(void *arg)
{
	DEBUG_PRINT("------------ starting X handling thread --------------");

	struct Client *c = (struct Client*)arg;
	cef_window_info_t windowInfo = {};

	cef_browser_settings_t browserSettings = {.size = sizeof(cef_browser_settings_t)};
	XEvent ev;

	if (!(c->dpy = XOpenDisplay(NULL)))
	{
		die("Can't open display\n");
	}
	c->scr = XDefaultScreen(c->dpy);

	c->attrs.event_mask = 
		Button1MotionMask |
		Button2MotionMask | 
		Button3MotionMask |
		Button4MotionMask |
		Button5MotionMask |
		ButtonMotionMask |
		ButtonPressMask |
		ButtonReleaseMask |
		ColormapChangeMask |
		EnterWindowMask |
		ExposureMask |
		FocusChangeMask |
		KeyPressMask |
		KeyReleaseMask |
		KeymapStateMask |
		LeaveWindowMask |
		OwnerGrabButtonMask |
		PointerMotionHintMask |
		PointerMotionMask |
		PropertyChangeMask |
		ResizeRedirectMask |
		StructureNotifyMask |
		SubstructureNotifyMask |
//		SubstructureRedirectMask |
		VisibilityChangeMask
		;

	const int black = BlackPixel(c->dpy, c->scr);
	const int white = WhitePixel(c->dpy, c->scr);
	c->win = XCreateSimpleWindow(
		c->dpy,
		DefaultRootWindow(c->dpy),
		0, 0, 1200, 800,
		0,
		black,
		white
	);		
	DEBUG_PRINT("WINID: %d", c->win);

	c->gc = XCreateGC(c->dpy, c->win, 0, 0);
	XSetBackground(c->dpy, c->gc, white);
	XSetForeground(c->dpy, c->gc, black);

	// request the window closed message so we can exit the thread
	Atom wm_delete_window = XInternAtom(c->dpy, "WM_DELETE_WINDOW", False);
	XSetWMProtocols(c->dpy, c->win, &wm_delete_window, 1);

	XClearWindow(c->dpy, c->win);
	XMapWindow(c->dpy, c->win);
	XSync(c->dpy, False);

	windowInfo.parent_window = c->win;
	c->client = init_client();

	RINC(c->client);

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

	cef_browser_host_create_browser(&windowInfo, c->client, &cefUrl, &browserSettings, NULL);

	DEBUG_PRINT("browser creation requested");

	const int x11fd = ConnectionNumber(c->dpy);

	// for the timeout of select()
	struct timeval tv;

#if RUN_TIMER_TEST
	// for checking time using a tickcount
	const time_t tStart = time(NULL);
#endif

	int running = 1;
	while(running) {
		fd_set x11fdset;
		FD_ZERO(&x11fdset);
		FD_SET(x11fd, &x11fdset);

		tv.tv_usec = 0;
		tv.tv_sec = 1;

		if (select(x11fd + 1, &x11fdset, 0, 0, &tv))
		{
			XNextEvent(c->dpy, &ev);

			DEBUG_PRINT("window, event: %d, %s", c->win, XEVENT_NAMES[ev.type]);
			if(handler[ev.type]) {
				(handler[ev.type])(c, &ev);
			}
			if (ev.type == ClientMessage && ev.xclient.data.l[0] == wm_delete_window) {
				DEBUG_PRINT("destroying window");

				XDestroyWindow(c->dpy, c->win);
				running = 0;
			}
		}
		else
		{
#if RUN_TIMER_TEST			
			const time_t tNow = time(NULL);
			if (difftime(tNow, tStart) > 10.0)
			{
				DEBUG_PRINT("second test - XMapWindow");
				XMapWindow(c->dpy, c->win);
				XFlush(c->dpy);
			}
			else if (difftime(tNow, tStart) > 5)
			{
				DEBUG_PRINT("first test - XUnmapWindow");
				XUnmapWindow(c->dpy, c->win);
				XFlush(c->dpy);
			}
#endif			
		}
	}
	DEBUG_PRINT("exiting X event loop");

	XFreeGC(c->dpy, c->gc);
	XDestroyWindow(c->dpy, c->win);
	XCloseDisplay(c->dpy);
	return NULL;
}

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

	if (0 != (errno = pthread_create(&c->thread, NULL, &runx, (void*)c))) {
		eprintf("pthread_create failed:");
		return;
	}
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
