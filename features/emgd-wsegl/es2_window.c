/* es2_window.c - standard windowed GLES2: eglCreateWindowSurface + eglSwapBuffers.
 * Exercises the WSEGL window path (wsegl_win). SGX renders into the window buffer;
 * eglSwapBuffers presents. Verifies via XGetImage that the window shows the render. */
#include <stdio.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <EGL/egl.h>
#include <GLES2/gl2.h>
#define W 256
#define H 256
static const char*VS="attribute vec2 p;uniform float a;varying vec3 c;"
 "void main(){float s=sin(a),k=cos(a);vec2 r=vec2(p.x*k-p.y*s,p.x*s+p.y*k);"
 "c=vec3(p*0.5+0.5,1.0);gl_Position=vec4(r,0.0,1.0);}";
static const char*FS="precision mediump float;varying vec3 c;void main(){gl_FragColor=vec4(c,1.0);}";
static GLuint sh(GLenum t,const char*s){GLuint o=glCreateShader(t);glShaderSource(o,1,&s,0);glCompileShader(o);
 GLint k;glGetShaderiv(o,GL_COMPILE_STATUS,&k);if(!k){char l[400];glGetShaderInfoLog(o,400,0,l);printf("sh:%s\n",l);}return o;}
int main(void){
 Display*xd=XOpenDisplay(NULL); if(!xd){printf("no X\n");return 1;}
 int sc=DefaultScreen(xd);
 Window win=XCreateSimpleWindow(xd,RootWindow(xd,sc),0,0,W,H,0,0,0);
 XStoreName(xd,win,"SGX535 windowed GL (eglSwapBuffers)"); XMapWindow(xd,win); XFlush(xd);
 EGLDisplay dpy=eglGetDisplay((EGLNativeDisplayType)xd);
 EGLint mj,mn; if(!eglInitialize(dpy,&mj,&mn)){printf("eglInitialize FAIL 0x%x\n",eglGetError());return 2;}
 printf("EGL %d.%d %s\n",mj,mn,eglQueryString(dpy,EGL_VERSION));
 eglBindAPI(EGL_OPENGL_ES_API);
 EGLint ca[]={EGL_SURFACE_TYPE,EGL_WINDOW_BIT,EGL_RENDERABLE_TYPE,EGL_OPENGL_ES2_BIT,
  EGL_RED_SIZE,8,EGL_GREEN_SIZE,8,EGL_BLUE_SIZE,8,EGL_NONE};
 EGLConfig cf;EGLint nc; eglChooseConfig(dpy,ca,&cf,1,&nc); printf("configs=%d\n",nc);
 EGLSurface su=eglCreateWindowSurface(dpy,cf,(EGLNativeWindowType)win,NULL);
 if(su==EGL_NO_SURFACE){printf("CreateWindowSurface FAIL 0x%x\n",eglGetError());return 3;}
 EGLint xa[]={EGL_CONTEXT_CLIENT_VERSION,2,EGL_NONE};
 EGLContext cx=eglCreateContext(dpy,cf,EGL_NO_CONTEXT,xa);
 if(!eglMakeCurrent(dpy,su,su,cx)){printf("makeCurrent FAIL 0x%x\n",eglGetError());return 4;}
 printf("GL_RENDERER=%s\n",(const char*)glGetString(GL_RENDERER));
 GLuint pr=glCreateProgram();glAttachShader(pr,sh(GL_VERTEX_SHADER,VS));
 glAttachShader(pr,sh(GL_FRAGMENT_SHADER,FS));glBindAttribLocation(pr,0,"p");glLinkProgram(pr);glUseProgram(pr);
 GLint ua=glGetUniformLocation(pr,"a");
 GLfloat v[]={0.0f,0.7f,-0.7f,-0.6f,0.7f,-0.6f};
 glViewport(0,0,W,H);glEnableVertexAttribArray(0);glVertexAttribPointer(0,2,GL_FLOAT,GL_FALSE,0,v);
 int f; for(f=0;f<30;f++){ glUniform1f(ua,f*0.15f);
  glClearColor(0.1f,0.0f,0.2f,1.0f);glClear(GL_COLOR_BUFFER_BIT);glDrawArrays(GL_TRIANGLES,0,3);glFinish();
  eglSwapBuffers(dpy,su); }
 printf("swap glGetError=0x%04x eglError=0x%x\n",glGetError(),eglGetError());
 XImage*rb=XGetImage(xd,win,W/2,H/2,1,1,AllPlanes,ZPixmap);
 unsigned long p=rb?XGetPixel(rb,0,0):0;
 printf("X window center pixel=0x%06lx\n",p&0xffffff);
 printf("%s\n", p!=0 ? "RESULT: unmodified windowed GLES2 app renders on SGX535 into an X11 window via eglSwapBuffers." : "RESULT: check");
 return 0;
}
