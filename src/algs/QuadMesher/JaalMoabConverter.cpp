#include "meshkit/JaalMoabConverter.hpp"

iBase_EntityHandle
JaalMoabConverter :: new_MOAB_Handle(iMesh_Instance imesh, Jaal::Vertex *vertex)
{
     int err;
     iBase_EntityHandle newHandle;

     std::map<PNode, iBase_EntityHandle> ::iterator it;
     it = moabnode.find(vertex);

     if( it != moabnode.end() ) {
          newHandle = it->second;
          iMesh_deleteEnt(imesh, newHandle, &err);
          assert( !err );
     }

     Point3D p = vertex->getXYZCoords();
     iMesh_createVtx(imesh, p[0], p[1], p[2], &newHandle, &err);
     assert( !err );

     moabnode[vertex]    = newHandle;
     jaalnode[newHandle] = vertex;
     return newHandle;
}


///////////////////////////////////////////////////////////////////////////////

iBase_EntityHandle
JaalMoabConverter ::new_MOAB_Handle(iMesh_Instance imesh, Face *face)
{
     int status, err;

     iBase_EntityHandle newHandle;

     std::map<PFace, iBase_EntityHandle> ::iterator fit;
     fit = moabface.find(face);

     if( fit != moabface.end() ) {
          newHandle = fit->second;
          iMesh_deleteEnt(imesh, newHandle, &err);
          assert(!err);
     }

     vector<iBase_EntityHandle> connect;

     int nnodes = face->getSize(0);
     connect.resize(nnodes);

     std::map<PNode, iBase_EntityHandle> ::iterator nit;
     for (int j = 0; j < nnodes; j++) {
          Vertex *v  = face->getNodeAt(j);
          nit = moabnode.find( v );
          assert( nit != moabnode.end() );
          connect[j] = nit->second;
     }

     switch (nnodes) {
     case 3:
          iMesh_createEnt(imesh, iMesh_TRIANGLE, &connect[0], nnodes, &newHandle,
                          &status, &err);
          assert(!err);
          break;
     case 4:
          iMesh_createEnt(imesh, iMesh_QUADRILATERAL, &connect[0], nnodes,
                          &newHandle, &status, &err);
          assert(!err);
          break;
     default:
          iMesh_createEnt(imesh, iMesh_POLYGON, &connect[0], nnodes, &newHandle,
                          &status, &err);
          assert(!err);
          break;
     }

     moabface[face]  = newHandle;
     jaalface[newHandle] = face;

     return newHandle;
}

///////////////////////////////////////////////////////////////////////////////

int
JaalMoabConverter::toMOAB(Jaal::Mesh *jmesh, iMesh_Instance &imesh, iBase_EntitySetHandle entitySet)
{
     int err;

     assert( jmesh->isPruned() );

     if (imesh == 0) {
          int optlen = 0;
          char *options = NULL;
          iMesh_newMesh(options, &imesh, &err, optlen);
          assert(!err);
     }

     iBase_EntityHandle newHandle;
     std::map<PNode, iBase_EntityHandle> ::const_iterator niter;

     size_t numnodes = jmesh->getSize(0);

     size_t ncount0 = 0, ncount1 = 0;
     for (size_t i = 0; i < numnodes; i++) {
          Vertex *v = jmesh->getNodeAt(i);
          niter = moabnode.find(v);
          if( niter == moabnode.end() ) {
               ncount0++;
               newHandle = new_MOAB_Handle(imesh, v);
               if (entitySet) {
                    iMesh_addEntToSet(imesh, newHandle, entitySet, &err);
                    assert( !err );
               }
          } else {
               ncount1++;
               newHandle = niter->second;
               Point3D p3d = v->getXYZCoords();
               iMesh_setVtxCoord(imesh, newHandle, p3d[0], p3d[1], p3d[2], &err);
               assert( !err );
          }
     }

     size_t numfaces = jmesh->getSize(2);
     for (size_t i = 0; i < numfaces; i++) {
          Face *f = jmesh->getFaceAt(i);
          newHandle = new_MOAB_Handle(imesh, f);

          if (entitySet) {
               iMesh_addEntToSet(imesh, newHandle, entitySet, &err);
               assert( !err );
          }
     }
     return 0;
}

/////////////////////////////////////////////////////////////////////////////////////

Jaal::Mesh*
JaalMoabConverter ::fromMOAB(iMesh_Instance imesh, Jaal::Mesh *m, iBase_EntitySetHandle entitySet)
{
     jmesh = m;
     if( jmesh == NULL ) jmesh = new Jaal::Mesh;

     int err;

     if (entitySet == 0)
          iMesh_getRootSet(imesh, &entitySet, &err);

     SimpleArray<iBase_EntityHandle> tfaceHandles;
     iMesh_getEntities(imesh, entitySet, iBase_FACE, iMesh_TRIANGLE, ARRAY_INOUT(tfaceHandles), &err);

     size_t numFaces= tfaceHandles.size();

     SimpleArray<iBase_EntityHandle> facenodes;

     jaalnode.clear();
     for( int i = 0; i < (int) numFaces; i++) {
          iMesh_getEntAdj(imesh, tfaceHandles[i], iBase_VERTEX, ARRAY_INOUT(facenodes), &err);
          for (int j = 0; j < 3; j++)
               jaalnode[ facenodes[j] ] = NULL;
     }

     size_t numNodes = jaalnode.size();

     jmesh->reserve(numNodes, 0);

     Point3D p3d;
     double x, y, z;

     std::map<iBase_EntityHandle, PNode> ::const_iterator niter;
     Vertex *jnode;

     size_t id = 0;
     for( niter = jaalnode.begin(); niter != jaalnode.end(); ++niter) {
          iBase_EntityHandle nhandle = niter->first;
          iMesh_getVtxCoord(imesh, nhandle, &x, &y, &z, &err);
          p3d[0] = x;
          p3d[1] = y;
          p3d[2] = z;
          jnode  = Jaal::Vertex::newObject();
          jnode->setXYZCoords(p3d);
          jnode->setID(id++);
          jmesh->addNode(jnode);
          moabnode[jnode] = nhandle;
          jaalnode[ nhandle ] = jnode;
     }

     jmesh->reserve(numFaces, 2);

     NodeSequence connect(3);

     for (size_t i = 0; i < numFaces; i++) {
          SimpleArray<iBase_EntityHandle> facenodes;
          iMesh_getEntAdj(imesh, tfaceHandles[i], iBase_VERTEX, ARRAY_INOUT(facenodes), &err);
          for (int j = 0; j < 3; j++)
               connect[j] = jaalnode[facenodes[j]];
          Face *face = Face::newObject();;
          face->setNodes(connect);
          jmesh->addFace(face);
          jaalface[tfaceHandles[i] ] = face;
          moabface[face] = tfaceHandles[i];
     }

     return jmesh;
}
