#include "StdAfx.h"
#include "JMatrix.h"

#include <math.h>

/**
 * Calculates sin and cos of an angle
 * @param angle angle to find sine and cosine of
 * @param pSine pointer to location to store sine
 * @param pCosine pointer to location to store cosine
*/
void JMath_SinCos(double angle, double *pSine, double *pCosine)
{
  if(pSine)
    *pSine = sinf(angle);
  if(pCosine)
    *pCosine = cosf(angle);
}

JVector JMath_ApplyMatrix(const JVector &vector, const JMatrix &matrix)
{
  
	JVector  result = JVector_Create(0,0,0);

    result.v[0] = vector.v[0] * matrix.f[0]  +
                vector.v[1] * matrix.f[4]  +
                vector.v[2] * matrix.f[8]  + matrix.f[12];
    
    result.v[1] = vector.v[0] * matrix.f[1]  +
                vector.v[1] * matrix.f[5]  +
                vector.v[2] * matrix.f[9]  + matrix.f[13];
    
    result.v[2] = vector.v[0] * matrix.f[2]   +
                vector.v[1] * matrix.f[6]   +
                vector.v[2] * matrix.f[10]  + matrix.f[14];

    return result;  
}

JVector JVector_Cross(const JVector &v1, const JVector &v2)
{
	JVector n;
	n.x = v1.y * v2.z - v1.z * v2.y;
	n.y = v1.z * v2.x - v1.x * v2.z;
	n.z = v1.x * v2.y - v1.y * v2.x;

	float mag = n.x + n.y + n.z;
	n.x /= mag;
	n.y /= mag;
	n.z /= mag;

	return n;
}

JVector JVector_Create(double x,double y,double z){
	JVector t;
	t.Set(x,y,z);
	return t;
}

JVector JVector::operator-() const
{
  JVector temp;

  temp.x = -x;
  temp.y = -y;
  temp.z = -z;
  temp.w = -w;

  return temp;
}

//==============================================================================
// Addition with assignment operator
//==============================================================================
JVector& JVector::operator+=(const JVector &v)
{
  x += v.x;
  y += v.y;
  z += v.z;
  w += v.w;
  return *this;
}

//==============================================================================
// Subtraction with assignment operator
//==============================================================================
JVector& JVector::operator-=(const JVector &v)
{
  x -= v.x;
  y -= v.y;
  z -= v.z;
  w -= v.w;
  return *this;
}

//==============================================================================
// Multiplication with assignment operator
//==============================================================================
JVector& JVector::operator*=(const JVector &v)
{
  x *= v.x;
  y *= v.y;
  z *= v.z;
  w *= v.w;
  return *this;
}

//==============================================================================
// Multiplication by scalar with assignment operator
//==============================================================================
JVector& JVector::operator*=(float f)
{
  x *= f;
  y *= f;
  z *= f;
  w *= f;
  return *this;
}

//==============================================================================
// Division with assignment operator
//==============================================================================
JVector& JVector::operator/=(const JVector &v)
{
  x /= v.x;
  y /= v.y;
  z /= v.z;
  w /= v.w;
  return *this;
}

//==============================================================================
// Division by scalar with assignment operator
//==============================================================================
JVector& JVector::operator/=(float f)
{
  float invF = 1.0f / f;
  x *= invF;
  y *= invF;
  z *= invF;
  w *= invF;
  return *this;
}

//==============================================================================
// Global addition operator
//==============================================================================
JVector operator+(const JVector &v1, const JVector &v2)
{ 
  JVector tmp; 
  tmp.x = v1.x + v2.x;
  tmp.y = v1.y + v2.y;
  tmp.z = v1.z + v2.z;
  tmp.w = v1.w + v2.w;
  return tmp;
}

//==============================================================================
// Global subtraction operator
//==============================================================================
JVector operator-(const JVector &v1, const JVector &v2)
{ 
  JVector tmp; 
  tmp.x = v1.x - v2.x;
  tmp.y = v1.y - v2.y;
  tmp.z = v1.z - v2.z;
  tmp.w = v1.w - v2.w;
  return tmp;
}

//==============================================================================
// Global multiplication operator
//==============================================================================
JVector operator*(const JVector &v1, const JVector &v2)
{
  JVector tmp; 
  tmp.x = v1.x * v2.x;
  tmp.y = v1.y * v2.y;
  tmp.z = v1.z * v2.z;
  tmp.w = v1.w * v2.w;
  return tmp;
}

//==============================================================================
// Global scalar multiplication operator
//==============================================================================
JVector operator*(const JVector &v1, float f2)
{
  JVector tmp; 
  tmp.x = v1.x * f2;
  tmp.y = v1.y * f2;
  tmp.z = v1.z * f2;
  tmp.w = v1.w * f2;
  return tmp;
}

