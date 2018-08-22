
#pragma once

/**
 * Vector structures
 *
  */
#pragma pack(1)
struct Vector3
{
	float x,y,z;
};

struct JVector
{
  union
  {
    struct { double x, y, z, w; };
    double v[4];

  };

  static const JVector zero;

  void SetZero() {
	    x=y=z=w=0;
  }

  void Set(double x, double y, double z) {
	  this->x = x; this->y = y; this->z = z; this->w = 0;
  }

  void Normalize() {
	float mag = x + y + z;
	x /= mag;
	y /= mag;
	z /= mag;
	w = 0;
  }


  // Assignment operators
  JVector& operator+=(const JVector &v);
  JVector& operator-=(const JVector &v);
  JVector& operator*=(const JVector &v);
  JVector& operator*=(float f);
  JVector& operator/=(const JVector &v);
  JVector& operator/=(float f);

   // Arithmetic operators
  JVector operator-() const;

};

JVector operator+(const JVector &v1, const JVector &v2);
JVector operator-(const JVector &v1, const JVector &v2);
JVector operator*(const JVector &a, const JVector &b);
JVector operator*(const JVector &v1, float f2);
JVector operator*(float f1, const JVector &v2);
JVector operator/(const JVector &a, const JVector &b);
JVector operator/(const JVector &v1, float f2);

JVector JVector_Cross(const JVector &v1, const JVector &v2);
JVector JVector_Create(double x,double y,double z);
/*
struct Vector4
{
	float x,y,z,w;
};
*/

#pragma pop



