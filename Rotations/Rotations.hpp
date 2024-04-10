#include <math.h>
#include "Matrix3D.h"
#include "Vector3D.h"
#include "Quaternion.h"
using OpenSees::Matrix3D;
typedef ASDQuaternion<double> Versor;

Vector3D Versor2Vector(Versor);
Versor   Matrix2Versor(Matrix3D);
// matrix_to_versor

static constexpr Matrix3D Eye3 {{{
  {1, 0, 0},
  {0, 1, 0},
  {0, 0, 1}
}}};

Vector3D Axial(const Matrix3D &X)
{
// Return the axial vector x of the given skew-symmetric 3x3 matrix X.
// =========================================================================================
// function by Claudio Perez                                                            2023
// -----------------------------------------------------------------------------------------

  return {X(3,2), X(1,3), X(2,1)};
}

Matrix3D Spin(const Vector3D &u)
{
//SPIN determine the spin tensor of a vector
// S = SPIN (U)
// the function determines the spin tensor S of a vector U with three components
 
// =========================================================================================
// FEDEASLab - Release 5.2, July 2021
// Matlab Finite Elements for Design, Evaluation and Analysis of Structures
// Professor Filip C. Filippou (filippou@berkeley.edu)
// Department of Civil and Environmental Engineering, UC Berkeley
// Copyright(c) 1998-2021. The Regents of the University of California. All Rights Reserved.
// =========================================================================================
// function by Veronique LeCorvec                                                    08-2008
// refactored by Claudio M. Perez                                                       2023
// -----------------------------------------------------------------------------------------

  return Matrix3D{{{{  0  ,  u(3), -u(2)},
                    {-u(3),   0  ,  u(1)},
                    { u(2), -u(1),   0  }}}};
}

void GibSO3(const Vector3D &vec, double *a, double *b=nullptr, double *c=nullptr)
{
//
//Compute coefficients of the Rodrigues formula and their derivatives.
//
//                       [1,2]     | [3]
//                       ----------+-------
//                       a1        |
//                       a2        | c4
//                       a3        | c5
//
//                       b1 = -c0  | c1
//                       b2        | c2 
//                       b3        | c3
//
//
// [1] Perez, C. M., and Filippou F. C. (2024) "On Nonlinear Geometric 
//     Transformations of Finite Elements" 
//     Int. J. Numer. Meth. Engrg. 2024 (Expected)
//
// [2] Ritto-Corrêa, M. and Camotim, D. (2002) "On the differentiation of the
//     Rodrigues formula and its significance for the vector-like parameterization
//     of Reissner-Simo beam theory"
//     Int. J. Numer. Meth. Engrg., 55(9), pp.
//     1005–1032. Available at: https://doi.org/10.1002/nme.532.
//
// [3] Ibrahimbegović, A. and Mikdad, M.A. (1998) ‘Finite rotations in dynamics of
//     beams and implicit time‐stepping’, 41, pp. 781–814.
//
// [4] Pfister, F. (1998) ‘Bernoulli Numbers and Rotational Kinematics’,
//     Journal of Applied Mechanics, 65(3), pp. 758–763. 
//     Available at: https://doi.org/10.1115/1.2789120.
//
// =========================================================================================
// function by Claudio Perez                                                            2023
// -----------------------------------------------------------------------------------------
//
  double angle2 =  vec.dot(vec);

  if (angle2  <= 1e-07) {
    if (a != nullptr) {
      a[0]   =   0.0;
      a[1]   =   1.0        - angle2*(1.0/6.0   - angle2*(1.0/120.0  - angle2/5040.0));
      a[2]   =   0.5        - angle2*(1.0/24.0  - angle2*(1.0/720.0  - angle2/40320.0));
      a[3]   =   1.0/6.0    - angle2/(1.0/120.0 - angle2/(1.0/5040.0 - angle2/362880.0));
    }
    if (b != nullptr) {
      b[1]   = - 1.0/3.0    + angle2*(1.0/30.0  - angle2*(1.0/840.0  - angle2/45360.0));
      b[2]   = - 1.0/12.0   + angle2*(1.0/180.0 - angle2*(1.0/6720.0 - angle2/453600.0));
      b[3]   = - 1.0/60.0   + angle2*(1.0/1260.0 - angle2*(1.0/60480 - angle2/4989600.0));
    }
    if (c != nullptr) {
      c[1]   = b[3] - b[2]; 
      c[2]   = 1.0/90.0     - angle2*(1.0/1680.0 - angle2*(1.0/75600.0 - angle2/5987520.0)); 
      c[3]   = 1.0/630.0    - angle2*(1.0/15120.0 - angle2*(1.0/831600.0 - angle2/77837760.0));
    }

  } else {

    double angle  = vec.norm();
//  double angle  = sqrt(angle2);
    double sn     = sin(angle);
    double cs     = cos(angle);
    double angle3 = angle*angle2;
    double angle4 = angle*angle3;
    double angle5 = angle*angle4;
    double angle6 = angle*angle5;
                                                    
    if (a != nullptr) {
      a[0] = cs;
      a[1] = sn / angle;   
      a[2] = ( 1.0 - a[0] ) / angle2;    
  //  a[3] = ( 1.0 - a[1] ) / angle2;
      a[3] = (angle - sn)/(angle3);
    }

    if (b != nullptr) {
      b[1] = ( angle*cs - sn)/angle3;  
      b[2] = ( angle*sn - 2 + 2*cs)/angle4;    
      b[3] = ( 3*sn - 2*angle -  angle*cs )/angle5;    
    }
    if (c != nullptr) {
      c[1] = (3*sn - angle2*sn - 3*angle*cs)/(angle5);
      c[2] = (8 - 8*cs - 5*angle*sn + angle2*cs)/(angle5*angle);
      c[3] = (8*angle + 7*angle*cs + angle2*sn - 15*sn)/(angle5*angle2);
    }
  }
}



