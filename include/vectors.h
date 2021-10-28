#pragma once

#include <stdint.h>
#include <math.h>
#include <xmmintrin.h>

#define FORCE_INLINE  __forceinline

//Helpers
#define M_PI          3.14159265358979323846f
#define DEG2RAD(_a)   ((_a)*M_PI/180.0f) 
#define RAD2DEG(_a)   ((_a)*180.0f/M_PI)
#define INT_MIN       (-2147483647 - 1)
#define INT_MAX       2147483647
#define FLT_MAX       3.402823466e+38F

//Shuffle helpers
//Examples: SHUFFLE3(v, 0,1,2) leaves the vector unchanged.
//          SHUFFLE3(v, 0,0,0) replicates the X coordinate throughout the vector.
#define SHUFFLE2(V, X,Y) vec2(_mm_shuffle_ps((V).m, (V).m, _MM_SHUFFLE(Y,Y,Y,X)))
#define SHUFFLE3(V, X,Y,Z) vec3(_mm_shuffle_ps((V).m, (V).m, _MM_SHUFFLE(Z,Z,Y,X)))
#define SHUFFLE4(V, X,Y,Z,W) vec4(_mm_shuffle_ps((V).m, (V).m, _MM_SHUFFLE(W,Z,Y,X)))

struct vec2
{   
	__m128 m;

	FORCE_INLINE vec2() {}	
	FORCE_INLINE explicit vec2(const float *p) { m = _mm_set_ps(p[1], p[1], p[1], p[0]); }
	FORCE_INLINE explicit vec2(float x, float y) { m = _mm_set_ps(y, y, y, x); }
	FORCE_INLINE explicit vec2(__m128 v) { m = v; }

	FORCE_INLINE float x() const { return _mm_cvtss_f32(m); }
	FORCE_INLINE float y() const { return _mm_cvtss_f32(_mm_shuffle_ps(m, m, _MM_SHUFFLE(1, 1, 1, 1))); }
	
	FORCE_INLINE vec2 yx() const { return SHUFFLE2(*this, 1, 0); }
	FORCE_INLINE vec2 xx() const { return SHUFFLE2(*this, 0, 0); }
	FORCE_INLINE vec2 yy() const { return SHUFFLE2(*this, 1, 1); }

	//unaligned store	
	FORCE_INLINE void store(float *p) const { p[0] = x(); p[1] = y();}

	void setX(float x)
	{ 
		m = _mm_move_ss(m, _mm_set_ss(x)); 
	}
	void setY(float y)
	{
		__m128 t = _mm_move_ss(m, _mm_set_ss(y));
		t = _mm_shuffle_ps(t, t, _MM_SHUFFLE(3, 2, 0, 0));
		m = _mm_move_ss(t, m);
	}
	
	FORCE_INLINE float operator[] (size_t i) const { return m.m128_f32[i]; };
	FORCE_INLINE float& operator[] (size_t i) { return m.m128_f32[i]; };
};

struct vec3
{   
	__m128 m;

	FORCE_INLINE vec3() {}	
	FORCE_INLINE explicit vec3(const float *p){ m = _mm_set_ps(p[2], p[2], p[1], p[0]); }
	FORCE_INLINE explicit vec3(float x, float y, float z) { m = _mm_set_ps(z, z, y, x); }
	FORCE_INLINE explicit vec3(__m128 v) { m = v; }
	FORCE_INLINE explicit vec3(vec2 v, float z) {m = _mm_add_ps(v.m, _mm_set_ps(0.0f, 0.0f, z , 0.0f));}

	FORCE_INLINE float x() const { return _mm_cvtss_f32(m); }
	FORCE_INLINE float y() const { return _mm_cvtss_f32(_mm_shuffle_ps(m, m, _MM_SHUFFLE(1, 1, 1, 1))); }
	FORCE_INLINE float z() const { return _mm_cvtss_f32(_mm_shuffle_ps(m, m, _MM_SHUFFLE(2, 2, 2, 2))); }
	
