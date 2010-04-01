#ifndef MESH_H
#define MESH_H

#include <iostream>
#include <cassert>
#include <fstream>
#include <math.h>
#include <string>
#include <values.h>

#include <vector>
#include <list>
#include <map>
#include <set>
#include <deque>
#include <algorithm>

#ifdef USE_MOAB
#include <iMesh.h>
#include <MBInterface.hpp>
#include "SimpleArray.h"
#endif

#ifdef USE_MESQUITE
#include <Mesquite_all_headers.hpp>
#endif

#define BEGIN_JAAL_NAMESPACE  namespace Jaal {
#define END_JAAL_NAMESPACE    }

#include "basic_math.h"
#include "circumcenter.h"

///////////////////////////////////////////////////////////////////////////////

using namespace std;

typedef std::pair<size_t, size_t> FacePair;

BEGIN_JAAL_NAMESPACE

class Vertex;
class Face;
class Edge;

typedef Vertex* NodeType;
typedef Edge* EdgeType;
typedef Face* FaceType;

///////////////////////////////////////////////////////////////////////////////

typedef int AttribKey;

#ifdef USE_BOOST_LIBS
struct Attribute
{
  bool operator ==(const Attribute & rhs) const
  {
    return key == rhs.key;
  }
  AttribKey key;
  boost::any value;
};
#else
struct Attribute
{
  bool operator ==(const Attribute & rhs) const
  {
    return key == rhs.key;
  }

  AttribKey key;
  union datatype_t
  {
    char cdata;
    float fdata;
    int idata;
    double ddata;
  }datatype;
};
#endif

///////////////////////////////////////////////////////////////////////////////

class MeshEntity
{
public:

#ifdef USE_BOOST_LIBS
  static AttribKey getAttribKey(const string &s);
  static AttribKey addAttribute(const string &s);
  static bool hasAttribute(const string &);
  static void removeAttribute(const string &s);
#endif

  MeshEntity()
  {
    locked = 0; // Default: No lock
    intTag = 0;
    visitMark = 0; // Default: Not yet visited.
    removeMark = 0; // Default: Active< not removable.
    constrained = 0; // Default: No Contrainted
    boundarymark = 0; // Default: Internal entity
    moab_entity_handle = (iBase_EntityHandle)0; // Default: Handle not available.
  }

  void setVisitMark(bool r)
  {
    visitMark = r;
  }

  bool isVisited() const
  {
    return visitMark;
  }

  void setRemoveMark(bool r)
  {
    removeMark = r;
  }

  void setLock()
  {
    locked = 1;
  }

  bool isLocked()
  {
    return locked;
  }

  void releaseLock()
  {
    locked = 0;
  }

  bool isRemoved() const
  {
    return removeMark;
  }

  bool isBoundary() const
  {
    return boundarymark;
  }

  void setBoundaryMark(int b)
  {
    boundarymark = b;
  }

  int getBoundaryMark() const
  {
    return boundarymark;
  }

  bool isConstrained() const
  {
    if (boundarymark > 0 || constrained) return 1;
    return 0;
  }

  void setConstrainedMark(int b)
  {
    constrained = b;
  }

  void setID(size_t id)
  {
    gid = id;
  }

  size_t getID() const
  {
    return gid;
  }

  void setTag( int v)
  {
    intTag = v;
  }
  int getTag() const
  { return intTag;}

#ifdef USE_BOOST_LIBS
  template<class T>
  void setAttribute(const string &s, const T &val)
  {
    AttribKey key = getAttribKey(s);
    setAttribute(key, val);
  }

  template<class T>
  void setAttribute(const AttribKey &k, const T &val)
  {
    Attribute property;
    property.key = k;

    list<Attribute>::iterator it;

    it = std::find(attriblist.begin(), attriblist.end(), property);

    if (it == attriblist.end())
    {
      property.value = val;
      attriblist.push_back(property);
    } else
    it->value = val;
  }

  template<class T>
  int getAttribute(const string &s, T &val) const
  {
    AttribKey key = getAttribKey(s);
    return getAttribute(key, val);
  }

  template<class T>
  int getAttribute(const AttribKey &k, T &val) const
  {
    Attribute property;
    property.key = k;

    list<Attribute>::const_iterator it;

    it = std::find(attriblist.begin(), attriblist.end(), property);

    if (it == attriblist.end()) return 1;

    boost::any anyvalue = it->value;

    if (anyvalue.type() == typeid (T))
    {
      val = boost::any_cast<T > (anyvalue);
      return 0;
    }
    return 1;
  }
#endif