//==============================================================================
// Global scalar multiplication operator
//==============================================================================
JVector operator*(float f1, const JVector &v1)
{
  JVector tmp; 
  tmp.x = v1.x * f1;
  tmp.y = v1.y * f1;
  tmp.z = v1.z * f1;
  tmp.w = v1.w * f1;
  return tmp;
}

//==============================================================================
// Global division operator
//==============================================================================
JVector operator/(const JVector &v1, const JVector &v2)
{
  JVector tmp; 
  tmp.x = v1.x / v2.x;
  tmp.y = v1.y / v2.y;
  tmp.z = v1.z / v2.z;
  tmp.w = v1.w / v2.w;
  return tmp;
}

//==============================================================================
// Global scalar division operator
//==============================================================================
JVector operator/(const JVector &v1, float f2)
{
  JVector tmp; 
  tmp.x = v1.x / f2;
  tmp.y = v1.y / f2;
  tmp.z = v1.z / f2;
  tmp.w = v1.w / f2;
  return tmp;
}

const JVector JVector::zero = 
{
  {   
    0.f, 0.f, 0.f, 0.f
  }
};


const JMatrix JMatrix::identity = 
{
  {
    1.f, 0.f, 0.f, 0.f,
    0.f, 1.f, 0.f, 0.f,
    0.f, 0.f, 1.f, 0.f, 
    0.f, 0.f, 0.f, 1.f
  }
};

void JMatrix::Translate(const JVector &t)
{
 
  f[3*4+0] += f[0*4+0] * t.x + f[1*4+0] * t.y + f[2*4+0] * t.z;
  f[3*4+1] += f[0*4+1] * t.x + f[1*4+1] * t.y + f[2*4+1] * t.z;
  f[3*4+2] += f[0*4+2] * t.x + f[1*4+2] * t.y + f[2*4+2] * t.z;
  f[3*4+3] += f[0*4+3] * t.x + f[1*4+3] * t.y + f[2*4+3] * t.z; 
}

void JMatrix::Scale(const JVector &scale)
{
	m[0][0] *= scale.x;
	m[1][1] *= scale.y;
	m[2][2] *= scale.z;
}


void JMatrix::Rotate(const JVector &vAxis, float angle)
{

  // build the rotation matrix
	
  JMatrix rot = JMatrix::identity;
  double c,s,t;
  JMath_SinCos(angle, &s, &c);
  t = 1.0 - c;   

  rot.m[0][0] = t*vAxis.x*vAxis.x + c;
  rot.m[1][0] = t*vAxis.x*vAxis.y - s*vAxis.z;
  rot.m[2][0] = t*vAxis.x*vAxis.z + s*vAxis.y;

  rot.m[0][1] = t*vAxis.x*vAxis.y + s*vAxis.z;
  rot.m[1][1] = t*vAxis.y*vAxis.y + c;
  rot.m[2][1] = t*vAxis.y*vAxis.z - s*vAxis.x;

  rot.m[0][2] = t*vAxis.x*vAxis.z - s*vAxis.y;
  rot.m[1][2] = t*vAxis.y*vAxis.z + s*vAxis.x;
  rot.m[2][2] = t*vAxis.z*vAxis.z + c;
  rot.m[3][2] = 0.f;
  

	/*
	float s = sinf(angle);
	float c = cosf(angle);
	double t = 1.0 - c;
	double x = vAxis.x;
	double y = vAxis.y;
	double z = vAxis.z;

	double tx = t * x;
	double ty = t * y;
	double tz = t * z;
	
	double sz = s * z;
	double sy = s * y;
	double sx = s * x;
	
	JMatrix m = JMatrix::identity;
	m.f[0]  = tx * x + c;
	m.f[1]  = tx * y + sz;
	m.f[2]  = tx * z - sy;
	m.f[3]  = 0;

	m.f[4]  = tx * y - sz;
	m.f[5]  = ty * y + c;
	m.f[6]  = ty * z + sx;
	m.f[7]  = 0;

	m.f[8]  = tx * z + sy;
	m.f[9]  = ty * z - sx;
	m.f[10] = tz * z + c;
	m.f[11] = 0;

	m.f[12] = 0;
	m.f[13] = 0; 
	m.f[14] = 0;
	m.f[15] = 1; 
	*/

	Multiply(*this, rot);
}

void JMatrix::Multiply(const JMatrix &a, const JMatrix &b)
{
  JMatrix product;

  for (int i = 0; i < 16; i += 4) 
  {
	for (int j = 0; j < 4; j++) 
	{
		product.f[i + j] = 0.0;
		for (int k = 0; k < 4; k++)
		{
			product.f[i + j] += a.f[i + k] * b.f[k*4 + j];
		}
	}
  }

  *this = product;
}