	FORCE_INLINE vec3 yzx() const { return SHUFFLE3(*this, 1, 2, 0); }
	FORCE_INLINE vec3 zxy() const { return SHUFFLE3(*this, 2, 0, 1); }
	
	FORCE_INLINE void store(float *p) const { p[0] = x(); p[1] = y(); p[2] = z(); }

	void setX(float x)
	{
		m = _mm_move_ss(m, _mm_set_ss(x));
	}
	
	void setY(float y)
	{
		__m128 t = _mm_move_ss(m, _mm_set_ss(y));
		t = _mm_shuffle_ps(t, t, _MM_SHUFFLE(3, 2, 0, 0));
		m = _mm_move_ss(t, m);
	}
	
	void setZ(float z)
	{
		__m128 t = _mm_move_ss(m, _mm_set_ss(z));
		t = _mm_shuffle_ps(t, t, _MM_SHUFFLE(3, 0, 1, 0));
		m = _mm_move_ss(t, m);
	}
	
	FORCE_INLINE float operator[] (size_t i) const { return m.m128_f32[i]; };
	FORCE_INLINE float& operator[] (size_t i) { return m.m128_f32[i]; };
};

struct vec4
{   
	__m128 m;

	FORCE_INLINE vec4() {}	
	FORCE_INLINE explicit vec4(const float *p){ m = _mm_set_ps(p[3], p[2], p[1], p[0]); }
	FORCE_INLINE explicit vec4(float x, float y, float z, float w) { m = _mm_set_ps(w, z, y, x); }
	FORCE_INLINE explicit vec4(__m128 v) { m = v; }
	FORCE_INLINE explicit vec4(vec2 v) { m = v.m;}
	FORCE_INLINE explicit vec4(vec2 v, float z, float w) {m = _mm_add_ps(v.m, _mm_set_ps(0.0f, 0.0f, z , w));}
	FORCE_INLINE explicit vec4(vec3 v) { m = v.m;}
	FORCE_INLINE explicit vec4(vec3 v, float w) { m = _mm_add_ps(v.m, _mm_set_ps(0.0f, 0.0f, 0.0f, w));}


	FORCE_INLINE float x() const { return _mm_cvtss_f32(m); }
	FORCE_INLINE float y() const { return _mm_cvtss_f32(_mm_shuffle_ps(m, m, _MM_SHUFFLE(1, 1, 1, 1))); }
	FORCE_INLINE float z() const { return _mm_cvtss_f32(_mm_shuffle_ps(m, m, _MM_SHUFFLE(2, 2, 2, 2))); }
	FORCE_INLINE float w() const { return _mm_cvtss_f32(_mm_shuffle_ps(m, m, _MM_SHUFFLE(3, 3, 3, 3))); }
	
	//TODO: add more swizzles
	FORCE_INLINE vec4 wyzx() const { return SHUFFLE4(*this, 3, 1, 2, 0); }
	FORCE_INLINE vec4 wzxy() const { return SHUFFLE4(*this, 3, 2, 0, 1); }
	
	FORCE_INLINE void store(float *p) const { p[0] = x(); p[1] = y(); p[2] = z(); p[3] = w(); }

	void setX(float x)
	{
		m = _mm_move_ss(m, _mm_set_ss(x));
	}
	
	void setY(float y)
	{
		__m128 t = _mm_move_ss(m, _mm_set_ss(y));
		t = _mm_shuffle_ps(t, t, _MM_SHUFFLE(3, 2, 0, 0));
		m = _mm_move_ss(t, m);
	}
	
	void setZ(float z)
	{
		__m128 t = _mm_move_ss(m, _mm_set_ss(z));
		t = _mm_shuffle_ps(t, t, _MM_SHUFFLE(3, 0, 1, 0));
		m = _mm_move_ss(t, m);
	}
	