  void set_MOAB_Handle( iBase_EntityHandle &h)
  { moab_entity_handle = h;}

  iBase_EntityHandle get_MOAB_Handle() const
  { return moab_entity_handle;}

  void setLayerID( int l )
  { layerID = l;}

  int getLayerID() const
  { return layerID;}

protected:
  size_t gid;
  bool constrained;
  int boundarymark;
  int intTag;
  int layerID;
  volatile bool locked;
  volatile char checkID;
  volatile bool visitMark, removeMark;

  iBase_EntityHandle moab_entity_handle;

#ifdef USE_BOOST_LIBS
  std::list<Attribute> attriblist;
  static AttribKey maxAttribID;
  static map<string, AttribKey> attribKeyMap;
#endif
};

///////////////////////////////////////////////////////////////////////////////

class Vertex : public MeshEntity
{
  static size_t global_id;

public:

  static NodeType newObject();

  Vertex()
  {
    visitMark = 0;
    removeMark = 0;
    boundarymark = 0;
    mate = 0;
    primalface = 0;
  }

  static double length(const Vertex *v0, const Vertex *v1);
  static double length2(const Vertex *v0, const Vertex *v1);
  static Point3D mid_point(const Vertex *v0, const Vertex *v1, double alpha = 0.5);

  void setXYZCoords(const Point3D &p)
  {
    xyz[0] = p[0];
    xyz[1] = p[1];
    xyz[2] = p[2];
  }

  const Point3D &getXYZCoords() const
  {
    return xyz;
  }

  int getDegree() const
  {
    int ncount = 0;
    for (size_t i = 0; i < relations00.size(); i++)
    if (!relations00[i]->isRemoved()) ncount++;
    return ncount;
  }

  void addRelation0(Vertex *vertex)
  {
    if (!hasRelation0(vertex)) relations00.push_back(vertex);
    sort(relations00.begin(), relations00.end());
  }

  void removeRelation0(const Vertex *vertex)
  {
    if (hasRelation0(vertex))
    {
      vector<NodeType>::iterator it;
      it = remove(relations00.begin(), relations00.end(), vertex);
      relations00.erase(it, relations00.end());
      sort(relations00.begin(), relations00.end());
    }
  }

  void addRelation2(Face* face)
  {
    if (!hasRelation2(face)) relations02.push_back(face);
    sort(relations02.begin(), relations02.end());
  }

  void removeRelation2(const Face *face)
  {
    if (hasRelation2(face))
    {
      vector<FaceType>::iterator it;
      it = remove(relations02.begin(), relations02.end(), face);
      relations02.erase(it, relations02.end());
      sort(relations02.begin(), relations02.end());
    }
  }

  void clearRelations(int t)
  {
    if (t == 0) relations00.clear();
    if (t == 2) relations02.clear();
  }

  const vector<NodeType> &getRelations0() const
  {
    return relations00;
  }

  const vector<FaceType> &getRelations2() const
  {
    return relations02;
  }

  bool hasRelation0(const Vertex* vertex) const
  {
    vector<NodeType>::const_iterator it;

    it = find(relations00.begin(), relations00.end(), vertex);
    if (it == relations00.end()) return 0;

    return 1;
  }

  bool hasRelation2(const Face* face) const
  {
    vector<FaceType>::const_iterator it;

    it = find(relations02.begin(), relations02.end(), face);
    if (it == relations02.end()) return 0;

    return 1;
  }

  void setDualMate(Vertex *v)
  {
    mate = v;
  }

  Vertex* getDualMate() const
  {
    return mate;
  }

  void setPrimalFace( Face *f)
  {
    primalface = f;
  }

  Face* getPrimalFace() const
  { return primalface;}

  Vertex* getClone() const;

  void setFeatureLength( double f )
  { featureLength = f;}

  double getFeatureLength( ) const
  { return featureLength;}

private:
  // void * operator new( size_t size, void *);

  double featureLength;

  Vertex *mate;
  Face *primalface;
  vector<NodeType> relations00; // vertex-vertex
  vector<FaceType> relations02; // vertex-face

  Point3D xyz;
};

///////////////////////////////////////////////////////////////////////////////

class Edge : public MeshEntity
{
public:
  static EdgeType newObject();

  Edge()
  {
  }

