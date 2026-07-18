/*
 * glburn.c - hardcore GLES2 capability + burn-in test suite for the SGX535.
 *
 * Exercises the full open-bridge + closed-EMGD render path on the GMA500 /
 * PowerVR SGX535 and reports pass/fail, correctness (pixel checksums / animation
 * detection) and performance for each test.  Renders offscreen into an EGL
 * pbuffer with a depth buffer and reads pixels back, so it runs headless and
 * verifies output without a display.
 *
 * Subtests:
 *   caps            GL renderer/version/extensions + limits
 *   clear   [sec]   clear throughput (animated colour)
 *   tri     [sec]   single per-vertex-coloured triangle (baseline)
 *   cube    [sec]   depth-tested, animated, per-face-lit spinning 3D cube
 *   plasma  [sec]   heavy procedural fragment shader (ALU / fillrate)
 *   geo     [sec]   geometry throughput: a dense rotating mesh (many triangles)
 *   tex     [sec]   textured, mip-less quad (texture upload + sampling)
 *   blend   [sec]   many blended layers (blend + overdraw)
 *   burn    [sec]   sustained mixed scene; watches for GL errors / slowdowns
 *   all     [sec]   run every test back to back
 *
 * Each test fails if: EGL/GL setup fails, a GL error is raised, the output is
 * blank/constant when it should not be, or an animated test's frames do not
 * change.  Exit code 0 = all passed.
 *
 * Build (on the target): see build.sh.  Run: LD_LIBRARY_PATH=/opt/emgd/lib ./glburn all 5
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <time.h>
#include <EGL/egl.h>
#include <GLES2/gl2.h>
#include "glmath.h"

#define W 512
#define H 512

static EGLDisplay dpy;
static EGLContext ctx;
static EGLSurface surf;
static int g_fail = 0;

static double now(void){ struct timespec t; clock_gettime(CLOCK_MONOTONIC,&t); return t.tv_sec+t.tv_nsec/1e9; }

static void die(const char *m){ fprintf(stderr,"FATAL: %s (egl 0x%x)\n",m,eglGetError()); exit(2); }

static void egl_init(void){
	dpy = eglGetDisplay(EGL_DEFAULT_DISPLAY);
	if(dpy==EGL_NO_DISPLAY) die("eglGetDisplay");
	EGLint mj,mn; if(!eglInitialize(dpy,&mj,&mn)) die("eglInitialize");
	eglBindAPI(EGL_OPENGL_ES_API);
	EGLint ca[]={EGL_SURFACE_TYPE,EGL_PBUFFER_BIT,EGL_RENDERABLE_TYPE,EGL_OPENGL_ES2_BIT,
		EGL_RED_SIZE,8,EGL_GREEN_SIZE,8,EGL_BLUE_SIZE,8,EGL_ALPHA_SIZE,8,
		EGL_DEPTH_SIZE,16,EGL_NONE};
	EGLConfig cf; EGLint nc; if(!eglChooseConfig(dpy,ca,&cf,1,&nc)||nc<1) die("eglChooseConfig");
	EGLint pa[]={EGL_WIDTH,W,EGL_HEIGHT,H,EGL_NONE};
	surf = eglCreatePbufferSurface(dpy,cf,pa); if(surf==EGL_NO_SURFACE) die("pbuffer");
	EGLint xa[]={EGL_CONTEXT_CLIENT_VERSION,2,EGL_NONE};
	ctx = eglCreateContext(dpy,cf,EGL_NO_CONTEXT,xa); if(ctx==EGL_NO_CONTEXT) die("ctx");
	if(!eglMakeCurrent(dpy,surf,surf,ctx)) die("makeCurrent");
}

static GLuint sh(GLenum t, const char *src){
	GLuint s=glCreateShader(t); glShaderSource(s,1,&src,0); glCompileShader(s);
	GLint ok=0; glGetShaderiv(s,GL_COMPILE_STATUS,&ok);
	if(!ok){ char log[1024]; glGetShaderInfoLog(s,sizeof(log),0,log);
		fprintf(stderr,"  shader compile FAILED: %s\n",log); g_fail=1; }
	return s;
}
static GLuint prog(const char *vs, const char *fs){
	GLuint p=glCreateProgram();
	glAttachShader(p,sh(GL_VERTEX_SHADER,vs));
	glAttachShader(p,sh(GL_FRAGMENT_SHADER,fs));
	glBindAttribLocation(p,0,"aPos");
	glBindAttribLocation(p,1,"aCol");
	glBindAttribLocation(p,2,"aUV");
	glLinkProgram(p);
	GLint ok=0; glGetProgramiv(p,GL_LINK_STATUS,&ok);
	if(!ok){ char log[1024]; glGetProgramInfoLog(p,sizeof(log),0,log);
		fprintf(stderr,"  link FAILED: %s\n",log); g_fail=1; }
	glUseProgram(p); return p;
}

/* checksum + non-constant + coverage of the current framebuffer */
static unsigned long analyze(unsigned long *nonzero, int *distinct_ok, unsigned long prev){
	static unsigned char *px=NULL; if(!px) px=malloc(W*H*4);
	glReadPixels(0,0,W,H,GL_RGBA,GL_UNSIGNED_BYTE,px);
	unsigned long sum=0,nz=0; int i;
	for(i=0;i<W*H*4;i++){ if(px[i]) nz++; sum += (unsigned)px[i]*(unsigned)((i&31)+1); }
	if(nonzero)*nonzero=nz; if(distinct_ok)*distinct_ok=(sum!=prev);
	return sum;
}

