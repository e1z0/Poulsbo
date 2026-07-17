#include <stdio.h>
#include <time.h>
#include <EGL/egl.h>
#include <GLES2/gl2.h>
#include <X11/Xlib.h>
static double now(){struct timespec t;clock_gettime(1,&t);return t.tv_sec+t.tv_nsec/1e9;}
static const char*VS="attribute vec4 p;void main(){gl_Position=p;}";
static const char*FS="precision mediump float;void main(){gl_FragColor=vec4(0.5,0.5,1.0,1.0);}";
static GLuint sh(GLenum t,const char*s){GLuint o=glCreateShader(t);glShaderSource(o,1,&s,0);glCompileShader(o);return o;}
int main(){
 Display*xd=XOpenDisplay(0); int scr=DefaultScreen(xd);
 Window w=XCreateSimpleWindow(xd,RootWindow(xd,scr),0,0,256,256,0,0,0);
 XMapWindow(xd,w); XFlush(xd);
 EGLDisplay d=eglGetDisplay((EGLNativeDisplayType)xd); eglInitialize(d,0,0); eglBindAPI(EGL_OPENGL_ES_API);
 EGLint ca[]={EGL_SURFACE_TYPE,EGL_WINDOW_BIT,EGL_RENDERABLE_TYPE,EGL_OPENGL_ES2_BIT,EGL_RED_SIZE,8,EGL_NONE};
 EGLConfig cf;EGLint nc;eglChooseConfig(d,ca,&cf,1,&nc);
 EGLSurface su=eglCreateWindowSurface(d,cf,(EGLNativeWindowType)w,0);
 EGLint xa[]={EGL_CONTEXT_CLIENT_VERSION,2,EGL_NONE};
 EGLContext cx=eglCreateContext(d,cf,0,xa); eglMakeCurrent(d,su,su,cx);
 GLuint pr=glCreateProgram();glAttachShader(pr,sh(GL_VERTEX_SHADER,VS));glAttachShader(pr,sh(GL_FRAGMENT_SHADER,FS));
 glBindAttribLocation(pr,0,"p");glLinkProgram(pr);glUseProgram(pr);
 GLfloat v[]={0,0.7,-0.7,-0.6,0.7,-0.6}; glViewport(0,0,256,256);
 glEnableVertexAttribArray(0);glVertexAttribPointer(0,2,GL_FLOAT,0,0,v);
 double tr=0,ts=0; int N=1500;
 for(int f=0;f<N;f++){
   double a=now(); glClear(GL_COLOR_BUFFER_BIT); glDrawArrays(GL_TRIANGLES,0,3); glFinish();
   double b=now(); eglSwapBuffers(d,su); double c=now();
   tr+=b-a; ts+=c-b;
 }
 printf("PROF: render+finish=%.1fms/frame  swap(flip)=%.1fms/frame  total=%.1f FPS\n",
   tr/N*1000, ts/N*1000, N/(tr+ts));
 return 0;
}