  Edge(NodeType n1, NodeType n2)
  {
    setNodes(n1,n2);
  }

  void setNodes(const vector<NodeType> &v)
  {
    assert(v.size() == 2);
    setNodes( v[0], v[1] );
  }

  void setNodes( Vertex *v1, Vertex *v2 )
  {
    assert( v1 != v2 );
    connect[0] = v1;
    connect[1] = v2;
  }

  NodeType getNodeAt(int id) const
  {
    return connect[id];
  }

private:
  //  void * operator new( size_t size, void *);

  NodeType connect[2];
};

///////////////////////////////////////////////////////////////////////////////

class Face : public MeshEntity
{
public:
  static const int TRIANGLE = 3;
  static const int QUADRILATERAL = 4;

  static FaceType newObject();
  static FaceType create_quad(const FaceType t1, const FaceType t2);

  /////////////////////////////////////////////////////////////////////////////
  // Use modified Heron formula for find out the area of a triangle
  /////////////////////////////////////////////////////////////////////////////

  static double   tri_area ( const Point3D &p0, const Point3D &p1, const Point3D &p2);

  static Vec3D    normal( const Vertex *v0,  const Vertex *v1, const Vertex *v2 );
  static Vec3D    normal( const Point3D &p0, const Point3D &p1, const Point3D &v2 );

  /////////////////////////////////////////////////////////////////////////////
  // Calculate Area of Quadrilateral:
  // Example : Convex Case 
  //           Coorindates  (0.0,0.0)  ( 1.0, 1.0), (2.0, 0.0), (1,0, -1.0)
  //           Result :  2*( 0.5*2.0*1.0 ) = 2.0;
  //           Concave case:
  //           Coodinates: ( 0.0, 0.0), 10, 2), (2.0, 0.0), (10, -2)
  //           Result : 2 *( 0.5*2.0* 2.0) = 4.0
  /////////////////////////////////////////////////////////////////////////////
  static double   quad_area( const Point3D &p0, const Point3D &p1,
                             const Point3D &p2, const Point3D &p3);

  static bool is_convex_quad(const Point3D &p0, const Point3D &p1,
                             const Point3D &p2, const Point3D &p3);
  Face()
  {
    removeMark = 0;
    boundarymark = 0,
    visitMark = 0;
    dualnode = 0;
  }

  static NodeType opposite_node(const FaceType tri, NodeType n1, NodeType n2);
  static void opposite_nodes(const FaceType quad, NodeType n1, NodeType n2,
      NodeType &n3, NodeType &n4);

  static int check_on_boundary(const FaceType tri);

  int getType() const
  {
    if( connect.size() == 3) return Face::TRIANGLE;
    if( connect.size() == 4) return Face::QUADRILATERAL;
    return 0;
  }

  bool hasNode(const NodeType &vertex) const
  {
    if (find(connect.begin(), connect.end(), vertex) != connect.end()) return 1;
    return 0;
  }

  int queryNodeAt(const NodeType &vertex) const
  {
    for (size_t i = 0; i < connect.size(); i++)
    if (connect[i] == vertex) return i;

    return -1;
  }

  void reverse()
  { std::reverse( connect.begin(), connect.end() );}

  int getOrientation( const Vertex *ev0, const Vertex *ev1) const
  {
    size_t nsize = connect.size();

    for( int i = 0; i < nsize; i++)
    {
      Vertex *v0 = connect[(i+0)%nsize];
      Vertex *v1 = connect[(i+1)%nsize];
      if( v0 == ev0 && v1 == ev1 ) return 1;
      if( v0 == ev1 && v1 == ev0 ) return -1;
    }
    return 0;
  }

  int replaceNode(const Vertex *oldvertex, Vertex *newvertex)
  {

    for (size_t i = 0; i < connect.size(); i++)
    {
      if (connect[i] == oldvertex)
      {
        connect[i] = newvertex;
        return 0;
      }
    }
    return 1;
  }

  int getSize(int etype) const
  {
    if (etype == 0) return connect.size();
    return 0;
  }

  void setNodes(const vector<NodeType> &v)
  {
    connect = v;
  }

  const vector<NodeType> &getNodes() const
  {
    return connect;
  }

  NodeType getNodeAt(int id) const
  {
    return connect[id];
  }

  Point3D getCentroid() const;

  void setDualNode(const NodeType n)
  {
    dualnode = n;
  }