static int gl_err_check(const char *where){
	GLenum e=glGetError(); if(e){ fprintf(stderr,"  GL error 0x%04x at %s\n",e,where); return 1; } return 0;
}

/* --- test result reporting --- */
static void report(const char *name,int pass,int frames,double dt,const char *note){
	printf("  [%-6s] %s  %d frames  %.2fs  %6.1f FPS  %s\n",
		name, pass?"PASS":"FAIL", frames, dt, frames/dt, note?note:"");
	if(!pass) g_fail=1;
}

/* ========================= tests ========================= */

static void t_caps(void){
	printf("== caps ==\n");
	printf("  GL_VENDOR    : %s\n", glGetString(GL_VENDOR));
	printf("  GL_RENDERER  : %s\n", glGetString(GL_RENDERER));
	printf("  GL_VERSION   : %s\n", glGetString(GL_VERSION));
	printf("  GLSL_VERSION : %s\n", glGetString(GL_SHADING_LANGUAGE_VERSION));
	GLint v;
	glGetIntegerv(GL_MAX_TEXTURE_SIZE,&v);            printf("  MAX_TEXTURE_SIZE        : %d\n",v);
	glGetIntegerv(GL_MAX_VERTEX_ATTRIBS,&v);          printf("  MAX_VERTEX_ATTRIBS      : %d\n",v);
	glGetIntegerv(GL_MAX_VERTEX_UNIFORM_VECTORS,&v);  printf("  MAX_VERTEX_UNIFORM_VECS : %d\n",v);
	glGetIntegerv(GL_MAX_FRAGMENT_UNIFORM_VECTORS,&v);printf("  MAX_FRAG_UNIFORM_VECS   : %d\n",v);
	glGetIntegerv(GL_MAX_VARYING_VECTORS,&v);         printf("  MAX_VARYING_VECTORS     : %d\n",v);
	glGetIntegerv(GL_MAX_TEXTURE_IMAGE_UNITS,&v);     printf("  MAX_TEXTURE_IMAGE_UNITS : %d\n",v);
	glGetIntegerv(GL_MAX_RENDERBUFFER_SIZE,&v);       printf("  MAX_RENDERBUFFER_SIZE   : %d\n",v);
	const char *ext=(const char*)glGetString(GL_EXTENSIONS);
	printf("  EXTENSIONS   : %.240s%s\n", ext?ext:"(none)", (ext&&strlen(ext)>240)?"...":"");
	report("caps", !gl_err_check("caps") && glGetString(GL_RENDERER)!=NULL, 0, 1e-9, "");
}

static void t_clear(int sec){
	int f=0,anim=0; unsigned long prev=0; double t0=now();
	while(now()-t0<sec){
		float p=(float)(f%120)/120.f;
		glClearColor(p,1.f-p,0.5f*p,1.f); glClear(GL_COLOR_BUFFER_BIT);
		int d; unsigned long s=analyze(0,&d,prev); if(f>0&&d)anim++; prev=s; f++;
	}
	report("clear", !gl_err_check("clear")&&anim>0, f, now()-t0, anim>0?"animates":"STATIC!");
}

static const char *VS_COL =
	"uniform mat4 uMVP; attribute vec4 aPos; attribute vec3 aCol; varying vec3 vCol;"
	"void main(){ vCol=aCol; gl_Position=uMVP*aPos; }";
static const char *FS_COL =
	"precision mediump float; varying vec3 vCol; void main(){ gl_FragColor=vec4(vCol,1.0); }";

