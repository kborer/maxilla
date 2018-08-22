#pragma once



#include "Vector.h"

#pragma pack(1)

// some useful constants
const float PI            = 3.14159265f;      // PI 
const float PI2           = 6.283185307f;     // PI * 2
const float INV_PI        = 0.31830988618f;   // 1 / PI
const float PIdiv180      = 0.0174532925f;    // PI / 180
const float FLOAT_MAX     = 3.402823466e+38F; // Maximum value of a 32-bit float
const float FLOAT_MIN     = 1.175494351e-38F; // Minimum value of a 32 bit float

// angle representation conversions
inline float JMath_Deg2Rad(float deg) { return deg*(PIdiv180); }
inline float JMath_Rad2Deg(float rad) { return rad*(1.f / PIdiv180); }


struct JMatrix
{
  union
  {
    double f[16];
    double m[4][4];
    JVector r[4];
  };
  static const JMatrix identity;

  /** Translates the matrix by translation */
  void Translate(const JVector &translation);

  void Rotate(const JVector &axis, float angle);

  void Scale(const JVector &scale);

  void Multiply(const JMatrix &a, const JMatrix &b);
  void Multiply(const JMatrix &a) { Multiply(*this, a); };

};


#pragma pop

extern JVector JMath_ApplyMatrix(const JVector &vector, const JMatrix &matrix);