  NodeType getDualNode() const
  {
    return dualnode;
  }

  NodeType getNewDualNode()
  {
    if( dualnode ) delete dualnode;
    dualnode = Vertex::newObject(); assert(dualnode);
    Point3D p3d = getCentroid();
    dualnode->setXYZCoords( p3d );
    dualnode->setPrimalFace( this );
    return dualnode;
  }

  bool hasBoundaryNode() const
  {
    for( int i = 0; i < connect.size(); i++)
    if( connect[i]->isBoundary() ) return 1;
    return 0;
  }

  bool hasBoundaryEdge() const
  {
    int nSize = connect.size();
    for( int i = 0; i < nSize; i++)
    if( connect[i]->isBoundary() && connect[(i+1)%nSize]->isBoundary() ) return 1;
    return 0;
  }

  vector<Face*> getRelations202();  
  vector<Face*> getRelations212();
  vector<Vertex*> getRelations0();

  bool isConvex()
  {
     if( connect.size() <= 3 ) return 1;
     if( connect.size() == 4 ) 
         return is_convex_quad( connect[0]->getXYZCoords(),
	                        connect[1]->getXYZCoords(),
	                        connect[2]->getXYZCoords(),
	                        connect[3]->getXYZCoords() );
     return 0;
  }

  double getAspectRatio();
  double getArea()
  {
     if( connect.size() == 3 ) {
         return tri_area( connect[0]->getXYZCoords(),
	                   connect[1]->getXYZCoords(),
	                   connect[2]->getXYZCoords() );
     }

     if( connect.size() == 4 ) {
         return quad_area( connect[0]->getXYZCoords(),
	                   connect[1]->getXYZCoords(),
	                   connect[2]->getXYZCoords(),
	                   connect[3]->getXYZCoords() );
     }
     return 0.0;
  }

private:
  // void * operator new( size_t size, void *);

#ifdef USE_MOAB
  iBase_EntityHandle faceHandle;
#endif
  NodeType dualnode;
  vector<NodeType> connect;
};

///////////////////////////////////////////////////////////////////////////////

class Mesh
{

public:
  static vector<FaceType> getRelations112(NodeType v0, NodeType v1);
  static vector<FaceType> getRelations102(NodeType v0, NodeType v1);
  static int  make_chain(vector<Edge> &edges);
  static int  is_closed_chain(const vector<Edge> &edges);
  static int  is_closeable_chain(const vector<Edge> &edges);
  static int  rotate_chain(vector<Edge> &edges, Vertex* start_vertex);

  typedef vector<NodeType> NodeContainer;
  typedef vector<EdgeType> EdgeContainer;
  typedef vector<FaceType> FaceContainer;

  Mesh()
  {
    for (int i = 0; i < 4; i++)
    {
      for (int j = 0; j < 4; j++) adjTable[i][j] = 0;
    }
    hasdual = 0;
    meshname = "unknown";
    boundary_status = 0;
  }

  ~Mesh()
  {
    clearAll();
  }
  void setName(const string &s)
  { meshname = s;}
  string getName() const
  { return meshname;}

  void reserve( size_t nSize, int entity)
  {
    if( entity == 0) nodes.reserve( nSize);
    if( entity == 1) edges.reserve( nSize);
    if( entity == 2) faces.reserve( nSize);
  }

  bool hasDual() const
  {
    return hasdual;
  }

  void setDual(bool d)
  {
    hasdual = d;
  }

  int isHomogeneous() const;

  int getNumOfComponents();

  int readData(const string &f);

  bool getAdjTable(int i, int j) const
  {
    return adjTable[i][j];
  }

  int getEulerCharacteristic()
  {
    size_t F = getSize(2);
    size_t E = getSize(1);
    size_t V = getSize(0);

    return F - E + V;
  }

  int nearest_neighbour( const Vertex *v, Vertex* &neigh, double &d);

  size_t getSize(int d)
  {
    if (d == 0) return nodes.size();
    if (d == 1) return count_edges();
    if (d == 2) return faces.size();
    return 0;
  }

  // If the topology of the mesh is changed, make sure to setBoundaryStatus = 0
  // so that it is again determined. 
  void setBoundaryStatus( bool v ) { boundary_status = v; }

  // If the boundary nodes and faces are knowm, return the value = 1. otherwise 0
  bool getBoundaryStatus() const { return boundary_status; }

