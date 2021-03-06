#ifndef BASIC_MATH_H
#define BASIC_MATH_H

#include <stdlib.h>
#include <math.h>
#include <assert.h>
#include <iostream>
#include <vector>
using namespace std;

#define ANGLE_IN_DEGREES  0
#define ANGLE_IN_RADIANS  1

template<class DataType, int n>
class Array {
public:
     DataType &operator[](int i) {
          return data[i];
     }
     const DataType &operator[](int i) const {
          return data[i];
     }
private:
     DataType  data[n];
};
typedef Array<double,2> Point2D;
typedef Array<double,3> Point3D;
typedef Array<double,3> Array3D;
typedef Array<double,4> Array4D;
typedef Array<double,3> Vec3D;

typedef Array<float,2> Point2F;
typedef Array<float,3> Point3F;
typedef Array<float,3> Array3F;
typedef Array<float,4> Array4F;
typedef Array<float,3> Vec3F;

namespace Math {
template<class T>
T random_value(T minval, T maxval)
{
}

template<>
inline int random_value(int minval, int maxval)
{
     return(minval + rand() % (int) (maxval - minval + 1));
}

template<>
inline size_t random_value(size_t minval, size_t maxval)
{
     return(size_t) (minval + rand() % (int) (maxval - minval + 1));
}

template<>
inline double random_value(double minval, double maxval)
{
     return minval + 0.5 * (1.0 + drand48())*(maxval - minval);
}

template<>
inline float random_value(float minval, float maxval)
{
     return minval + 0.5 * (1.0 + drand48())*(maxval - minval);
}

inline void create_vector( const Point3D &head, const Point3D &tail, Vec3D &xyz)
{
     xyz[0] = head[0] - tail[0];
     xyz[1] = head[1] - tail[1];
     xyz[2] = head[2] - tail[2];
}

inline double length( const Point3D &A, const Point3D &B)
{
     double dx = A[0] - B[0];
     double dy = A[1] - B[1];
     double dz = A[2] - B[2];
     return sqrt( dx*dx + dy*dy + dz*dz );
}

inline double length2( const Point3D &A, const Point3D &B)
{
     double dx = A[0] - B[0];
     double dy = A[1] - B[1];
     double dz = A[2] - B[2];
     return dx*dx + dy*dy + dz*dz;
}

inline double magnitude( const Vec3D &A )
{
     return sqrt( A[0]*A[0] + A[1]*A[1] + A[2]*A[2] );
}

inline double dot_product( const Vec3D &A, const Vec3D &B)
{
     return A[0]*B[0] + A[1]*B[1] + A[2]*B[2];
}

inline void cross_product( const Vec3D &A, const Vec3D &B, Vec3D &C)
{
     C[0] = A[1]*B[2] - A[2]*B[1];
     C[1] = A[2]*B[0] - A[0]*B[2];
     C[2] = A[0]*B[1] - A[1]*B[0];
}

inline double poly_area(const vector<Point2D> &p )
{
     // Formula from wikipedia....
     double sum = 0.0;
     int nSize = p.size();

     assert( nSize >= 3);

     for( int i  = 0; i < nSize; i++)
          sum += p[i][0]*p[(i+1)%nSize][1] -p[(i+1)%nSize][0]*p[i][1];
     return 0.5*sum;
}

inline void poly_centroid( const vector<Point2D> &p, Point2D &c )
{
     int nSize = p.size();
     assert( nSize >= 3);

     // Formula from wikipedia. Note that centroid does not depend on the
     // orientation of the polygon. The division by Area will take care
     // of correct value.

     // For convex bodies, centroid is always inside the region.

     double cx = 0.0;
     double cy = 0.0;
     double cf;

     for( int i  = 0; i < nSize; i++) {
          cf  = p[i][0]*p[(i+1)%nSize][1] - p[(i+1)%nSize][0]*p[i][1];
          cx +=  (p[i][0]+p[(i+1)%nSize][0])*cf;
          cy +=  (p[i][1]+p[(i+1)%nSize][1])*cf;
     }

     double A = poly_area( p );

     c[0] = cx/(6.0*A);
     c[1] = cy/(6.0*A);
}

inline void normal( const Point3D &p0, const Point3D &p1, const Point3D &p2,
                    Vec3D &normal)
{
     Vec3D p1p0, p2p0;
     Math::create_vector( p2, p0, p2p0);
     Math::create_vector( p1, p0, p1p0);
     Math::cross_product( p2p0, p1p0, normal);

     double mag = Math::magnitude( normal );
     normal[0] /= mag;
     normal[1] /= mag;
     normal[2] /= mag;
}

inline Vec3D unit_vector( const Point3D &head, const Point3D &tail)
{
     Vec3D uvec;
     create_vector( head, tail, uvec);
     double dl  = magnitude(uvec);

     uvec[0]  /=  dl;
     uvec[1]  /=  dl;
     uvec[2]  /=  dl;

     return uvec;
}

///////////////////////////////////////////////////////////////////////////////

inline double getVectorAngle( const Vec3D &A, const Vec3D &B, int measure)
{
     double AB = dot_product(A,B);
     double Am = magnitude(A);
     double Bm = magnitude(B);

     if( Am < 1.0E-15 || Bm < 1.0E-15) return 0.0;

     double x = AB/(Am*Bm);

     if( x > 1.0) x = 1.0;
     if( x < -1.0) x = -1.0;

     if( measure == ANGLE_IN_DEGREES ) return 180*acos(x)/M_PI;
     return acos(x);
}

//////////////////////////////////////////////////////////////////////////////

template<class T>
inline T max_value( const T &a, const T &b, const T &c)
{
     return max(a,max(b, c));
}

template<class T>
inline T min_value( const T &a, const T &b, const T &c)
{
     return min(a,min(b, c));
}

template <class T, size_t n>
inline double getAngle(const Array<T, n> &VecA, const Array<T, n> &VecB,
                       int unit_measure)
{
     double Abar, Bbar, theta;
     Abar = magnitude(VecA);
     Bbar = magnitude(VecB);

     if (Abar < 1.0E-10 || Bbar < 1.0E-10) {
          cout << " Warning: Error in Angle calculation " << endl;
          cout << " Magnitude of Vector A is " << Abar << endl;
          cout << " Magnitude of Vector B is " << Bbar << endl;
          return 0.0;
     }

     double value = dot_product(VecA, VecB) / (Abar * Bbar);

     if (value > +1.0) value = +1.0;
     if (value < -1.0) value = -1.0;

     theta = acos(value);

     if (unit_measure == ANGLE_IN_DEGREES) theta *= (180.0 / M_PI);

     return theta;
}

//////////////////////////////////////////////////////////////////////////////

template <class T, size_t n>
inline T getAngle(const Array<T, n> &pa, const Array<T, n> &pb,
                  const Array<T, n> &pc, int unit_measure = 0)
{
     Array<T, n> VecA = create_vector(pb, pa);
     Array<T, n> VecB = create_vector(pc, pa);
     return getAngle(VecA, VecB, unit_measure);
}

///////////////////////////////////////////////////////////////////////////////

inline double getTriAngle(const Point3D &pa, const Point3D &pb, const Point3D &pc)
{
     double a2   =  length2( pb, pc );
     double b2   =  length2( pc, pa );
     double c2   =  length2( pa, pb );
     double cosA =  (b2 + c2 - a2)/(2*sqrt(b2*c2) );

     if( cosA >  1.0) cosA =  1.0;
     if( cosA < -1.0) cosA = -1.0;
     return 180*acos(cosA)/M_PI;

}
///////////////////////////////////////////////////////////////////////////////

inline void getTriAngles(const Point3D &pa, const Point3D &pb, const Point3D &pc, Point3D &angles)
{
     double a2   =  length2( pb, pc );
     double b2   =  length2( pc, pa );
     double c2   =  length2( pa, pb );
     double cosA =  (b2 + c2 - a2)/(2*sqrt(b2*c2) );
     double cosB =  (a2 + c2 - b2)/(2*sqrt(a2*c2) );
     double cosC =  (a2 + b2 - c2)/(2*sqrt(a2*b2) );

     if( cosA >  1.0) cosA =  1.0;
     if( cosA < -1.0) cosA = -1.0;
     angles[0] = 180*acos(cosA)/M_PI;

     if( cosB >  1.0) cosB =  1.0;
     if( cosB < -1.0) cosB = -1.0;
     angles[1] = 180*acos(cosB)/M_PI;

     if( cosC >  1.0) cosC =  1.0;
     if( cosC < -1.0) cosC = -1.0;
     angles[2] = 180*acos(cosC)/M_PI;
}

///////////////////////////////////////////////////////////////////////////////
}

#include "FastArea.hpp"

#endif