	void setW(float w)
	{
		__m128 t = _mm_move_ss(m, _mm_set_ss(w));
		t = _mm_shuffle_ps(t, t, _MM_SHUFFLE(0, 2, 1, 0));
		m = _mm_move_ss(t, m);
	}
	
	FORCE_INLINE float operator[] (size_t i) const { return m.m128_f32[i]; };
	FORCE_INLINE float& operator[] (size_t i) { return m.m128_f32[i]; };
};

//Helpers to load integer arguments to avoid manual casting
FORCE_INLINE vec2 vec2i(int x, int y) { return vec2((float)x, (float)y); }
FORCE_INLINE vec3 vec3i(int x, int y, int z) { return vec3((float)x, (float)y, (float)z);}
FORCE_INLINE vec4 vec4i(int x, int y, int z, int w) { return vec4((float)x, (float)y, (float)z, (float)w);}

// Helpers for initializing static data.
#define VCONST extern const __declspec(selectany)
struct vconstu
{
	union { uint32_t u[4]; __m128 v; };
	inline operator __m128() const { return v; }
};

VCONST vconstu vsignbits = { 0x80000000, 0x80000000, 0x80000000, 0x80000000 };


typedef vec2 bool2; 
typedef vec3 bool3;
typedef vec4 bool4; 


//vec2 common operators:
FORCE_INLINE vec2  operator+ (vec2 a, vec2 b) {a.m = _mm_add_ps(a.m, b.m); return a;} 
FORCE_INLINE vec2  operator- (vec2 a, vec2 b) {a.m = _mm_sub_ps(a.m, b.m); return a;}
FORCE_INLINE vec2  operator* (vec2 a, vec2 b) {a.m = _mm_mul_ps(a.m, b.m); return a;}
FORCE_INLINE vec2  operator/ (vec2 a, vec2 b) {a.m = _mm_div_ps(a.m, b.m); return a;}
FORCE_INLINE vec2  operator* (vec2 a, float  b) {a.m = _mm_mul_ps(a.m, _mm_set1_ps(b)); return a;}
FORCE_INLINE vec2  operator/ (vec2 a, float  b) {a.m = _mm_div_ps(a.m, _mm_set1_ps(b)); return a;}
FORCE_INLINE vec2  operator* (float  a, vec2 b) {b.m = _mm_mul_ps(_mm_set1_ps(a), b.m); return b;}
FORCE_INLINE vec2  operator/ (float  a, vec2 b) {b.m = _mm_div_ps(_mm_set1_ps(a), b.m); return b;}
FORCE_INLINE vec2& operator+= (vec2 &a, vec2 b) {a = a + b; return a;}
FORCE_INLINE vec2& operator-= (vec2 &a, vec2 b) {a = a - b; return a;} 
FORCE_INLINE vec2& operator*= (vec2 &a, vec2 b) {a = a * b; return a;}
FORCE_INLINE vec2& operator/= (vec2 &a, vec2 b) {a = a / b; return a;}
FORCE_INLINE vec2& operator*= (vec2 &a, float  b) {a = a * b; return a;}
FORCE_INLINE vec2& operator/= (vec2 &a, float  b) {a = a / b; return a;}
FORCE_INLINE bool2 operator== (vec2 a, vec2 b) {a.m = _mm_cmpeq_ps(a.m, b.m); return a;}
FORCE_INLINE bool2 operator!= (vec2 a, vec2 b) {a.m = _mm_cmpneq_ps(a.m, b.m); return a;}
FORCE_INLINE bool2 operator< (vec2 a, vec2 b) {a.m = _mm_cmplt_ps(a.m, b.m); return a;}
FORCE_INLINE bool2 operator> (vec2 a, vec2 b) {a.m = _mm_cmpgt_ps(a.m, b.m); return a;}
FORCE_INLINE bool2 operator<= (vec2 a, vec2 b) {a.m = _mm_cmple_ps(a.m, b.m); return a;}
FORCE_INLINE bool2 operator>= (vec2 a, vec2 b) {a.m = _mm_cmpge_ps(a.m, b.m); return a;}
FORCE_INLINE vec2  vec_min(vec2 a, vec2 b){a.m = _mm_min_ps(a.m, b.m); return a;}
FORCE_INLINE vec2  vec_max(vec2 a, vec2 b){a.m = _mm_max_ps(a.m, b.m); return a;}