  // A mesh is simple, when each edge is shared by at the most two faces which is
  // topological simple. A geometrically simple mesh will not have any crossing,
  // or overlapping edges, but this has not been checked right now.
  bool isSimple();

  // A mesh is consistently oriented when an edge shared by two faces, traversed in
  // opposite direction. Such a mesh will have proper normals ( either all inside or
  // or all outside ).
  bool isConsistentlyOriented();

  //  Make the mesh consistent. 
  void makeConsistentlyOriented();

  //  How many strongly connected components this mesh has ?
  int getNumOfConnectedComponents();

  //  Get #of boundary nodes, edges, and faces information.
  size_t getBoundarySize(int d) const;

  // Appends a node in the mesh.
  void addNode(NodeType v)
  {
    nodes.push_back(v);
  }

  // Appends a bulk of nodes in the mesh.
  void addNodes( const vector<NodeType> &vnodes)
  {
    for( size_t i = 0; i < vnodes.size(); i++)
    addNode( vnodes[i] );
  }

  // Get the node at the specified position in the container.
  NodeType getNodeAt(size_t id) const
  {
    assert(id < nodes.size());
    return nodes[id];
  }

  // Get All the nodes in the mesh after lazy prunning ( Garbage collection ).
  const vector<NodeType> &getNodes()
  { return nodes;}

  // Add a face in the mesh. No duplication is checked..
  void addFace(FaceType v)
  {
    faces.push_back(v);
  }

  // Add bulk of faces in the mesh. No duplication is checked..
  void addFaces( vector<FaceType> & vfaces)
  {
    for( size_t i = 0; i < vfaces.size(); i++)
    addFace( vfaces[i] );
  }

  // Get the face at the specified position in the container.
  FaceType getFaceAt(size_t id) const
  {
    assert(id < faces.size());
    return faces[id];
  }

  // Get rid of entities which are marked "removed" from the mesh. 
  // a la Lazy garbage collection.
  //
  void prune();

  // Check if the lazy garbage collection is performed..
  bool isPruned() const;

  // Renumber mesh entities starting from index = 0
  void enumerate(int etype);

  // Search the boundary of the mesh (nodes, edges, and faces).
  int search_boundary();

  // Build entity-entity relations.
  int build_relations(int src, int dst)
  {
    if (src == 0 && dst == 0) return build_relations00();
    if (src == 0 && dst == 2) return build_relations02();
    return 1;
  }

  // clean specified entity-entity relations.
  void clear_relations(int src, int dst);

  // Return all the edges of the primal mesh ...
  EdgeContainer getEdges();

  // Return all the edges of dual mesh...
  EdgeContainer getDualEdges() const;

  // Returm all the matching edges of the dual graph...
  EdgeContainer getMatchingDuals() const;

  // Filter out face.
  FaceContainer filter( int facetype) const;

  // Save the mesh and all its attributes ( in the simple format ).
  void saveAs(const string &s);

  // Empty every thing in the Mesh, but don't delete the objects.
  void emptyAll();

  // Empty every thing in the Mesh, and also deallocate all the objects.
  void clearAll();

  // Reverse the connection of all the faces in the mesh. This will be flip
  // the normal of each face.
  void reverse()
  {
    for( size_t i = 0; i < faces.size(); i++)
         faces[i]->reverse();;
  }

  // Collect topological information. Ideally an internal vertex has 4 faces.
  vector<int> get_topological_statistics(int entity = 0, bool sorted = 1);

  // Collect nodes and faces in Depth First Sequence ...
  vector<Vertex*> get_Depth_First_Ordered_Nodes( Vertex *f = NULL );
  vector<Face*> get_Depth_First_Ordered_Faces( Face *f = NULL );

  // Collect nodes and faces in Breadth First Sequence ...
  vector<Vertex*> get_Breadth_First_Ordered_Nodes( Vertex *f = NULL );
  vector<Face*> get_Breadth_First_Ordered_Faces( Face *f = NULL );
  //
  // Creates waves of nodes/faces starting from the boundary. Each vertex/face
  // is assigned layerID, denoting the position in the wave fronts.
  // Used in smoothing the nodes in Advancing front style.
  //
  int setWavefront(int ofwhat);

  //  Converts the mesh into MOAB data structures.
  int toMOAB( iMesh_Instance &imesh, iBase_EntitySetHandle eset = 0 );

  //  Fill the mesh from MOAB..
  int fromMOAB( iMesh_Instance imesh, iBase_EntitySetHandle eset = 0);