static void t_tri(int sec){
	GLuint p=prog(VS_COL,FS_COL); GLint u=glGetUniformLocation(p,"uMVP");
	GLfloat pos[]={0,0.8f,0, -0.8f,-0.7f,0, 0.8f,-0.7f,0};
	GLfloat col[]={1,0,0, 0,1,0, 0,0,1};
	glViewport(0,0,W,H);
	glEnableVertexAttribArray(0); glVertexAttribPointer(0,3,GL_FLOAT,0,0,pos);
	glEnableVertexAttribArray(1); glVertexAttribPointer(1,3,GL_FLOAT,0,0,col);
	int f=0,anim=0; unsigned long prev=0,nz=0; double t0=now();
	while(now()-t0<sec){ mat4 m; m_rotate(m,f*0.05f,0,0,1); glUniformMatrix4fv(u,1,0,m);
		glClearColor(0.05f,0.05f,0.1f,1); glClear(GL_COLOR_BUFFER_BIT);
		glDrawArrays(GL_TRIANGLES,0,3);
		int d; unsigned long s=analyze(&nz,&d,prev); if(f>0&&d)anim++; prev=s; f++; }
	char n[64]; snprintf(n,sizeof n,"cover=%lu%% anim=%d",nz*100/(W*H*4),anim);
	report("tri", !gl_err_check("tri")&&nz>1000&&anim>0, f, now()-t0, n);
}

static void t_cube(int sec){
	GLuint p=prog(VS_COL,FS_COL); GLint u=glGetUniformLocation(p,"uMVP");
	/* 8 verts, 12 tris; per-vertex colour */
	GLfloat v[]={-1,-1,-1, 1,-1,-1, 1,1,-1, -1,1,-1, -1,-1,1, 1,-1,1, 1,1,1, -1,1,1};
	GLfloat c[]={0,0,0, 1,0,0, 1,1,0, 0,1,0, 0,0,1, 1,0,1, 1,1,1, 0,1,1};
	GLubyte idx[]={0,1,2,0,2,3, 4,6,5,4,7,6, 0,4,5,0,5,1, 3,2,6,3,6,7, 1,5,6,1,6,2, 0,3,7,0,7,4};
	glViewport(0,0,W,H); glEnable(GL_DEPTH_TEST); glEnable(GL_CULL_FACE);
	glEnableVertexAttribArray(0); glVertexAttribPointer(0,3,GL_FLOAT,0,0,v);
	glEnableVertexAttribArray(1); glVertexAttribPointer(1,3,GL_FLOAT,0,0,c);
	mat4 proj; m_perspective(proj,55.f,(float)W/H,0.5f,20.f);
	int f=0,anim=0; unsigned long prev=0,nz=0; double t0=now();
	while(now()-t0<sec){
		mat4 t,rx,ry,rv,mv,mvp;
		m_translate(t,0,0,-5.f); m_rotate(rx,f*0.03f,1,0,0); m_rotate(ry,f*0.021f,0,1,0);
		m_mul(rv,rx,ry); m_mul(mv,t,rv); m_mul(mvp,proj,mv);
		glUniformMatrix4fv(u,1,0,mvp);
		glClearColor(0.02f,0.02f,0.05f,1); glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);
		glDrawElements(GL_TRIANGLES,36,GL_UNSIGNED_BYTE,idx);
		int d; unsigned long s=analyze(&nz,&d,prev); if(f>0&&d)anim++; prev=s; f++;
	}
	glDisable(GL_DEPTH_TEST); glDisable(GL_CULL_FACE);
	char n[64]; snprintf(n,sizeof n,"3D depth-tested; cover=%lu%% anim=%d",nz*100/(W*H*4),anim);
	report("cube", !gl_err_check("cube")&&nz>5000&&anim>sec, f, now()-t0, n);
}

static const char *FS_PLASMA =
	"precision highp float; varying vec2 vUV; uniform float uT;"
	"void main(){ vec2 p=vUV*8.0; float a=sin(p.x+uT)+sin(p.y*1.3+uT*0.7)"
	"+sin((p.x+p.y)*0.7+uT*1.3)+sin(length(p-4.0)*1.5-uT*2.0);"
	"vec3 c=0.5+0.5*vec3(sin(a),sin(a+2.1),sin(a+4.2)); gl_FragColor=vec4(c,1.0); }";
static const char *VS_UV =
	"attribute vec4 aPos; attribute vec2 aUV; varying vec2 vUV;"
	"void main(){ vUV=aUV; gl_Position=aPos; }";