//vec3 common operators:
FORCE_INLINE vec3  operator+ (vec3 a, vec3 b) {a.m = _mm_add_ps(a.m, b.m); return a;} 
FORCE_INLINE vec3  operator- (vec3 a, vec3 b) {a.m = _mm_sub_ps(a.m, b.m); return a;}
FORCE_INLINE vec3  operator* (vec3 a, vec3 b) {a.m = _mm_mul_ps(a.m, b.m); return a;}
FORCE_INLINE vec3  operator/ (vec3 a, vec3 b) {a.m = _mm_div_ps(a.m, b.m); return a;}
FORCE_INLINE vec3  operator* (vec3 a, float  b) {a.m = _mm_mul_ps(a.m, _mm_set1_ps(b)); return a;}
FORCE_INLINE vec3  operator/ (vec3 a, float  b) {a.m = _mm_div_ps(a.m, _mm_set1_ps(b)); return a;}
FORCE_INLINE vec3  operator* (float  a, vec3 b) {b.m = _mm_mul_ps(_mm_set1_ps(a), b.m); return b;}
FORCE_INLINE vec3  operator/ (float  a, vec3 b) {b.m = _mm_div_ps(_mm_set1_ps(a), b.m); return b;}
FORCE_INLINE vec3& operator+= (vec3 &a, vec3 b) {a = a + b; return a;}
FORCE_INLINE vec3& operator-= (vec3 &a, vec3 b) {a = a - b; return a;} 
FORCE_INLINE vec3& operator*= (vec3 &a, vec3 b) {a = a * b; return a;}
FORCE_INLINE vec3& operator/= (vec3 &a, vec3 b) {a = a / b; return a;}
FORCE_INLINE vec3& operator*= (vec3 &a, float  b) {a = a * b; return a;}
FORCE_INLINE vec3& operator/= (vec3 &a, float  b) {a = a / b; return a;}
FORCE_INLINE bool3 operator== (vec3 a, vec3 b) {a.m = _mm_cmpeq_ps(a.m, b.m); return a;}
FORCE_INLINE bool3 operator!= (vec3 a, vec3 b) {a.m = _mm_cmpneq_ps(a.m, b.m); return a;}
FORCE_INLINE bool3 operator< (vec3 a, vec3 b) {a.m = _mm_cmplt_ps(a.m, b.m); return a;}
FORCE_INLINE bool3 operator> (vec3 a, vec3 b) {a.m = _mm_cmpgt_ps(a.m, b.m); return a;}
FORCE_INLINE bool3 operator<= (vec3 a, vec3 b) {a.m = _mm_cmple_ps(a.m, b.m); return a;}
FORCE_INLINE bool3 operator>= (vec3 a, vec3 b) {a.m = _mm_cmpge_ps(a.m, b.m); return a;}
FORCE_INLINE vec3  vec_min(vec3 a, vec3 b) {a.m = _mm_min_ps(a.m, b.m); return a;}
FORCE_INLINE vec3  vec_max(vec3 a, vec3 b) {a.m = _mm_max_ps(a.m, b.m); return a;}

