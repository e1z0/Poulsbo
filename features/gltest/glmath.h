/* glmath.h - tiny column-major 4x4 matrix helpers for the GL burn tests. */
#ifndef GLMATH_H
#define GLMATH_H
#include <math.h>
#include <string.h>

typedef float mat4[16];

static inline void m_identity(mat4 m){ memset(m,0,sizeof(mat4)); m[0]=m[5]=m[10]=m[15]=1.f; }

static inline void m_mul(mat4 r, const mat4 a, const mat4 b){
	mat4 t; int i,j,k;
	for(i=0;i<4;i++) for(j=0;j<4;j++){ float s=0;
		for(k=0;k<4;k++) s += a[k*4+j]*b[i*4+k];
		t[i*4+j]=s; }
	memcpy(r,t,sizeof(mat4));
}

static inline void m_perspective(mat4 m, float fovy_deg, float aspect, float n, float f){
	float t = 1.f/tanf(fovy_deg*(float)M_PI/360.f);
	memset(m,0,sizeof(mat4));
	m[0]=t/aspect; m[5]=t; m[10]=(f+n)/(n-f); m[11]=-1.f; m[14]=(2.f*f*n)/(n-f);
}
static inline void m_translate(mat4 m, float x, float y, float z){
	m_identity(m); m[12]=x; m[13]=y; m[14]=z;
}
static inline void m_rotate(mat4 m, float ang, float x, float y, float z){
	float c=cosf(ang), s=sinf(ang), l=sqrtf(x*x+y*y+z*z);
	if(l>0){x/=l;y/=l;z/=l;}
	m_identity(m);
	m[0]=x*x*(1-c)+c;   m[1]=y*x*(1-c)+z*s; m[2]=x*z*(1-c)-y*s;
	m[4]=x*y*(1-c)-z*s; m[5]=y*y*(1-c)+c;   m[6]=y*z*(1-c)+x*s;
	m[8]=x*z*(1-c)+y*s; m[9]=y*z*(1-c)-x*s; m[10]=z*z*(1-c)+c;
}
#endif