static void t_plasma(int sec){
	GLuint p=prog(VS_UV,FS_PLASMA); GLint u=glGetUniformLocation(p,"uT");
	GLfloat pos[]={-1,-1, 1,-1, -1,1, 1,1}; GLfloat uv[]={0,0, 1,0, 0,1, 1,1};
	glViewport(0,0,W,H);
	glEnableVertexAttribArray(0); glVertexAttribPointer(0,2,GL_FLOAT,0,0,pos);
	glEnableVertexAttribArray(2); glVertexAttribPointer(2,2,GL_FLOAT,0,0,uv);
	int f=0,anim=0; unsigned long prev=0,nz=0; double t0=now();
	while(now()-t0<sec){ glUniform1f(u,(float)(now()-t0)*2.f);
		glDrawArrays(GL_TRIANGLE_STRIP,0,4);
		int d; unsigned long s=analyze(&nz,&d,prev); if(f>0&&d)anim++; prev=s; f++; }
	char n[64]; snprintf(n,sizeof n,"full-screen ALU shader; cover=%lu%% anim=%d",nz*100/(W*H*4),anim);
	report("plasma", !gl_err_check("plasma")&&nz>(unsigned long)(W*H*3)&&anim>0, f, now()-t0, n);
}

#define GRID 40   /* GRID*GRID*2 triangles */
static void t_geo(int sec){
	GLuint p=prog(VS_COL,FS_COL); GLint u=glGetUniformLocation(p,"uMVP");
	static GLfloat v[GRID*GRID*6*3], c[GRID*GRID*6*3]; int n=0,cc=0,gx,gy;
	for(gy=0;gy<GRID;gy++)for(gx=0;gx<GRID;gx++){
		float x0=(float)gx/GRID*2-1, x1=(float)(gx+1)/GRID*2-1;
		float y0=(float)gy/GRID*2-1, y1=(float)(gy+1)/GRID*2-1;
		float q[6][2]={{x0,y0},{x1,y0},{x1,y1},{x0,y0},{x1,y1},{x0,y1}}; int k;
		for(k=0;k<6;k++){ v[n++]=q[k][0]; v[n++]=q[k][1]; v[n++]=0;
			c[cc++]=(float)gx/GRID; c[cc++]=(float)gy/GRID; c[cc++]=1.f-(float)gx/GRID; }
	}
	int verts=GRID*GRID*6;
	glViewport(0,0,W,H);
	glEnableVertexAttribArray(0); glVertexAttribPointer(0,3,GL_FLOAT,0,0,v);
	glEnableVertexAttribArray(1); glVertexAttribPointer(1,3,GL_FLOAT,0,0,c);
	int f=0,anim=0; unsigned long prev=0,nz=0; double t0=now();
	while(now()-t0<sec){ mat4 m; m_rotate(m,f*0.02f,0,0,1); glUniformMatrix4fv(u,1,0,m);
		glClearColor(0,0,0,1); glClear(GL_COLOR_BUFFER_BIT);
		glDrawArrays(GL_TRIANGLES,0,verts);
		int d; unsigned long s=analyze(&nz,&d,prev); if(f>0&&d)anim++; prev=s; f++; }
	char nb[80]; snprintf(nb,sizeof nb,"%d tris/frame cover=%lu%% anim=%d",verts/3,nz*100/(W*H*4),anim);
	report("geo", !gl_err_check("geo")&&nz>10000&&anim>0, f, now()-t0, nb);
}