Matrix3D ddTanSO3(const Vector3D &v, const Vector3D &p, const Vector3D &q)
{
// [1] Perez, C.M., and Filippou F. C.. "On Nonlinear Geometric 
//     Transformations of Finite Elements" 
//     Int. J. Numer. Meth. Engrg. 2024 (Expected)
//
// =========================================================================================
// function by Claudio Perez                                                            2023
// -----------------------------------------------------------------------------------------
  double a[4], b[4], c[4];
  GibSO3(v, a, b, c);

  const Vector3D pxq = p.cross(q);
  const Vector3D vxp = v.cross(p);

  const Matrix3D psq { p.bun(q) + q.bun(p) };
  const Matrix3D vov { v.bun(v) };

  Matrix3D dT{0.0};

  dT.addTensorProduct(p, q, a[3])
    .addTensorProduct(q, p, a[3])
    .addDiagonal(b[1]*p.dot(q))
    .addTensorProduct(pxq, v, b[2])
    .addTensorProduct(v, pxq, b[2])
    .addDiagonal(vxp.dot(q)*b[2])

    .addTensorProduct(q, v, b[3]*v.dot(p))
    .addTensorProduct(v, q, b[3]*v.dot(p))
    .addTensorProduct(p, v, b[3]*v.dot(q))
    .addTensorProduct(v, p, b[3]*v.dot(q))
    .addDiagonal(v.dot(p)*v.dot(q)*b[3])

    .addTensorProduct(v, v, c[1]*p.dot(q) + c[2]*(vxp.dot(q)) + c[3]*v.dot(p)*v.dot(q));

  return dT;

//return a[3]*psq + b[1]*p.dot(q)*Eye3
//     + b[2]*(pxq.bun(v) + v.bun(pxq) + vxp.dot(q)*Eye3)
//     + b[3]*( v.dot(p)*(q.bun(v) + v.bun(q)) 
//          + v.dot(q)*(p.bun(v) + v.bun(p)) 
//          + v.dot(p)*v.dot(q)*Eye3) 
//     + vov*(c[1]*p.dot(q) + c[2]*(vxp.dot(q)) + c[3]*v.dot(p)*v.dot(q));
}

