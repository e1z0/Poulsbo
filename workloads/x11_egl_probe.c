#include <EGL/egl.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>

#include <dlfcn.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

typedef unsigned int GLenum;
typedef unsigned int GLbitfield;
typedef float GLfloat;
typedef unsigned char GLubyte;

#define GL_VENDOR 0x1f00
#define GL_RENDERER 0x1f01
#define GL_VERSION 0x1f02
#define GL_COLOR_BUFFER_BIT 0x00004000

typedef const GLubyte *(*pfn_glGetString)(GLenum name);
typedef void (*pfn_glClearColor)(GLfloat red, GLfloat green,
				 GLfloat blue, GLfloat alpha);
typedef void (*pfn_glClear)(GLbitfield mask);
typedef void (*pfn_glViewport)(int x, int y, int width, int height);
typedef void (*pfn_glFinish)(void);

static void die(const char *msg)
{
	fprintf(stderr, "%s\n", msg);
	exit(1);
}

static void require_egl(EGLBoolean ok, const char *what)
{
	if (!ok) {
		fprintf(stderr, "%s failed: 0x%04x\n", what, eglGetError());
		exit(1);
	}
}

static const char *safe_str(const char *s)
{
	return s ? s : "(null)";
}

static void *resolve_gles_symbol(void *gles_handle, const char *name)
{
	void *sym = NULL;

	if (gles_handle)
		sym = dlsym(gles_handle, name);
	if (!sym)
		sym = (void *)eglGetProcAddress(name);
	return sym;
}