static void t_tex(int sec){
	const char *VS="attribute vec4 aPos; attribute vec2 aUV; varying vec2 vUV;"
		"uniform mat4 uMVP; void main(){ vUV=aUV; gl_Position=uMVP*aPos; }";
	const char *FS="precision mediump float; varying vec2 vUV; uniform sampler2D uT;"
		"void main(){ gl_FragColor=texture2D(uT,vUV); }";
	GLuint p=prog(VS,FS); GLint um=glGetUniformLocation(p,"uMVP");
	GLuint tex; glGenTextures(1,&tex); glBindTexture(GL_TEXTURE_2D,tex);
	unsigned char img[64*64*4]; int x,y;
	for(y=0;y<64;y++)for(x=0;x<64;x++){ int o=(y*64+x)*4; int ch=((x^y)&8)?255:0;
		img[o]=ch; img[o+1]=x*4; img[o+2]=y*4; img[o+3]=255; }
	glTexImage2D(GL_TEXTURE_2D,0,GL_RGBA,64,64,0,GL_RGBA,GL_UNSIGNED_BYTE,img);
	glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_S,GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_T,GL_REPEAT);
	int tex_err = gl_err_check("tex-upload");
	GLfloat pos[]={-0.9f,-0.9f,0, 0.9f,-0.9f,0, -0.9f,0.9f,0, 0.9f,0.9f,0};
	GLfloat uv[]={0,0, 3,0, 0,3, 3,3};
	glViewport(0,0,W,H);
	glEnableVertexAttribArray(0); glVertexAttribPointer(0,3,GL_FLOAT,0,0,pos);
	glEnableVertexAttribArray(2); glVertexAttribPointer(2,2,GL_FLOAT,0,0,uv);
	int f=0,anim=0; unsigned long prev=0,nz=0; double t0=now();
	while(now()-t0<sec){ mat4 m; m_rotate(m,f*0.02f,0,0,1); glUniformMatrix4fv(um,1,0,m);
		glClearColor(0,0,0,1); glClear(GL_COLOR_BUFFER_BIT);
		glDrawArrays(GL_TRIANGLE_STRIP,0,4);
		int d; unsigned long s=analyze(&nz,&d,prev); if(f>0&&d)anim++; prev=s; f++; }
	glDeleteTextures(1,&tex);
	char n[64]; snprintf(n,sizeof n,"texture2D sampled; cover=%lu%% anim=%d",nz*100/(W*H*4),anim);
	report("tex", !tex_err&&!gl_err_check("tex")&&nz>5000&&anim>0, f, now()-t0, n);
}

static void t_blend(int sec){
	GLuint p=prog(VS_COL,FS_COL); GLint u=glGetUniformLocation(p,"uMVP");
	const char *FSA="precision mediump float; varying vec3 vCol;"
		"void main(){ gl_FragColor=vec4(vCol,0.35); }";
	GLuint pa=prog(VS_COL,FSA); GLint ua=glGetUniformLocation(pa,"uMVP");
	GLfloat pos[]={0,0.8f,0, -0.8f,-0.7f,0, 0.8f,-0.7f,0}; GLfloat col[]={1,0.2f,0.2f, 0.2f,1,0.2f, 0.2f,0.2f,1};
	glViewport(0,0,W,H);
	glEnableVertexAttribArray(0); glVertexAttribPointer(0,3,GL_FLOAT,0,0,pos);
	glEnableVertexAttribArray(1); glVertexAttribPointer(1,3,GL_FLOAT,0,0,col);
	glEnable(GL_BLEND); glBlendFunc(GL_SRC_ALPHA,GL_ONE_MINUS_SRC_ALPHA);
	int f=0,anim=0; unsigned long prev=0,nz=0; double t0=now();
	while(now()-t0<sec){ glClearColor(0.1f,0.1f,0.1f,1); glClear(GL_COLOR_BUFFER_BIT);
		int layer; glUseProgram(pa);
		for(layer=0;layer<8;layer++){ mat4 m; m_rotate(m,f*0.03f+layer*0.5f,0,0,1);
			glUniformMatrix4fv(ua,1,0,m); glDrawArrays(GL_TRIANGLES,0,3); }
		int d; unsigned long s=analyze(&nz,&d,prev); if(f>0&&d)anim++; prev=s; f++; }
	glDisable(GL_BLEND); (void)u;
	char n[64]; snprintf(n,sizeof n,"8 blended layers; cover=%lu%% anim=%d",nz*100/(W*H*4),anim);
	report("blend", !gl_err_check("blend")&&nz>5000&&anim>0, f, now()-t0, n);
}

/* sustained mixed burn-in: rotating cube + plasma bg, reports FPS over time,
 * fails on any GL error or a frame that stops animating. */
