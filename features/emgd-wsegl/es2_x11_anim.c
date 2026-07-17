/* es2_x11_anim.c - sustained, animated 3D on X11 via the SGX535.
 * A spinning, per-vertex-coloured triangle rendered by GLES2 on the SGX each
 * frame and presented into a visible X11 window (XShmPutImage where available,
 * else XPutImage). Proves the complete windowed 3D pipeline runs continuously:
 * shaders + rasterization on the SGX, N frames, animation verified (frames
 * differ), FPS reported. */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <time.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/extensions/XShm.h>
#include <sys/shm.h>
#include <EGL/egl.h>
#include <GLES2/gl2.h>

#define W 256
#define H 256
#define NFRAMES 120

static const char *VS =
	"attribute vec2 p; uniform float a; varying vec3 c;"
	"void main(){ float s=sin(a),co=cos(a);"
	" vec2 r=vec2(p.x*co-p.y*s, p.x*s+p.y*co);"
	" c=vec3(p*0.5+0.5,1.0); gl_Position=vec4(r,0.0,1.0);}";
static const char *FS =
	"precision mediump float; varying vec3 c;"
	"void main(){ gl_FragColor=vec4(c,1.0);}";

static GLuint sh(GLenum t,const char*s){GLuint o=glCreateShader(t);glShaderSource(o,1,&s,0);
	glCompileShader(o);GLint k=0;glGetShaderiv(o,GL_COMPILE_STATUS,&k);
	if(!k){char l[512];glGetShaderInfoLog(o,512,0,l);printf("shader: %s\n",l);}return o;}

static double now(void){struct timespec t;clock_gettime(CLOCK_MONOTONIC,&t);
	return t.tv_sec+t.tv_nsec/1e9;}

int main(void){
	Display *xd=XOpenDisplay(NULL);
	if(!xd){printf("no X display\n");return 1;}
	int scr=DefaultScreen(xd);
	Window win=XCreateSimpleWindow(xd,RootWindow(xd,scr),0,0,W,H,0,0,BlackPixel(xd,scr));
	XStoreName(xd,win,"SGX535 GLES2 (spinning triangle)");
	XMapWindow(xd,win);XFlush(xd);
	GC gc=XCreateGC(xd,win,0,0);

	/* try MIT-SHM for a fast present */
	int useshm=XShmQueryExtension(xd);
	XShmSegmentInfo shm; XImage *img=NULL;
	if(useshm){
		img=XShmCreateImage(xd,DefaultVisual(xd,scr),24,ZPixmap,0,&shm,W,H);
		shm.shmid=shmget(IPC_PRIVATE,img->bytes_per_line*img->height,IPC_CREAT|0777);
		shm.shmaddr=img->data=shmat(shm.shmid,0,0);
		shm.readOnly=False;
		if(!XShmAttach(xd,&shm)){useshm=0;}
		XSync(xd,False);
	}
	unsigned char *xbuf = useshm ? (unsigned char*)img->data : malloc(W*H*4);
	if(!useshm){ img=XCreateImage(xd,DefaultVisual(xd,scr),24,ZPixmap,0,(char*)xbuf,W,H,32,0);}
	printf("present path: %s\n", useshm?"XShmPutImage (shared memory)":"XPutImage");

	EGLDisplay dpy=eglGetDisplay(EGL_DEFAULT_DISPLAY);
	EGLint mj,mn; if(!eglInitialize(dpy,&mj,&mn)){printf("eglInitialize failed\n");return 2;}
	eglBindAPI(EGL_OPENGL_ES_API);
	EGLint ca[]={EGL_SURFACE_TYPE,EGL_PBUFFER_BIT,EGL_RENDERABLE_TYPE,EGL_OPENGL_ES2_BIT,
		EGL_RED_SIZE,8,EGL_GREEN_SIZE,8,EGL_BLUE_SIZE,8,EGL_NONE};
	EGLConfig cf;EGLint nc;eglChooseConfig(dpy,ca,&cf,1,&nc);
	EGLint pa[]={EGL_WIDTH,W,EGL_HEIGHT,H,EGL_NONE};
	EGLSurface su=eglCreatePbufferSurface(dpy,cf,pa);
	EGLint xa[]={EGL_CONTEXT_CLIENT_VERSION,2,EGL_NONE};
	EGLContext cx=eglCreateContext(dpy,cf,EGL_NO_CONTEXT,xa);
	eglMakeCurrent(dpy,su,su,cx);
	printf("GL_RENDERER = %s\n",(const char*)glGetString(GL_RENDERER));

	GLuint pr=glCreateProgram();
	glAttachShader(pr,sh(GL_VERTEX_SHADER,VS));
	glAttachShader(pr,sh(GL_FRAGMENT_SHADER,FS));
	glBindAttribLocation(pr,0,"p");glLinkProgram(pr);glUseProgram(pr);
	GLint ua=glGetUniformLocation(pr,"a");
	GLfloat v[]={0.0f,0.75f,-0.75f,-0.65f,0.75f,-0.65f};
	glViewport(0,0,W,H);glEnableVertexAttribArray(0);
	glVertexAttribPointer(0,2,GL_FLOAT,GL_FALSE,0,v);

	unsigned char *px=malloc(W*H*4);
	unsigned long sum0=0,sumN=0; int distinct=0; unsigned long prev=0; double t0=now();
	for(int f=0;f<NFRAMES;f++){
		glUniform1f(ua,f*0.10f);
		glClearColor(0.05f,0.05f,0.10f,1.0f);glClear(GL_COLOR_BUFFER_BIT);
		glDrawArrays(GL_TRIANGLES,0,3);glFinish();
		glReadPixels(0,0,W,H,GL_RGBA,GL_UNSIGNED_BYTE,px);
		unsigned long s=0;
		for(int y=0;y<H;y++)for(int x=0;x<W;x++){       /* RGBA->BGRX, flip Y + checksum */
			unsigned char*sp=&px[((H-1-y)*W+x)*4],*d=&xbuf[(y*W+x)*4];
			d[0]=sp[2];d[1]=sp[1];d[2]=sp[0];d[3]=255;
			s += sp[0]*3u + sp[1]*5u + sp[2]*7u + (unsigned)(x&15);
		}
		if(useshm)XShmPutImage(xd,win,gc,img,0,0,0,0,W,H,False);
		else XPutImage(xd,win,gc,img,0,0,0,0,W,H);
		XFlush(xd);
		if(f==0)sum0=s; if(f==NFRAMES-1)sumN=s;
		if(f>0 && s!=prev) distinct++;   /* frame changed vs previous */
		prev=s;
	}
	double dt=now()-t0;
	printf("rendered %d frames in %.2fs = %.1f FPS\n",NFRAMES,dt,NFRAMES/dt);
	printf("frame checksum: f0=0x%lx fN=0x%lx ; %d/%d frames changed vs previous\n",
		sum0,sumN,distinct,NFRAMES-1);
	printf("%s\n", (distinct > NFRAMES/2 && sum0!=sumN) ?
		"RESULT: SGX535 SUSTAINED ANIMATED 3D rendering in an X11 window (frames animate)." :
		"RESULT: see values");
	XSync(xd,False);
	return 0;
}
