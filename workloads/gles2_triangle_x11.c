#include <EGL/egl.h>
#include <GLES2/gl2.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

static const char *vertex_shader_src =
    "attribute vec2 a_pos;\n"
    "attribute vec3 a_color;\n"
    "varying vec3 v_color;\n"
    "void main(void) {\n"
    "  v_color = a_color;\n"
    "  gl_Position = vec4(a_pos, 0.0, 1.0);\n"
    "}\n";

static const char *fragment_shader_src =
    "precision mediump float;\n"
    "varying vec3 v_color;\n"
    "void main(void) {\n"
    "  gl_FragColor = vec4(v_color, 1.0);\n"
    "}\n";

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

static GLuint compile_shader(GLenum stage, const char *src) {
    GLuint shader = glCreateShader(stage);
    glShaderSource(shader, 1, &src, NULL);
    glCompileShader(shader);

    GLint ok = 0;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &ok);
    if (!ok) {
        char log[512];
        GLsizei size = 0;
        glGetShaderInfoLog(shader, sizeof(log), &size, log);
        fprintf(stderr, "shader compile failed: %.*s\n", size, log);
        exit(1);
    }
    return shader;
}

static GLuint build_program(void) {
    GLuint vs = compile_shader(GL_VERTEX_SHADER, vertex_shader_src);
    GLuint fs = compile_shader(GL_FRAGMENT_SHADER, fragment_shader_src);
    GLuint program = glCreateProgram();
    glAttachShader(program, vs);
    glAttachShader(program, fs);
    glBindAttribLocation(program, 0, "a_pos");
    glBindAttribLocation(program, 1, "a_color");
    glLinkProgram(program);

    GLint ok = 0;
    glGetProgramiv(program, GL_LINK_STATUS, &ok);
    if (!ok) {
        char log[512];
        GLsizei size = 0;
        glGetProgramInfoLog(program, sizeof(log), &size, log);
        fprintf(stderr, "program link failed: %.*s\n", size, log);
        exit(1);
    }

    glDeleteShader(vs);
    glDeleteShader(fs);
    return program;
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

    XStoreName(xdpy, win, "gles2_triangle_x11");
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

    GLuint program = build_program();
    glUseProgram(program);

    const GLfloat vertices[] = {
         0.0f,  0.7f, 1.0f, 0.0f, 0.0f,
        -0.7f, -0.7f, 0.0f, 1.0f, 0.0f,
         0.7f, -0.7f, 0.0f, 0.0f, 1.0f,
    };
    GLuint vbo = 0;

    glViewport(0, 0, 320, 240);
    glClearColor(0.08f, 0.08f, 0.08f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);

    glGenBuffers(1, &vbo);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(GLfloat), (const void *)0);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(GLfloat), (const void *)(2 * sizeof(GLfloat)));
    glEnableVertexAttribArray(0);
    glEnableVertexAttribArray(1);

    glDrawArrays(GL_TRIANGLES, 0, 3);
    require_egl(eglSwapBuffers(dpy, surface), "eglSwapBuffers");

    XFlush(xdpy);
    sleep(2);

    glDeleteBuffers(1, &vbo);
    glDeleteProgram(program);
    eglDestroyContext(dpy, ctx);
    eglDestroySurface(dpy, surface);
    eglTerminate(dpy);
    XDestroyWindow(xdpy, win);
    XCloseDisplay(xdpy);
    XFree(vis);
    return 0;
}