Matrix3D dExpSO3(const Vector3D &v)
{
// [1] Perez, C.M., and Filippou F. C.. "On Nonlinear Geometric 
//     Transformations of Finite Elements" 
//     Int. J. Numer. Meth. Engrg. 2024 (Expected)
//
// =========================================================================================
// function by Claudio Perez                                                            2023
// -----------------------------------------------------------------------------------------

  // Form first Gib coefficients
  double a[4];
  GibSO3(v, a);

  Matrix3D T{0.0};
  T.addDiagonal(a[1])
   .addSpin(v, a[2])
   .addTensorProduct(v, v, a[3]);

  return T;

//return a[1]*Eye3 + a[2]*Th + a[3]*v.bun(v);
}

Matrix3D dExpInvSO3(const Vector3D &v)
{
// =========================================================================================
// function by Claudio Perez                                                            2023
// -----------------------------------------------------------------------------------------
//
  constexpr double tol = 1/20;

  Matrix3D Sv = Spin(v);

  double angle = v.norm();
  if (abs(angle) > M_PI/1.01) {
    v = v - 2*v/angle*floor(angle + M_PI)/2;
    angle = v.norm();
  }

  double angle2 = angle*angle;
  double angle3 = angle*angle2;
  double angle4 = angle*angle3;
  double angle5 = angle*angle4;
  double angle6 = angle*angle5;

  double eta;
  if (angle > tol) {
    eta = (1-0.5*angle*cot(0.5*angle))/angle2;
  } else {
    eta = 1/12 + angle2/720 + angle4/30240 + angle6/1209600;
  }
  return Eye3 - 0.5*Sv + eta*Sv*Sv;
}

Matrix3D ddExpInvSO3(const Vector3D& th, const Vector3D& v)
{
// =========================================================================================
// function by Claudio Perez                                                            2023
// -----------------------------------------------------------------------------------------

  constexpr double tol = 1/20;
  double angle = th.norm();

  if (fabs(angle) > M_PI/1.01) {
    v = v - 2*v/angle*floor(angle + M_PI)/2;
    angle = v.norm();
  }

  
  double angle2 = angle*angle;
  double angle3 = angle*angle2;
  double angle4 = angle*angle3;
  double angle5 = angle*angle4;
  double angle6 = angle*angle5;

  double eta, mu;
  if (angle < tol) {
    eta = 1/12 + angle2/720 + angle4/30240 + angle6/1209600;
    mu  = 1/360 + angle2/7560 + angle4/201600 + angle6/5987520;

  } else {
    double an2 = angle/2;
    double sn  = sin(an2);
    double cs  = cos(an2);

    eta = (sn - angle2*cs)/(angle2*sn);
    mu  = (angle*(angle + 2*sn*cs) - 8*sn*sn)/(4*angle4*sn*sn);
  }

  Matrix3D St2 = Spin(th);
  St2 = St2*St2;
  Matrix3D dH  = -0.5*Spin(v) + eta*(Eye3*th.dot(v) + th.bun(v) - 2*v.bun(th)) + mu*St2*v.bun(th);

  return dH*dLogSO3(th);
}

Matrix3D dTanSO3(const Vector3D &v, const Vector3D &p, char repr)
{
//
// repr     'L' or 'R' indicating left or right representation, 
//          respectively, for the tangent space of SO(3)
// =========================================================================================
// function by Claudio Perez                                                            2023
// -----------------------------------------------------------------------------------------

  if (nargin < 3) {
    repr = 'L';
  }

  double a[4], b[4];
  GibSO3(v, a, b);

  Matrix3D vxpov = v.cross(p).bun(v);

  Matrix3D Xi;
  switch (repr) {
    case 'R':
      Xi = - a2*Spin(p) + a3*v.dot(p)*Eye3 + a3*v.bun(p)
           + b1*a.bun(v) + b2*vxpov + b3*v.dot(p)*v.bun(v);
    case 'L':
      Xi =   a2*Spin(p) + a3*v.dot(p)*Eye3 + a3*v.bun(p)
           + b1*a.bun(v) - b2*vxpov + b3*v.dot(p)*v.bun(v);
  }
  return Xi;
}