  int check_unused_objects();

  //
  // Connect a strip of faces. Termination occurs when the face is reached
  // (1) At the boundary (2) Return back to the starting face.
  // There will be two strips per quadrilateral which are orthogonal to each
  // other. Using in the Ring Collapse Simplification
  //
  void get_quad_strips(Face *rootface, vector<Face*> &strip1,
      vector<Face*> &strip2);

  //
  void set_strip_markers();

  // Collect all the boundary nodes in the mesh..
  vector<Vertex*> get_bound_nodes();

  //
  // Collect all the boundary faces in the mesh. The boundary faces could be
  // defined in two ways, bound_what flag is used for that purpose.
  // bound_what  = 0   At least one vertex is on the boundary
  //             = 1   At least one edge is on the boundary.
  //
  vector<Face*> get_bound_faces( int bound_what = 1);

  //
  // Get the Aspect ratio of each face in the mesh. Aspect ratio is defined
  // as min_edge_length/max_edge_length..
  //
  vector<float> getAspectRatio(bool sorted = 1);

  //
  // Get unsigned surface area of the mesh. The calculation is for both 
  // 2D and 3D surface elements...
  //
  double  getSurfaceArea();

  // Check the Convexity..
  int check_convexity();

  // Collect Quadlity Statistics
  int get_quality_statistics( const string &s );

  //
  // Check if the surface triangulation is Delaunay. Works only for
  // triangle mesh. We use Jonathan Shewchuk's predicates and all his
  // ideas. For 2D check with Circum-Circle and for 2D Equitorial Sphere.
  //
  bool isDelaunay(); 

  //  
  // If the surface triangulation is not Delaunay, use edge-flips to 
  // tranform the mesh into Delaunay.
  //
  int makeDelaunay();


private:
  iBase_EntityHandle get_MOAB_Handle(iMesh_Instance imesh, Vertex *v);
  iBase_EntityHandle get_MOAB_Handle(iMesh_Instance imesh, Face *v);

  volatile char adjTable[4][4];
  size_t count_edges();
  string meshname;

  bool hasdual;
  bool boundary_status;

  NodeContainer nodes;
  EdgeContainer edges;
  FaceContainer faces;

  string filename;
  std::map<int, int> global2local;
  void readTriNodes(const string &s);
  void readTriEdges(const string &s);
  void readTriFaces(const string &s);

  int build_relations00();
  int build_relations02();

  int setNodeWavefront();
  int setFaceWavefront();

  int read_off_format_data( const string &s);
  int read_simple_format_data( const string &s);
  int read_triangle_format_data( const string &s);
};

struct MeshOptimization
{
};

///////////////////////////////////////////////////////////////////////////////
// Graph Matching operations ....
///////////////////////////////////////////////////////////////////////////////

int quadrangulate(Mesh *mesh);

////////////////////////////////////////////////////////////////////////////////
//Helper functions ....
////////////////////////////////////////////////////////////////////////////////
Mesh *struct_tri_grid(int nx, int ny);
Mesh *struct_quad_grid(int nx, int ny);

////////////////////////////////////////////////////////////////////////////////
// Mesh Optimization ...
////////////////////////////////////////////////////////////////////////////////

void laplacian_smoothing(Mesh *mesh, int numIters, int verbose = 0);
void laplacian_smoothing(Mesh *mesh, vector<Vertex*> &q, int numIters, int verbose = 0);
void laplacian_smoothing(iMesh_Instance imesh, int numIters);

int mesh_shape_optimization(iMesh_Instance imesh );

#ifdef USE_MOAB

vector<int> getVertexFaceDegrees(iMesh_Instance &imesh);

int readMeshData(iMesh_Instance &imesh, const string &s);

int quadrangulation(iMesh_Instance &imesh, iBase_EntitySetHandle input_mesh = 0,
    bool replace = true, iBase_EntitySetHandle *output_mesh = NULL);

void mesh_optimization(Mesh *mesh);

inline Point3D make_vector( const Vertex* head, const Vertex *tail)
{
  Point3D phead = head->getXYZCoords();
  Point3D ptail = tail->getXYZCoords();

  Point3D xyz;
  xyz[0] = phead[0] - ptail[0];
  xyz[1] = phead[1] - ptail[1];
  xyz[2] = phead[2] - ptail[2];

  return xyz;
}

#endif

END_JAAL_NAMESPACE


#endif

