#pragma once

#include "vectors.h"

struct mat4
{
	vec4 row[4];

	//constructors
	explicit mat4(const float = 1.0f)
	{
		row[0] = vec4(1.0f, 0.0f, 0.0f, 0.0f);
		row[1] = vec4(0.0f, 1.0f, 0.0f, 0.0f);
		row[2] = vec4(0.0f, 0.0f, 1.0f, 0.0f);
		row[3] = vec4(0.0f, 0.0f, 0.0f, 1.0f);
	}

	explicit mat4(vec4 r0, vec4 r1, vec4 r2, vec4 r3)
	{
		row[0] = r0;
		row[1] = r1; 
		row[2] = r2; 
		row[3] = r3; 
	}

	explicit mat4(float *p)
	{
		int index = 0;
		for(int i = 0; i < 4; i++)
		{
			row[i] = vec4(p[index + 0], p[index + 1], p[index + 2], p[index + 3]);
			index += 4;
		}
	}

	//multidimensional array access operators: mat4(row, column)
	FORCE_INLINE float  operator() (size_t r, size_t c) const { return row[r][c]; };

	FORCE_INLINE float&  operator() (size_t r, size_t c) { return row[r][c]; };
};

//mat4 operations
//because we're using _vectorcall, we don't need to use references/pointers

FORCE_INLINE void transpose(mat4 &a)
{
	_MM_TRANSPOSE4_PS(a.row[0].m, a.row[1].m, a.row[2].m, a.row[3].m);
}

mat4 operator+ (mat4 a, mat4 b) 
{
	a.row[0] = a.row[0] + b.row[0];
	a.row[1] = a.row[1] + b.row[1];
	a.row[2] = a.row[2] + b.row[2];
	a.row[3] = a.row[3] + b.row[3];
	
	return a;
}

mat4 operator- (mat4 a, mat4 b) 
{
	a.row[0] = a.row[0] - b.row[0];
	a.row[1] = a.row[1] - b.row[1];
	a.row[2] = a.row[2] - b.row[2];
	a.row[3] = a.row[3] - b.row[3];
	
	return a;
}

//linear combination: row-vector * matrix
vec4 operator* (vec4 a, mat4 b)
{
	vec4 result;	
	result = a.x() * b.row[0];
	result += a.y() * b.row[1];
	result += a.z() * b.row[2];
	result += a.w() * b.row[3];
	
	return result;
}

vec4 operator* (mat4 b, vec4 a)
{
	vec4 result = vec4();	
	result = a.x() * b.row[0];
	result += a.y() * b.row[1];
	result += a.z() * b.row[2];
	result += a.w() * b.row[3];

	return result;
}

mat4 operator* (mat4 a, mat4 b) 
{
	a.row[0] = a.row[0] * b;
	a.row[1] = a.row[1] * b;
	a.row[2] = a.row[2] * b;
	a.row[3] = a.row[3] * b;
	
	return a;
}
	
mat4 lookAt(vec3 from, vec3 to, vec3 up_)
{
    vec3 fwd = normalize(from - to);
    vec3 right = cross(normalize(up_), fwd);
    vec3 up = cross(fwd, right);
    
	return mat4(vec4(    right.x(),         up.x(),        fwd.x(),      0.0f),
	            vec4(    right.y(),         up.y(),        fwd.y(),      0.0f),
				vec4(    right.z(),         up.z(),        fwd.z(),      0.0f),
				vec4(-dot(from, right), -dot(from, up), -dot(from, fwd), 1.0f));

}

mat4 vulkanPerspectiveSymmetric(float width, float height, 
                                float n, float f)
{
    //NOTE: Uses reverse depth !!!
    //pipeline depth info struct compare op should be VK_COMPARE_OP_GREATER_OR_EQUAL;
    
    float m00 = (2.0f * n)/width;
    float m11 = -(2.0f * n)/height;
    float m22 = n/(f-n);
    float m32 = (n*f)/(f-n);

    return mat4(vec4(m00,  0.0f, 0.0f, 0.0f),
                vec4(0.0f, m11,  0.0f, 0.0f),
                vec4(0.0f, 0.0f, m22, -1.0f),
                vec4(0.0f, 0.0f, m32,  0.0f));
}

mat4 vulkanPerspective(float l, float r, 
                       float b, float t, 
                       float n, float f)
{
    //NOTE: Uses reverse depth !!!
    //pipeline depth info struct compare op should be VK_COMPARE_OP_GREATER_OR_EQUAL;
    
    float m00 = (2.0f * n)/(r-l);
    float m20 = (r+l)/(r-l);
    float m11 = (2.0f * n)/(b-t);
    float m21 = (b+t)/(b-t);
    float m22 = n/(f-n);
    float m32 = (n*f)/(f-n);

    return mat4(vec4(m00,  0.0f, 0.0f, 0.0f),
                vec4(0.0f, m11,  0.0f, 0.0f),
                vec4(m20,  m21,  m22, -1.0f),
                vec4(0.0f, 0.0f, m32,  0.0f));
}

mat4 vulkanPerspective(float aspect,
                       float yFov,
                       float n,
                       float f)
{
    //NOTE: Uses reverse depth !!!
    //pipeline depth info struct compare op should be VK_COMPARE_OP_GREATER_OR_EQUAL;
    
    float focalLength = 1.0f / tan( DEG2RAD( yFov * 0.5f ) );
    float x = focalLength/aspect;
    float y = -focalLength;
    float A = n/(f-n);
    float B = f * A;
    
    return mat4(vec4( x,   0.0f, 0.0f,   0.0f),
                vec4(0.0f,  y,   0.0f,   0.0f),
                vec4(0.0f, 0.0f,  A,    -1.0f),
                vec4(0.0f, 0.0f,  B,     0.0f));
}