Matrix3D ExpSO3(const Vector3D &v)
{
// =========================================================================================
// function by Claudio Perez                                                            2023
// -----------------------------------------------------------------------------------------

  //Form the first Gib coefficients
  double a[4];
  GibSO3(th, a);

  //Form 3x3 skew-symmetric matrix Th from axial vector th
  Matrix3D Th = Spin(v);

  return Eye3 + a[1]*Th + a[2]*Th*Th;

}



Vector3D LogSO3(const Matrix3D &R)
{
//LOGSO3 Inverse of the exponential map on SO(3).
// vect = LogSO3(R)
// vect = LogSO3(R,option)
// Returns the axial parameters associated with the rotation `R`. The result
// should satisfy the following equality for any 3-vector, `v`:
//
//       LogSO3(expm(spin(v))) == v
//
// where `expm` is the Matlab built-in matrix exponential, and `spin` is a function
// which produces the skew-symmetric 3x3 matrix associated with vector `v`.
//
// Parameters
//   R       (3x3)   Rotation matrix.
//   option  string  Algorithm option; default is 'Quat'. See reference below.
//                   All other options are for internal use.
//
// Remarks
//
// - Does not check if input is really a rotation. If you arent sure, use
//   Matlab's `logm`. This is slower, but more general. Note however that`logm`
//   may not be as accurate in corner cases and sometimes returns complex-valued
//   results.
// - The angle corresponding to the returned vector is always in the interval [0,pi].
//
//
// References
// 1. Nurlanov Z (2021) Exploring SO(3) logarithmic map: degeneracies and 
//    derivatives.
//
// =========================================================================================
// function by Claudio Perez                                                            2023
// -----------------------------------------------------------------------------------------

  return Versor2Vector(Matrix2Versor(R));

}


Matrix3D TanSO3(const Vector3D &vec)
{
//  Compute right differential of the exponential.
//
// =========================================================================================
// function by Claudio Perez                                                            2023
// -----------------------------------------------------------------------------------------

    double angle2 = vec.dot(vec);
    double a1, a2, a3;

//  Check for angle near 0
    if (angle2 < 1e-08) {
      a1  = 1.0     - angle2*(1.0/6.0   - angle2*(1.0/120.0  - angle2/5040.0));
      a2  = 0.5     - angle2*(1.0/24.0  - angle2*(1.0/720.0  - angle2/40320.0));
      a3  = 1.0/6.0 - angle2/(1.0/120.0 - angle2/(1.0/5040.0 - angle2/362880.0));

    } else {
      double angle = sqrt (angle2);
      a1  = sin(angle)        /angle; 
      a2  = (1.0 - cos(angle))/angle2;
      a3  = (1.0 - a1        )/angle2;
    }

//  Assemble differential
    Matrix3D T;
    T(1,1)  =         a1 + a3*vec[1]*vec[1];
    T(1,2)  = -vec[3]*a2 + a3*vec[1]*vec[2];
    T(1,3)  =  vec[2]*a2 + a3*vec[1]*vec[3];
    T(2,1)  =  vec[3]*a2 + a3*vec[2]*vec[1];
    T(2,2)  =         a1 + a3*vec[2]*vec[2];
    T(2,3)  = -vec[1]*a2 + a3*vec[2]*vec[3];
    T(3,1)  = -vec[2]*a2 + a3*vec[3]*vec[1];
    T(3,2)  =  vec[1]*a2 + a3*vec[3]*vec[2];
    T(3,3)  =         a1 + a3*vec[3]*vec[3];
    return T;
}