//vec4 common operators:
FORCE_INLINE vec4  operator+ (vec4 a, vec4 b) {a.m = _mm_add_ps(a.m, b.m); return a;} 
FORCE_INLINE vec4  operator- (vec4 a, vec4 b) {a.m = _mm_sub_ps(a.m, b.m); return a;}
FORCE_INLINE vec4  operator* (vec4 a, vec4 b) {a.m = _mm_mul_ps(a.m, b.m); return a;}
FORCE_INLINE vec4  operator/ (vec4 a, vec4 b) {a.m = _mm_div_ps(a.m, b.m); return a;}
FORCE_INLINE vec4  operator* (vec4 a, float  b) {a.m = _mm_mul_ps(a.m, _mm_set1_ps(b)); return a;} 
FORCE_INLINE vec4  operator/ (vec4 a, float  b) {a.m = _mm_div_ps(a.m, _mm_set1_ps(b)); return a;}
FORCE_INLINE vec4  operator* (float  a, vec4 b) {b.m = _mm_mul_ps(_mm_set1_ps(a), b.m); return b;}
FORCE_INLINE vec4  operator/ (float  a, vec4 b) {b.m = _mm_div_ps(_mm_set1_ps(a), b.m); return b;}
FORCE_INLINE vec4& operator+= (vec4 &a, vec4 b) {a = a + b; return a;}
FORCE_INLINE vec4& operator-= (vec4 &a, vec4 b) {a = a - b; return a;} 
FORCE_INLINE vec4& operator*= (vec4 &a, vec4 b) {a = a * b; return a;}
FORCE_INLINE vec4& operator/= (vec4 &a, vec4 b) {a = a / b; return a;}
FORCE_INLINE vec4& operator*= (vec4 &a, float  b) {a = a * b; return a;}
FORCE_INLINE vec4& operator/= (vec4 &a, float  b) {a = a / b; return a;}
FORCE_INLINE bool4 operator== (vec4 a, vec4 b) {a.m = _mm_cmpeq_ps(a.m, b.m); return a;}
FORCE_INLINE bool4 operator!= (vec4 a, vec4 b) {a.m = _mm_cmpneq_ps(a.m, b.m); return a;}
FORCE_INLINE bool4 operator< (vec4 a, vec4 b) {a.m = _mm_cmplt_ps(a.m, b.m); return a;}
FORCE_INLINE bool4 operator> (vec4 a, vec4 b) {a.m = _mm_cmpgt_ps(a.m, b.m); return a;}
FORCE_INLINE bool4 operator<= (vec4 a, vec4 b) {a.m = _mm_cmple_ps(a.m, b.m); return a;}
FORCE_INLINE bool4 operator>= (vec4 a, vec4 b) {a.m = _mm_cmpge_ps(a.m, b.m); return a;}
FORCE_INLINE vec4  vec_min(vec4 a, vec4 b) {a.m = _mm_min_ps(a.m, b.m); return a;}
FORCE_INLINE vec4  vec_max(vec4 a, vec4 b) {a.m = _mm_max_ps(a.m, b.m); return a;}

// Unary operators.
FORCE_INLINE vec2 operator- (vec2 a) { return vec2(_mm_setzero_ps()) - a; }
FORCE_INLINE vec2 abs(vec2 v) { v.m = _mm_andnot_ps(vsignbits, v.m); return v; }

FORCE_INLINE vec3 operator- (vec3 a) { return vec3(_mm_setzero_ps()) - a; }
FORCE_INLINE vec3 abs(vec3 v) { v.m = _mm_andnot_ps(vsignbits, v.m); return v; }

FORCE_INLINE vec4 operator- (vec4 a) { return vec4(_mm_setzero_ps()) - a; }
FORCE_INLINE vec4 abs(vec4 v) { v.m = _mm_andnot_ps(vsignbits, v.m); return v; }

// Horizontal min/max.

