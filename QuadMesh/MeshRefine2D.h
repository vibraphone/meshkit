#ifndef REFINE_H
#define REFINE_H

#include <bitset>
#include "Mesh.h"
#include "EdgeFlip.h"

#include "basic_math.h"

using namespace std;

/////////////////////////////////////////////////////////////////////////////////
//   MeshRefine2D:
//   		LongestEdgeRefine2D: ConsistencyRefine2D
//   		ObtuseRefine2D: ConsistencyRefine2D
//   		GradeRefine2D: ConsistencyRefine2D
//   		Refine2D14:  ConsistencyRefine2D
//   		CentroidRefine2D
//   		DelaunayRefine2D
//   		Square3SubDivision
////////////////////////////////////////////////////////////////////////////////
//Class Refine, refines an existing triangulation. This class works for 2D cases
//or the parametric domain of 3D surface triangulation. The user need to provide
//which cells need to be refined. There could be many criteria for deviding the
//cell, so this is not part of the class. 
//
//Input :   Connection :  Connectivity of the triangulation ( 1D array)
//      :   ParamCoords:  parametric coordinates of the vertices.
//      :   markFace   :  Faces which are marked for refinement.
//
//Output:   Connection :
//          ParamCoords:
//          
//If the user is working with 3D triangulated surface, it is assumed that he 
//has some representation ( example NURBS ) from which he can calculate the
//physical Coordinates from the paramCoords. Again this is not the responsiblity
//of the class.
//
//------------------------------------------------------------------------------
//Number of Division     :   New Vertices           :  New Faces
//------------------------------------------------------------------------------
//    2                  :     1                    :  1
//    3                  :     0                    :  2
//    4                  :     3                    :  3
//------------------------------------------------------------------------------
// Default Number of Subdivisions is 2, which is conservative approach, but 
// good at equalizing the aspect ratio with respect to neighbouring triangles.
//
// Programmer : Chaman Singh Verma
// Place      : Argonne National Lab.
//              Argonne, IL, USA
//
// 
////////////////////////////////////////////////////////////////////////////////
//
/**
 * REFINE AREA         : Increase the density where grid cells have high area/volume
 * REFINE ASPECT_RATIO : Increase the aspect Ratio
 * REFINE CURVATURE    : Create high density mesh near high curvature.
 */

namespace Jaal 
{
enum RefinePolicy { CENTROID_PLACEMENT, CIRCUMCENTER_PLACEMENT, LONGEST_EDGE_BISECTION};

enum RefineObjective {REFINE_AREA, REFINE_ASPECT_RATIO, REFINE_CURVATURE};

///////////////////////////////////////////////////////////////////////////////

//! \brief 2D Mesh Refinement class.
class MeshRefine2D
{
 public:

  MeshRefine2D() { 
     boundary_split_flag = 0; 
     numIterations   = 1;
  }
  virtual ~MeshRefine2D() {}

  void setMesh( Mesh *m ) {  mesh = m; }

// void setGeometry(  const iGeom_Instance &g ) { geom = g; }

  void setBoundarySplitFlag( bool f ) { boundary_split_flag = f; }

  vector<Vertex*> getNewNodes() const { return insertedNodes; }
  vector<Face*>   getNewFaces() const { return insertedFaces; }

  size_t  getNumFacesRefined() const { return numfacesRefined; }

  virtual int execute() = 0;

  virtual int initialize();

  void setNumOfIterations( int i ) { numIterations = i; }

 protected:
    Mesh *mesh;
    struct RefinedEdge
    {
       Edge    *edge;
       Vertex  *midVertex;
    };
    map<Vertex*, vector<RefinedEdge> > refinededges;

    vector<Face*>    insertedFaces;
    vector<Vertex*>  hangingVertex, insertedNodes;
    
    int     numIterations;
    bool    boundary_split_flag;                
    size_t  numfacesRefined;

    int finalize();

    int setVertexOnEdge(Vertex *v1, Vertex *v2);
    Vertex *getVertexOnEdge(Vertex *v1, Vertex *v2) const;

    bool hasEdge( Vertex *v1, Vertex *v2) const; 
    bool allow_edge_refinement( const Edge *edge) const;

    Edge* create_new_edge( const Vertex *v1, const Vertex *v2 );

    void  append_new_node( Vertex *v0 );
    Face* append_new_triangle(Vertex *v0, Vertex *v1, Vertex *v2);
    Face* append_new_quad(Vertex *v0, Vertex *v1, Vertex *v2, Vertex *v3);

    void remove_it( Face *face) {
         face->setRemoveMark(1);
    }

};

///////////////////////////////////////////////////////////////////////////////

struct Sqrt3Refine2D : public MeshRefine2D
{
   int execute();
};

///////////////////////////////////////////////////////////////////////////////

class CentroidRefine2D : public MeshRefine2D
{
 public:
   CentroidRefine2D() {}
   CentroidRefine2D(Mesh *m) { setMesh(m); }

   int  execute();
 private:
  int atomicOp(  Face *f);
  int refine_tri( Face *f);
  int refine_quad( Face *f);
};

///////////////////////////////////////////////////////////////////////////////

class LongestEdgeRefine2D : public MeshRefine2D 
{
 public:
  LongestEdgeRefine2D()
  { 
    cutOffAspectRatio = 0.50; 
  }

  ~LongestEdgeRefine2D() {}

  void setCutOffAspectRatio(double asp) { cutOffAspectRatio = asp;}

  int  initialize();
  int  execute();

 private:
  double cutOffAspectRatio;
  int  atomicOp( const Face *face);
};

///////////////////////////////////////////////////////////////////////////////

class ConsistencyRefine2D : public MeshRefine2D
{
   public:
     ~ConsistencyRefine2D() {}

     int  initialize();
     int  execute();

   private:
     bitset<3>  edge0, edge1, edge2, bitvec;

     int   atomicOp( Face *f);
     void  refineEdge0(const Face *f);
     void  refineEdge1(const Face *f);
     void  refineEdge2(const Face *f);

     void  subDivideQuad2Tri( const vector<Vertex*>  &qnodes);
     void  makeConsistent1( Face *f );
     void  makeConsistent2( Face *f );
     void  makeConsistent3( Face *f );
     void  makeConsistent();
     void  checkFaceConsistency( Face *f);
};

///////////////////////////////////////////////////////////////////////////////

class Refine2D14 : public MeshRefine2D 
{
 public:

  ~Refine2D14() {}

  int  initialize();
  int  execute();

 private:
  int  atomicOp( Face *f);
  int  refine_tri( Face *f);
  int  refine_quad( Face *f);
};

///////////////////////////////////////////////////////////////////////////////

struct DelaunayRefinement2D : public MeshRefine2D
{
  ~DelaunayRefinement2D() {}

  int  initialize();
  int  finalize();
  int  execute() {}
};

///////////////////////////////////////////////////////////////////////////////

class ObtuseRefine2D : public MeshRefine2D
{
  public:
   ObtuseRefine2D( ) { cutoffAngle = 90.0;}

   void setCutOffAngle( double a ) { cutoffAngle = std::max(90.0, a); }

   int  initialize();
   int  execute();

  private:
   double cutoffAngle;
   int   atomicOp(const Face *f);
};

///////////////////////////////////////////////////////////////////////////////

class GradeRefine2D : public MeshRefine2D
{
  public:
   int  initialize();
   int  finalize();
   int  execute();

  private:
     int atomicOp( const Vertex *v);
};

///////////////////////////////////////////////////////////////////////////////

}

#endif