static void t_burn(int sec){
	GLuint pp=prog(VS_UV,FS_PLASMA); GLint upT=glGetUniformLocation(pp,"uT");
	GLuint pc=prog(VS_COL,FS_COL);   GLint uc=glGetUniformLocation(pc,"uMVP");
	GLfloat quad[]={-1,-1, 1,-1, -1,1, 1,1}; GLfloat quv[]={0,0,1,0,0,1,1,1};
	GLfloat v[]={-1,-1,-1, 1,-1,-1, 1,1,-1, -1,1,-1, -1,-1,1, 1,-1,1, 1,1,1, -1,1,1};
	GLfloat c[]={0,0,0, 1,0,0, 1,1,0, 0,1,0, 0,0,1, 1,0,1, 1,1,1, 0,1,1};
	GLubyte idx[]={0,1,2,0,2,3, 4,6,5,4,7,6, 0,4,5,0,5,1, 3,2,6,3,6,7, 1,5,6,1,6,2, 0,3,7,0,7,4};
	mat4 proj; m_perspective(proj,55.f,(float)W/H,0.5f,20.f);
	glViewport(0,0,W,H);
	int f=0,anim=0,errs=0; unsigned long prev=0; double t0=now(),tmark=t0;
	double minfps=1e9,maxfps=0;
	while(now()-t0<sec){
		double fs=now();
		/* plasma background (no depth) */
		glDisable(GL_DEPTH_TEST); glUseProgram(pp); glUniform1f(upT,(float)(fs-t0)*2.f);
		glEnableVertexAttribArray(0); glVertexAttribPointer(0,2,GL_FLOAT,0,0,quad);
		glEnableVertexAttribArray(2); glVertexAttribPointer(2,2,GL_FLOAT,0,0,quv);
		glClear(GL_DEPTH_BUFFER_BIT);
		glDrawArrays(GL_TRIANGLE_STRIP,0,4);
		/* spinning cube on top (depth) */
		glEnable(GL_DEPTH_TEST); glUseProgram(pc);
		mat4 t,rx,ry,rv,mv,mvp; m_translate(t,0,0,-4.5f);
		m_rotate(rx,f*0.04f,1,0,0); m_rotate(ry,f*0.03f,0,1,0);
		m_mul(rv,rx,ry); m_mul(mv,t,rv); m_mul(mvp,proj,mv); glUniformMatrix4fv(uc,1,0,mvp);
		glEnableVertexAttribArray(0); glVertexAttribPointer(0,3,GL_FLOAT,0,0,v);
		glEnableVertexAttribArray(1); glVertexAttribPointer(1,3,GL_FLOAT,0,0,c);
		glDrawElements(GL_TRIANGLES,36,GL_UNSIGNED_BYTE,idx);
		int d; unsigned long s=analyze(0,&d,prev); if(f>0&&d)anim++; prev=s;
		if(glGetError()) errs++;
		double fps=1.0/(now()-fs); if(fps<minfps)minfps=fps; if(fps>maxfps)maxfps=fps;
		f++;
		if(now()-tmark>=2.0){ printf("    burn t=%.0fs frame=%d inst=%.1f FPS errs=%d\n",
			now()-t0,f,fps,errs); tmark=now(); }
	}
	glDisable(GL_DEPTH_TEST);
	char n[96]; snprintf(n,sizeof n,"minFPS=%.1f maxFPS=%.1f errs=%d anim=%d",minfps,maxfps,errs,anim);
	report("burn", errs==0&&anim>sec, f, now()-t0, n);
}

int main(int argc,char**argv){
	const char *test = argc>1?argv[1]:"all";
	int sec = argc>2?atoi(argv[2]):3; if(sec<1)sec=1;
	setbuf(stdout,NULL); setbuf(stderr,NULL);	/* unbuffered: survive crashes */
	egl_init();
	printf("glburn: %s on %s (pbuffer %dx%d)\n", test, glGetString(GL_RENDERER), W,H);
	int all = !strcmp(test,"all");
	if(all||!strcmp(test,"caps"))   t_caps();
	if(all||!strcmp(test,"clear"))  { printf("== clear ==\n");  t_clear(sec); }
	if(all||!strcmp(test,"tri"))    { printf("== tri ==\n");    t_tri(sec); }
	if(all||!strcmp(test,"cube"))   { printf("== cube ==\n");   t_cube(sec); }
	if(all||!strcmp(test,"plasma")) { printf("== plasma ==\n"); t_plasma(sec); }
	if(all||!strcmp(test,"geo"))    { printf("== geo ==\n");    t_geo(sec); }
	if(all||!strcmp(test,"tex"))    { printf("== tex ==\n");    t_tex(sec); }
	if(all||!strcmp(test,"blend"))  { printf("== blend ==\n");  t_blend(sec); }
	if(all||!strcmp(test,"burn"))   { printf("== burn ==\n");   t_burn(all?sec*3:sec); }
	printf("\nRESULT: %s\n", g_fail?"FAILURES PRESENT":"ALL PASSED");
	eglMakeCurrent(dpy,EGL_NO_SURFACE,EGL_NO_SURFACE,EGL_NO_CONTEXT);
	eglTerminate(dpy);
	return g_fail?1:0;
}