/*
---TODO---:
FORCE_INLINE float hmin(vec2 v){}
FORCE_INLINE float hmax(vec2 v){}
FORCE_INLINE float hmin(vec4 v){} 
FORCE_INLINE float hmax(vec4 v){}
*/
FORCE_INLINE float hmin(vec3 v) 
{
	v = vec_min(v, SHUFFLE3(v, 1, 0, 2));
	return vec_min(v, SHUFFLE3(v, 2, 0, 1)).x();
}
FORCE_INLINE float hmax(vec3 v) 
{
	v = vec_max(v, SHUFFLE3(v, 1, 0, 2));
	return vec_max(v, SHUFFLE3(v, 2, 0, 1)).x();
}

// 3D cross product.
FORCE_INLINE vec3 cross(vec3 a, vec3 b) {
	// x  <-  a.y*b.z - a.z*b.y
	// y  <-  a.z*b.x - a.x*b.z
	// z  <-  a.x*b.y - a.y*b.x
	// We can save a shuffle by grouping it in this wacky order:
	return (a.zxy()*b - a*b.zxy()).zxy();
}

// Returns a 3-bit code where bit0..bit2 is X..Z
FORCE_INLINE unsigned mask(vec2 v) { return _mm_movemask_ps(v.m) & 3;}
FORCE_INLINE unsigned mask(vec3 v) { return _mm_movemask_ps(v.m) & 7; }
FORCE_INLINE unsigned mask(vec4 v) { return _mm_movemask_ps(v.m) & 0xF;}

// Once we have a comparison, we can branch based on its results:
FORCE_INLINE bool any(bool2 v) { return mask(v) != 0; }
FORCE_INLINE bool all(bool2 v) { return mask(v) == 3; }

FORCE_INLINE bool any(bool3 v) { return mask(v) != 0; }
FORCE_INLINE bool all(bool3 v) { return mask(v) == 7; }

FORCE_INLINE bool any(bool4 v) { return mask(v) != 0; }
FORCE_INLINE bool all(bool4 v) { return mask(v) == 0xF; }

//Standard functions for vectors
FORCE_INLINE vec2 clamp(vec2 t, vec2 a, vec2 b) { return vec_min(vec_max(t, a), b); }
FORCE_INLINE float sum(vec2 v) { return v.x() + v.y(); }
FORCE_INLINE float dot(vec2 a, vec2 b) { return sum(a*b); }
FORCE_INLINE float length(vec2 v) { return sqrtf(dot(v, v)); }
FORCE_INLINE float lengthSq(vec2 v) { return dot(v, v); }
FORCE_INLINE vec2 normalize(vec2 v) { return v * (1.0f / length(v)); }
FORCE_INLINE vec2 lerp(vec2 a, vec2 b, float t) { return a + (b-a)*t; }

FORCE_INLINE vec3 clamp(vec3 t, vec3 a, vec3 b) { return vec_min(vec_max(t, a), b); }
FORCE_INLINE float sum(vec3 v) { return v.x() + v.y() + v.z(); }
FORCE_INLINE float dot(vec3 a, vec3 b) { return sum(a*b); }
FORCE_INLINE float length(vec3 v) { return sqrtf(dot(v, v)); }
FORCE_INLINE float lengthSq(vec3 v) { return dot(v, v); }
FORCE_INLINE vec3 normalize(vec3 v) { return v * (1.0f / length(v)); }
FORCE_INLINE vec3 lerp(vec3 a, vec3 b, float t) { return a + (b-a)*t; }

FORCE_INLINE vec4 clamp(vec4 t, vec4 a, vec4 b) { return vec_min(vec_max(t, a), b); }
FORCE_INLINE float sum(vec4 v) { return v.x() + v.y() + v.z() + v.w(); }
FORCE_INLINE float dot(vec4 a, vec4 b) { return sum(a*b); }
FORCE_INLINE float length(vec4 v) { return sqrtf(dot(v, v)); }
FORCE_INLINE float lengthSq(vec4 v) { return dot(v, v); }
FORCE_INLINE vec4 normalize(vec4 v) { return v * (1.0f / length(v)); }
FORCE_INLINE vec4 lerp(vec4 a, vec4 b, float t) { return a + (b-a)*t; }

