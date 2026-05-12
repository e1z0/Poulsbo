#include <EGL/egl.h>
#include <GLES2/gl2.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

static void die(const char *msg) {
    fprintf(stderr, "%s\n", msg);
    exit(1);
}

static void require_egl(EGLBoolean ok, const char *what) {
    if (!ok) {
        fprintf(stderr, "%s failed: 0x%04x\n", what, eglGetError());
        exit(1);
    }
}

int main(void) {
    Display *xdpy = XOpenDisplay(NULL);
    if (xdpy == NULL) {
        die("XOpenDisplay failed");
    }

    int screen = DefaultScreen(xdpy);
    Window root = RootWindow(xdpy, screen);

    static const EGLint config_attrs[] = {
        EGL_SURFACE_TYPE, EGL_WINDOW_BIT,
        EGL_RENDERABLE_TYPE, EGL_OPENGL_ES2_BIT,
        EGL_RED_SIZE, 8,
        EGL_GREEN_SIZE, 8,
        EGL_BLUE_SIZE, 8,
        EGL_NONE,
    };

    EGLDisplay dpy = eglGetDisplay((EGLNativeDisplayType)xdpy);
    if (dpy == EGL_NO_DISPLAY) {
        die("eglGetDisplay failed");
    }

    EGLint major = 0;
    EGLint minor = 0;
    require_egl(eglInitialize(dpy, &major, &minor), "eglInitialize");

    EGLConfig config = NULL;
    EGLint num_configs = 0;
    require_egl(eglChooseConfig(dpy, config_attrs, &config, 1, &num_configs), "eglChooseConfig");
    if (num_configs != 1) {
        die("no EGL config");
    }

    EGLint vid = 0;
    require_egl(eglGetConfigAttrib(dpy, config, EGL_NATIVE_VISUAL_ID, &vid), "eglGetConfigAttrib");

    XVisualInfo vis_template = {0};
    vis_template.visualid = (VisualID)vid;
    int vis_count = 0;
    XVisualInfo *vis = XGetVisualInfo(xdpy, VisualIDMask, &vis_template, &vis_count);
    if (vis == NULL || vis_count < 1) {
        die("XGetVisualInfo failed");
    }

    Colormap cmap = XCreateColormap(xdpy, root, vis->visual, AllocNone);
    XSetWindowAttributes swa = {0};
    swa.colormap = cmap;
    swa.event_mask = ExposureMask | StructureNotifyMask;

    Window win = XCreateWindow(
        xdpy,
        root,
        0,
        0,
        320,
        240,
        0,
        vis->depth,
        InputOutput,
        vis->visual,
        CWColormap | CWEventMask,
        &swa
    );

    XStoreName(xdpy, win, "gles2_clear_x11");
    XMapWindow(xdpy, win);

    static const EGLint ctx_attrs[] = {
        EGL_CONTEXT_CLIENT_VERSION, 2,
        EGL_NONE,
    };

    EGLSurface surface = eglCreateWindowSurface(dpy, config, (EGLNativeWindowType)win, NULL);
    if (surface == EGL_NO_SURFACE) {
        die("eglCreateWindowSurface failed");
    }

    EGLContext ctx = eglCreateContext(dpy, config, EGL_NO_CONTEXT, ctx_attrs);
    if (ctx == EGL_NO_CONTEXT) {
        die("eglCreateContext failed");
    }

    require_egl(eglMakeCurrent(dpy, surface, surface, ctx), "eglMakeCurrent");

    glViewport(0, 0, 320, 240);
    glClearColor(0.10f, 0.35f, 0.85f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);
    require_egl(eglSwapBuffers(dpy, surface), "eglSwapBuffers");

    XFlush(xdpy);
    sleep(2);

    eglDestroyContext(dpy, ctx);
    eglDestroySurface(dpy, surface);
    eglTerminate(dpy);
    XDestroyWindow(xdpy, win);
    XCloseDisplay(xdpy);
    XFree(vis);
    return 0;
}