int main(void)
{
	Display *xdpy;
	int screen;
	Window root;
	EGLDisplay dpy;
	EGLint major = 0;
	EGLint minor = 0;
	EGLConfig config = NULL;
	EGLint num_configs = 0;
	EGLint visual_id = 0;
	EGLint config_id = 0;
	EGLint renderable_type = 0;
	EGLint surface_type = 0;
	EGLint native_renderable = 0;
	XVisualInfo vis_template;
	XVisualInfo *vis;
	int vis_count = 0;
	Colormap cmap;
	XSetWindowAttributes swa;
	Window win;
	EGLSurface surface;
	EGLContext ctx;
	void *gles_handle = NULL;
	pfn_glGetString gl_get_string;
	pfn_glClearColor gl_clear_color;
	pfn_glClear gl_clear;
	pfn_glViewport gl_viewport;
	pfn_glFinish gl_finish;
	const GLubyte *gl_vendor;
	const GLubyte *gl_renderer;
	const GLubyte *gl_version;
	static const EGLint config_attrs[] = {
		EGL_SURFACE_TYPE, EGL_WINDOW_BIT,
		EGL_RENDERABLE_TYPE, EGL_OPENGL_ES2_BIT,
		EGL_RED_SIZE, 8,
		EGL_GREEN_SIZE, 8,
		EGL_BLUE_SIZE, 8,
		EGL_NONE,
	};
	static const EGLint ctx_attrs[] = {
		EGL_CONTEXT_CLIENT_VERSION, 2,
		EGL_NONE,
	};

	printf("display_env=%s\n", safe_str(getenv("DISPLAY")));
	printf("xauthority_env=%s\n", safe_str(getenv("XAUTHORITY")));

	xdpy = XOpenDisplay(NULL);
	if (!xdpy)
		die("XOpenDisplay failed");

	screen = DefaultScreen(xdpy);
	root = RootWindow(xdpy, screen);

	printf("x_display=%s\n", DisplayString(xdpy));
	printf("x_server_vendor=%s\n", ServerVendor(xdpy));
	printf("x_protocol=%d.%d\n", ProtocolVersion(xdpy),
	       ProtocolRevision(xdpy));
	printf("x_screen=%d\n", screen);
	printf("x_root_size=%dx%d\n", DisplayWidth(xdpy, screen),
	       DisplayHeight(xdpy, screen));
	printf("x_default_depth=%d\n", DefaultDepth(xdpy, screen));

	dpy = eglGetDisplay((EGLNativeDisplayType)xdpy);
	if (dpy == EGL_NO_DISPLAY)
		die("eglGetDisplay failed");

	require_egl(eglInitialize(dpy, &major, &minor), "eglInitialize");
	printf("egl_version=%d.%d\n", major, minor);
	printf("egl_vendor=%s\n", safe_str(eglQueryString(dpy, EGL_VENDOR)));
	printf("egl_version_string=%s\n",
	       safe_str(eglQueryString(dpy, EGL_VERSION)));
	printf("egl_client_apis=%s\n",
	       safe_str(eglQueryString(dpy, EGL_CLIENT_APIS)));

	require_egl(eglChooseConfig(dpy, config_attrs, &config, 1, &num_configs),
		    "eglChooseConfig");
	printf("egl_config_count=%d\n", num_configs);
	if (num_configs < 1)
		die("no EGL window/ES2 config");

	require_egl(eglGetConfigAttrib(dpy, config, EGL_CONFIG_ID, &config_id),
		    "eglGetConfigAttrib(EGL_CONFIG_ID)");
	require_egl(eglGetConfigAttrib(dpy, config, EGL_NATIVE_VISUAL_ID,
				       &visual_id),
		    "eglGetConfigAttrib(EGL_NATIVE_VISUAL_ID)");
	require_egl(eglGetConfigAttrib(dpy, config, EGL_RENDERABLE_TYPE,
				       &renderable_type),
		    "eglGetConfigAttrib(EGL_RENDERABLE_TYPE)");
	require_egl(eglGetConfigAttrib(dpy, config, EGL_SURFACE_TYPE,
				       &surface_type),
		    "eglGetConfigAttrib(EGL_SURFACE_TYPE)");
	require_egl(eglGetConfigAttrib(dpy, config, EGL_NATIVE_RENDERABLE,
				       &native_renderable),
		    "eglGetConfigAttrib(EGL_NATIVE_RENDERABLE)");

	printf("egl_config_id=%d\n", config_id);
	printf("egl_native_visual_id=0x%x\n", visual_id);
	printf("egl_renderable_type=0x%x\n", renderable_type);
	printf("egl_surface_type=0x%x\n", surface_type);
	printf("egl_native_renderable=%d\n", native_renderable);

	memset(&vis_template, 0, sizeof(vis_template));
	vis_template.visualid = (VisualID)visual_id;
	vis = XGetVisualInfo(xdpy, VisualIDMask, &vis_template, &vis_count);
	if (!vis || vis_count < 1)
		die("XGetVisualInfo failed");

	printf("x_visual_depth=%d\n", vis->depth);
	printf("x_visual_id=0x%lx\n", (unsigned long)vis->visualid);

	cmap = XCreateColormap(xdpy, root, vis->visual, AllocNone);
	memset(&swa, 0, sizeof(swa));
	swa.colormap = cmap;
	swa.event_mask = ExposureMask | StructureNotifyMask;

	win = XCreateWindow(xdpy, root, 16, 16, 256, 128, 0, vis->depth,
			    InputOutput, vis->visual,
			    CWColormap | CWEventMask, &swa);
	XStoreName(xdpy, win, "x11_egl_probe");
	XMapWindow(xdpy, win);
	XFlush(xdpy);

	surface = eglCreateWindowSurface(dpy, config, (EGLNativeWindowType)win,
					 NULL);
	if (surface == EGL_NO_SURFACE)
		die("eglCreateWindowSurface failed");

	require_egl(eglBindAPI(EGL_OPENGL_ES_API), "eglBindAPI");
	ctx = eglCreateContext(dpy, config, EGL_NO_CONTEXT, ctx_attrs);
	if (ctx == EGL_NO_CONTEXT)
		die("eglCreateContext failed");

	require_egl(eglMakeCurrent(dpy, surface, surface, ctx), "eglMakeCurrent");
	printf("egl_make_current=ok\n");

	gles_handle = dlopen("libGLESv2.so.2", RTLD_LAZY | RTLD_LOCAL);
	if (!gles_handle)
		gles_handle = dlopen("libGLESv2.so", RTLD_LAZY | RTLD_LOCAL);

	printf("gles_library=%s\n", gles_handle ? "present" : "missing");

	gl_get_string = (pfn_glGetString)resolve_gles_symbol(gles_handle,
							     "glGetString");
	gl_clear_color = (pfn_glClearColor)resolve_gles_symbol(gles_handle,
							      "glClearColor");
	gl_clear = (pfn_glClear)resolve_gles_symbol(gles_handle, "glClear");
	gl_viewport = (pfn_glViewport)resolve_gles_symbol(gles_handle,
							  "glViewport");
	gl_finish = (pfn_glFinish)resolve_gles_symbol(gles_handle, "glFinish");

	if (!gl_get_string || !gl_clear_color || !gl_clear || !gl_viewport) {
		printf("gles_entrypoints=missing\n");
	} else {
		gl_vendor = gl_get_string(GL_VENDOR);
		gl_renderer = gl_get_string(GL_RENDERER);
		gl_version = gl_get_string(GL_VERSION);

		printf("gl_vendor=%s\n",
		       gl_vendor ? (const char *)gl_vendor : "(null)");
		printf("gl_renderer=%s\n",
		       gl_renderer ? (const char *)gl_renderer : "(null)");
		printf("gl_version=%s\n",
		       gl_version ? (const char *)gl_version : "(null)");

		gl_viewport(0, 0, 256, 128);
		gl_clear_color(0.15f, 0.45f, 0.20f, 1.0f);
		gl_clear(GL_COLOR_BUFFER_BIT);
		if (gl_finish)
			gl_finish();
		require_egl(eglSwapBuffers(dpy, surface), "eglSwapBuffers");
		printf("probe_clear_swap=ok\n");
	}

	if (gles_handle)
		dlclose(gles_handle);
	sleep(1);
	eglMakeCurrent(dpy, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);
	eglDestroyContext(dpy, ctx);
	eglDestroySurface(dpy, surface);
	eglTerminate(dpy);
	XDestroyWindow(xdpy, win);
	XCloseDisplay(xdpy);
	XFree(vis);
	return 0;
}
