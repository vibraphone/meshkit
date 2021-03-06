#include "GFace.h"
#include <sstream>
#include <fstream>
#include <limits>
#include <set>

void iGeom_getEntAdj_Ext(iGeom  *geom,
                         iBase_EntityHandle entity_handle,
                         int to_dimension,
                         std::vector<iBase_EntityHandle> &entities)
{
     vector<iBase_EntityHandle> edges;
     int err = geom->getEntAdj(entity_handle, iBase_EDGE, edges);

     // First Edge is always correct. It is the reference.
     set<iBase_EntityHandle> edgeset;
     for (int i = 1; i < edges.size(); i++) edgeset.insert(edges[i]);

     iBase_EntityHandle start_vertex, next_vertex;
     vector<iBase_EntityHandle> edgenodes;
     err = geom->getEntAdj(edges[0], iBase_VERTEX, edgenodes);

     start_vertex = edgenodes[0];
     if (edgenodes.size() > 1) next_vertex = edgenodes[1];

     entities.clear();
     entities.push_back(start_vertex);

     iBase_EntityHandle curr_edge;
     for (int i = 1; i < edges.size(); i++) {
          entities.push_back(next_vertex);

          BOOST_FOREACH(curr_edge, edgeset) {
               err = geom->getEntAdj(curr_edge, iBase_VERTEX, edgenodes);
               if (edgenodes[0] == next_vertex) {
                    edges[i] = curr_edge;
                    next_vertex = edgenodes[1];
                    edgeset.erase(curr_edge);
                    break;
               }
               if (edgenodes[1] == next_vertex) {
                    edges[i] = curr_edge;
                    next_vertex = edgenodes[0];
                    edgeset.erase(curr_edge);
                    break;
               }
          }
     }
     assert(next_vertex == start_vertex);
     assert(entities.size() == edges.size());

     if (to_dimension == iBase_VERTEX) return;

     entities.clear();
     for (int i = 0; i < edges.size(); i++)
          entities.push_back(edges[i]);

     if (to_dimension == iBase_EDGE) return;

     cout << "Warning: Not yet implemented " << endl;
     exit(0);
}


///////////////////////////////////////////////////////////////////////////////

int GFace::get_hex_face_number(const vector<iBase_EntityHandle> &cellnodes,
                               const vector<iBase_EntityHandle> &facenodes,
                               int &side_no, int &orientation)
{
     static vector< vector<iBase_EntityHandle> > cellfaces(6);
     for (int i = 0; i < 6; i++) cellfaces[i].resize(4);

     cellfaces[0][0] = cellnodes[0];
     cellfaces[0][1] = cellnodes[3];
     cellfaces[0][2] = cellnodes[7];
     cellfaces[0][3] = cellnodes[4];

     cellfaces[1][0] = cellnodes[1];
     cellfaces[1][1] = cellnodes[2];
     cellfaces[1][2] = cellnodes[6];
     cellfaces[1][3] = cellnodes[5];

     cellfaces[2][0] = cellnodes[0];
     cellfaces[2][1] = cellnodes[1];
     cellfaces[2][2] = cellnodes[5];
     cellfaces[2][3] = cellnodes[4];

     cellfaces[3][0] = cellnodes[3];
     cellfaces[3][1] = cellnodes[2];
     cellfaces[3][2] = cellnodes[6];
     cellfaces[3][3] = cellnodes[7];

     cellfaces[4][0] = cellnodes[0];
     cellfaces[4][1] = cellnodes[1];
     cellfaces[4][2] = cellnodes[2];
     cellfaces[4][3] = cellnodes[3];

     cellfaces[5][0] = cellnodes[4];
     cellfaces[5][1] = cellnodes[5];
     cellfaces[5][2] = cellnodes[6];
     cellfaces[5][3] = cellnodes[7];

     side_no = -1;
     vector<iBase_EntityHandle>::const_iterator istart, iend;
     for (int i = 0; i < 6; i++) {
          istart = cellfaces[i].begin();
          iend = cellfaces[i].end();
          int found = 1;
          for (int j = 0; j < 4; j++) {
               if (std::find(istart, iend, facenodes[j]) == iend) {
                    found = 0;
                    break;
               }
          }
          if (found) side_no = i;
     }

     assert(side_no >= 0 && side_no < 6);

     return 0;
}


//////////////////////////////////////////////////////////////////////////////////

GFace::GFace( iBase_EntityHandle h, iGeom *g, iMesh *m, iRel *r, iRel::PairHandle *p)
{
     gFaceHandle = h;
     geom = g;
     mesh = m;
     rel  = r;
     relPair = p;

     kdtree = NULL;

     int err;
     err = geom->getEntUVRange(gFaceHandle, umin, vmin, umax, vmax);
     assert(!err);

     err = geom->getEntBoundBox(gFaceHandle, xmin, ymin, zmin, xmax, ymax, zmax);

     periodic[0] = 0;
     periodic[1] = 0;
     bool in_u, in_v;
     err = geom->isEntPeriodic(gFaceHandle, in_u, in_v);
     assert(!err);
     if (in_u) periodic[0] = 1;
     if (in_v) periodic[1] = 1;

     xlength = fabs(xmax - xmin);
     ylength = fabs(ymax - ymin);
     zlength = fabs(zmax - zmin);
     maxlength = max(xlength, max(ylength, zlength));

     uvCoords = NULL;
     numNeighs = 1;
     annIdx.resize(numNeighs);
     anndist.resize(numNeighs);
     build_kdtree();

}

///////////////////////////////////////////////////////////////////////////////

GFace::~GFace()
{
     delete_kdtree();
}

///////////////////////////////////////////////////////////////////////////////

bool GFace::hasSeam() const
{
     int err, sense_out;

     vector<iBase_EntityHandle> edgeHandles;
     err = geom->getEntAdj(gFaceHandle, iBase_EDGE, edgeHandles);
     assert(!err);

     int nSize = edgeHandles.size();
     for (int i = 0; i < nSize; i++) {
          err = geom->getEgFcSense(edgeHandles[i], gFaceHandle, sense_out);
          if (sense_out == 0) return 1;
     }
     return 0;

}

///////////////////////////////////////////////////////////////////////////////

void GFace::build_kdtree()
{
     delete_kdtree(); // If Allocated earlier

     int err;

     int N = 1000;
     double x, y, z;

     double du = (umax - umin) / (double) N;
     double dv = (vmax - vmin) / (double) N;

     int numnodes = (N + 1)*(N + 1);
     kdnodes = annAllocPts(numnodes, 3);

     uvCoords = new double[2 * numnodes];

     int index = 0;
     for (int j = 0; j < N + 1; j++) {
          double v = vmin + j*dv;
          if (v > vmax) v = vmax;
          for (int i = 0; i < N + 1; i++) {
               double u = umin + i*du;
               if (u > umax) u = umax;
               err = geom->getEntUVtoXYZ(gFaceHandle, u, v, x, y, z);
               kdnodes[index][0] = x;
               kdnodes[index][1] = y;
               kdnodes[index][2] = z;
               uvCoords[2 * index + 0] = u;
               uvCoords[2 * index + 1] = v;
               index++;
          }
     }

     kdtree = new ANNkd_tree(kdnodes, numnodes, 3);
     assert(kdtree);
}


///////////////////////////////////////////////////////////////////////////////

void GFace::delete_kdtree()
{
     if (kdtree == NULL) return;

     annDeallocPts(kdnodes);
     delete [] uvCoords;
     delete kdtree;
     kdtree = NULL;
     annClose();
}

///////////////////////////////////////////////////////////////////////////////


double GFace::getGeodesicLength(const Point2D &u0, const Point2D &uN) const
{
     int nsub = 2;

     int err;
     double u, v, du, dv;

     double x0, y0, z0;
     double x1, y1, z1;
     double dx, dy, dz, dl, arclen, maxlen;

     maxlen = 0.0;
     for (int ilevel = 0; ilevel < 10; ilevel++) {
          du = (uN[0] - u0[0]) / (double) nsub;
          dv = (uN[1] - u0[1]) / (double) nsub;

          err = geom->getEntUVtoXYZ(gFaceHandle, u0[0], u0[1], x0, y0, z0);
          assert(!err);

          arclen = 0.0;
          for (int i = 0; i < nsub; i++) {
               u = u0[0] + (i + 1) * du;
               v = u0[1] + (i + 1) * dv;
               err = geom->getEntUVtoXYZ(gFaceHandle, u, v, x1, y1, z1);
               assert(!err);
               dx = x1 - x0;
               dy = y1 - y0;
               dz = z1 - z0;
               dl = sqrt(dx * dx + dy * dy + dz * dz);
               arclen += dl;
               x0 = x1;
               y0 = y1;
               z0 = z1;
          }
          if (fabs(arclen - maxlen) < 1.0E-06) break;
          maxlen = arclen;
          nsub *= 2;
     }
     return arclen;
}

///////////////////////////////////////////////////////////////////////////////

int GFace::getNormal(const Point2D &uv, Vec3D &avec) const
{
     assert(kdtree);

     int err;
     double nx, ny, nz;
     err = geom->getEntNrmlUV(gFaceHandle, uv[0], uv[1], nx, ny, nz);
     assert(!err);

     double mag = 1.0 / sqrt(nx * nx + ny * ny + nz * nz);

     avec[0] = nx*mag;
     avec[1] = ny*mag;
     avec[2] = nz*mag;

     return 0;
}
///////////////////////////////////////////////////////////////////////////////

int GFace::getFirstDer(const Point2D &uv, Vec3D &du, Vec3D &dv) const
{
     int err;
     vector<double> uDeriv, vDeriv;
     /*
         err = geom->getEnt1stDrvt(gFaceHandle, uv[0], uv[1], uDeriv, vDeriv);
         assert(!err);

         Vec3D du;
         du[0] = uDeriv[0];
         du[1] = uDeriv[1];
         du[2] = uDeriv[2];

         Vec3D dv;
         dv[0] = vDeriv[0];
         dv[1] = vDeriv[1];
         dv[2] = vDeriv[2];

         return std::make_pair(du, dv);
     */
}

///////////////////////////////////////////////////////////////////////////////

int GFace::getXYZCoords(const Point2D &uv, Point3D &p3d) const
{
     int err;
     err = geom->getEntUVtoXYZ(gFaceHandle, uv[0], uv[1], p3d[0], p3d[1], p3d[2] );
     assert(!err);
     return err;
}

///////////////////////////////////////////////////////////////////////////////

int GFace::getUVCoords(const Point3D &xyz, Point2D &uv) const
{
     assert(kdtree);

     double tol = 1.0E-06;
     int err;
     double x, y, z, u, v, dx, dy, dz, derr;

     double xq = xyz[0];
     double yq = xyz[1];
     double zq = xyz[2];

     if (xq < xmin || xq > xmax)
          cout << "Warning: Query point outside X Range " << endl;

     if (yq < ymin || yq > ymax)
          cout << "Warning: Query point outside Y Range " << endl;

     if (zq < zmin || zq > zmax)
          cout << "Warning: Query point outside Z Range " << endl;

     err = geom->getEntXYZtoUV(gFaceHandle, xq, yq, zq, u, v);
     assert(!err);

     err = geom->getEntUVtoXYZ(gFaceHandle, u, v, x, y, z);
     assert(!err);

     dx = fabs(x - xq);
     dy = fabs(y - yq);
     dz = fabs(z - zq);
     derr = dx * dx + dy * dy + dz*dz;

     if (derr < tol * tol) {
          uv[0] = u;
          uv[1] = v;
          return 0;
     }

     double queryPoint[3], eps = 0.0;
     double xon, yon, zon;
     double dist1, dist2;

     queryPoint[0] = xq;
     queryPoint[1] = yq;
     queryPoint[2] = zq;

     kdtree->annkSearch(queryPoint, numNeighs, &annIdx[0], &anndist[0], eps);

     int index = annIdx[0];
     dist1 = anndist[0];

     x = kdnodes[index][0];
     y = kdnodes[index][1];
     z = kdnodes[index][2];
     u = uvCoords[2 * index + 0];
     v = uvCoords[2 * index + 1];

     double uguess = u;
     double vguess = v;

     err = geom->getEntXYZtoUVHint( gFaceHandle, x, y, z, uguess, vguess);
     err = geom->getEntUVtoXYZ(gFaceHandle, uguess, vguess, xon, yon, zon);
     dx = queryPoint[0] - xon;
     dy = queryPoint[1] - yon;
     dz = queryPoint[2] - zon;
     dist2 =  dx*dx + dy*dy + dz*dz;

     if( dist2 < dist1 ) {
          u = uguess;
          v = vguess;
     }

     uv[0] = u;
     uv[1] = v;
     return 0;
}


///////////////////////////////////////////////////////////////////////////////

int GFace::getUVCoords(const Point3D &xyz, const Point2D &nearto, Point2D &uv) const
{
     assert(kdtree);

     double tol = 1.0E-06;
     int err;
     double x, y, z, u, v, dx, dy, dz, derr;

     double xq = xyz[0];
     double yq = xyz[1];
     double zq = xyz[2];

     if (xq < xmin || xq > xmax)
          cout << "Warning: Query point outside X Range " << endl;

     if (yq < ymin || yq > ymax)
          cout << "Warning: Query point outside Y Range " << endl;

     if (zq < zmin || zq > zmax)
          cout << "Warning: Query point outside Z Range " << endl;

     u = nearto[0];
     v = nearto[1];
     geom->getEntXYZtoUVHint(gFaceHandle, xq, yq, zq, u, v);
     assert(!err);

     geom->getEntUVtoXYZ(gFaceHandle, u, v, x, y, z);
     assert(!err);

     dx = fabs(x - xq);
     dy = fabs(y - yq);
     dz = fabs(z - zq);
     derr = dx * dx + dy * dy + dz*dz;

     if (derr < tol * tol) {
          uv[0] = u;
          uv[1] = v;
          return 0;
     }

     assert(1);

     double queryPoint[3], eps = 0.0;
     double xon, yon, zon;
     double dist1, dist2;

     queryPoint[0] = xq;
     queryPoint[1] = yq;
     queryPoint[2] = zq;

     kdtree->annkSearch(queryPoint, numNeighs, &annIdx[0], &anndist[0], eps);

     int index = annIdx[0];
     dist1 = anndist[0];

     x = kdnodes[index][0];
     y = kdnodes[index][1];
     z = kdnodes[index][2];
     u = uvCoords[2 * index + 0];
     v = uvCoords[2 * index + 1];

     uv[0] = u;
     uv[1] = v;
     return 0;
}

///////////////////////////////////////////////////////////////////////////////

int GFace::getUVCoords(const Point2D &uvstart, double dist, Point2D &uvguess) const
{
     Point2D uv;
     Point3D p0, p1;
     getXYZCoords(uvstart, p0);
     getXYZCoords(uvguess, p1);

     double dx, dy, dz, dl;
     double u = uvguess[0];
     double v = uvguess[1];

     int ntrials = 0;
     while (1) {
          dx = p1[0] - p0[0];
          dy = p1[1] - p0[1];
          dz = p1[2] - p0[2];
          dl = sqrt(dx * dx + dy * dy + dz * dz);

          if (fabs(dl - dist) < 1.0E-15) break;

          u = uvstart[0] + (u - uvstart[0]) * dist / dl;
          v = uvstart[1] + (v - uvstart[1]) * dist / dl;
          uv[0] = u;
          uv[1] = v;

          getXYZCoords(uv, p1);

          if (ntrials++ == 100) {
               cout << " Warning: UV  Search failed" << endl;
               break;
          }
     }

     uvguess[0] = u;
     uvguess[1] = v;

     return 0;
}


///////////////////////////////////////////////////////////////////////////////

int GFace::getClosestPoint(const Point3D &qPoint, Point3D &p3d) const
{
     int err;
     double p[3], eps = 0.0;
     double xon, yon, zon;
     double xon1, yon1, zon1;
     double dx, dy, dz, dist1, dist2;

     dist1 = std::numeric_limits<double>::max();

     if (!kdtree) {
          xon = qPoint[0];
          yon = qPoint[1];
          zon = qPoint[2];
     }

     err = geom->getEntClosestPt(gFaceHandle, qPoint[0], qPoint[1], qPoint[2],
                                 xon, yon, zon);

     if (!err) {
          dx = qPoint[0] - xon;
          dy = qPoint[1] - yon;
          dz = qPoint[2] - zon;
          dist1 = dx * dx + dy * dy + dz*dz;
          p3d[0] = xon;
          p3d[1] = yon;
          p3d[2] = zon;
          return 0;
     }

     cout << " Searching from k-d tree " << endl;

     if (kdtree) {
          p[0] = qPoint[0];
          p[1] = qPoint[1];
          p[2] = qPoint[2];
          kdtree->annkSearch(p, numNeighs, &annIdx[0], &anndist[0], eps);
          dist2 = anndist[0];

          int index = annIdx[0];
          double u = uvCoords[2 * index + 0];
          double v = uvCoords[2 * index + 1];

          err= geom->getEntUVtoXYZ(gFaceHandle, u, v, xon1, yon1, zon1);
          dx = qPoint[0] - xon1;
          dy = qPoint[1] - yon1;
          dz = qPoint[2] - zon1;
          dist2 = dx * dx + dy * dy + dz*dz;

          if (dist2 < dist1) {
               //          cout << " With iGeom " << dist1 << " With KD Tree : " << dist2 << endl;
               xon = xon1;
               yon = yon1;
               zon = zon1;
          }
     }

     p3d[0] = xon;
     p3d[1] = yon;
     p3d[2] = zon;
     return 0;
}

///////////////////////////////////////////////////////////////////////////////

void GFace::projectEdgeHigherOrderNodes(const vector<double> &gnodes)
{
     int err;
     gllnodes = gnodes;

     if (mesh == 0) {
          cout << " Warning: No mesh is present in geometric face : Higher order nodes projection not done " << endl;
          return;
     }

     if (relPair == 0) {
          cout << " Warning: No assoc is present in geometric face : Higher order nodes projection not done " << endl;
          return;
     }

     if (rel == 0) {
          cout << " Warning: No relation is present in geometric face : Higher order nodes projection not done " << endl;
          return;
     }

     const char *tag1 = "HO_POINTS";
     err = mesh->getTagHandle(tag1, horder_tag);
     assert(!err);

     int nsize = gllnodes.size();
     arclength_ratio.resize(nsize);
     for (int i = 0; i < nsize; i++)
          arclength_ratio[i] = 0.5 * (1.0 + gllnodes[i]);

     iBase_EntitySetHandle meshSet;
     /*
         iRel_getEntSetAssociation(assoc, rel, gFaceHandle, 0, &meshSet, &err);
         assert(!err);
     */

     iBase_TagHandle dim_tag;
     const char *tag2 = "GEOM_DIMENSION";
     err = mesh->getTagHandle(tag2, dim_tag );
     assert(!err);

     int geom_dim;
     err = mesh->getEntSetIntData(meshSet, dim_tag, geom_dim);
     assert(!err);

     if (geom_dim == 2) {
          vector<iBase_EntityHandle> mEdges;
          err = mesh->getEntities(meshSet, iBase_EDGE, iMesh_ALL_TOPOLOGIES, mEdges);
          for (int i = 0; i < mEdges.size(); i++) projectEdgeHigherOrderNodes(mEdges[i]);
     }
}

///////////////////////////////////////////////////////////////////////////////

void GFace::projectEdgeHigherOrderNodes(iBase_EntityHandle mEdgeHandle)
{
     int err;
     vector<iBase_EntityHandle> edgeNodes;

     err = mesh->getEntAdj(mEdgeHandle, iBase_VERTEX, edgeNodes);
     //
     // end nodes parametric coordinates can be ambiguous in the case of periodic
     // surfaces, therefore, estimate UV near to the ends first.
     // In this case, at least one vertex of the edge will be on the surface.
     // i.e. both the nodes cann't be on the boundary curves. If that were the
     // case, ambiguities can occur.
     //

     Point3D p0, pN, pnear, pon;
     Point2D uv, uv0, uvN, uvnear;

     err = mesh->getVtxCoord(edgeNodes[0], p0[0], p0[1], p0[2]);
     err = mesh->getVtxCoord(edgeNodes[1], pN[0], pN[1], pN[2]);

     double u, dl, arclen, arcdist, unear = 0.98;

     pnear[0] = (1 - unear) * p0[0] + unear * pN[0];
     pnear[1] = (1 - unear) * p0[1] + unear * pN[1];
     pnear[2] = (1 - unear) * p0[2] + unear * pN[2];
     getUVCoords(pnear, uvnear);
     getUVCoords(p0, uvnear, uv0);

     pnear[0] = (1 - unear) * pN[0] + unear * p0[0];
     pnear[1] = (1 - unear) * pN[1] + unear * p0[1];
     pnear[2] = (1 - unear) * pN[2] + unear * p0[2];
     getUVCoords(pnear, uvnear);
     getUVCoords(pN, uvnear, uvN);

     char *tag_val = NULL;
     err = mesh->getData(mEdgeHandle, horder_tag, &tag_val);
     assert(!err);

     HO_Points *hopoints = (HO_Points *) tag_val;
     iBase_EntityHandle *nodesOnEdge = hopoints->nodeHandles;
     int numHPoints = hopoints->nx;
     int nhalf = numHPoints / 2;

     arclen = getGeodesicLength(uv0, uvN);

     // From the start->nhalf
     arcdist = 0.0;
     uvnear[0] = uv0[0];
     uvnear[1] = uv0[1];

     for (int k = 1; k < nhalf; k++) {
          iBase_EntityHandle currvertex = nodesOnEdge[k];
          dl = arclength_ratio[k] * arclen - arcdist;
          u = gllnodes[k];
          uv[0] = 0.5 * (1 - u) * uv0[0] + 0.5 * (1 + u) * uvN[0];
          uv[1] = 0.5 * (1 - u) * uv0[1] + 0.5 * (1 + u) * uvN[1];
//      getUVCoords(uvnear, dl, uv);
          getXYZCoords(uv, pon);
          err = mesh->setVtxCoord(currvertex, pon[0], pon[1], pon[2]);
          uvnear[0] = uv[0];
          uvnear[1] = uv[1];
          arcdist += dl;
     }

     // From the end->nhalf
     arcdist = 0.0;
     uvnear[0] = uvN[0];
     uvnear[1] = uvN[1];
     for (int k = 0; k < nhalf; k++) {
          iBase_EntityHandle currvertex = nodesOnEdge[numHPoints - 1 - k];
          dl = arclength_ratio[k] * arclen - arcdist;
          u = gllnodes[k];
          uv[0] = 0.5 * (1 - u) * uvN[0] + 0.5 * (1 + u) * uv0[0]; // careful, u0, uN swapped
          uv[1] = 0.5 * (1 - u) * uvN[1] + 0.5 * (1 + u) * uv0[1]; // careful, u0, uN swapped
//      getUVCoords(uvnear, dl, uv);
          getXYZCoords(uv, pon);
          mesh->setVtxCoord(currvertex, pon[0], pon[1], pon[2]);
          uvnear[0] = uv[0];
          uvnear[1] = uv[1];
          arcdist += dl;
     }

     if (numHPoints % 2) {
          int midpos = nhalf;
          iBase_EntityHandle currvertex = nodesOnEdge[midpos];
          u = gllnodes[midpos + 1];
          uv[0] = 0.5 * (1 - u) * uv0[0] + 0.5 * (1 + u) * uvN[0];
          uv[1] = 0.5 * (1 - u) * uv0[1] + 0.5 * (1 + u) * uvN[1];
          getXYZCoords(uv, pon);
          mesh->setVtxCoord(currvertex, pon[0], pon[1], pon[2]);
     }
}

////////////////////////////////////////////////////////////////////////////////

void GFace::projectFaceHigherOrderNodes(const vector<double> &gnodes)
{
     int err;
     gllnodes = gnodes;

     if (mesh == 0) {
          cout << " Warning: No mesh is present in geometric face : Higher order nodes projection not done " << endl;
          return;
     }

     if (relPair == 0) {
          cout << " Warning: No assoc is present in geometric face : Higher order nodes projection not done " << endl;
          return;
     }

     if (rel == 0) {
          cout << " Warning: No relation is present in geometric face : Higher order nodes projection not done " << endl;
          return;
     }

     const char *tag1 = "HO_POINTS";
     err = mesh->getTagHandle(tag1, horder_tag);
     assert(!err);

     int nsize = gllnodes.size();
     arclength_ratio.resize(nsize);
     for (int i = 0; i < nsize; i++)
          arclength_ratio[i] = 0.5 * (1.0 + gllnodes[i]);

     iBase_EntitySetHandle meshSet;
     err = relPair->getEntSetRelation(gFaceHandle, 0, meshSet);
     assert(!err);

     iBase_TagHandle dim_tag;
     const char *tag2 = "GEOM_DIMENSION";
     err = mesh->getTagHandle(tag2, dim_tag);
     assert(!err);

     int geom_dim;
     err = mesh->getEntSetIntData(meshSet, dim_tag, geom_dim);
     assert(!err);

     if (geom_dim == 2) {
          vector<iBase_EntityHandle> mFaces;
          err = mesh->getEntities(meshSet, iBase_FACE, iMesh_ALL_TOPOLOGIES, mFaces);
          for (int i = 0; i < mFaces.size(); i++) projectFaceHigherOrderNodes(mFaces[i]);
     }
}

////////////////////////////////////////////////////////////////////////////////

void GFace::projectFaceHigherOrderNodes(iBase_EntityHandle mFaceHandle)
{
     int err;

     int offset, nx, ny, numHPoints;

     vector<iBase_EntityHandle> faceNodes;
     err = mesh->getEntAdj(mFaceHandle, iBase_VERTEX, faceNodes);

     char *tag_val = NULL;
     int tag_val_allocated, tag_val_size;
     err = mesh->getData(mFaceHandle, horder_tag, &tag_val);
     assert(!err);

     HO_Points *hopoints = (HO_Points *) tag_val;
     nx = hopoints->nx;
     ny = hopoints->ny;
     numHPoints = nx*ny;

     iBase_EntityHandle *nodeHandles = hopoints->nodeHandles;

     vector<double> u(numHPoints);
     vector<double> v(numHPoints);

     iBase_EntityHandle currvertex;

     Point3D p3d, p0, p1, corners[4], pnear;
     Point2D uv, uvnear;
     double  t = 0.95;
     err = mesh->getVtxCoord(faceNodes[0], corners[0][0], corners[0][1], corners[0][2]);
     err = mesh->getVtxCoord(faceNodes[1], corners[1][0], corners[1][1], corners[1][2]);
     err = mesh->getVtxCoord(faceNodes[2], corners[2][0], corners[2][1], corners[2][2]);
     err = mesh->getVtxCoord(faceNodes[3], corners[3][0], corners[3][1], corners[3][2]);

     p0     = linear_interpolation01( corners[0], corners[1], t );
     p1     = linear_interpolation01( corners[0], corners[3], t );
     pnear  = linear_interpolation01( p0, p1, 0.50);
     getUVCoords(pnear, uvnear);

     offset = 0;
     currvertex = nodeHandles[offset];
     mesh->getVtxCoord(currvertex, p3d[0], p3d[1], p3d[2]);
     getUVCoords(p3d, uvnear, uv);
     u[offset] = uv[0];
     v[offset] = uv[1];

     p0     = linear_interpolation01( corners[1], corners[0], t );
     p1     = linear_interpolation01( corners[1], corners[2], t );
     pnear  = linear_interpolation01( p0, p1, 0.50);
     getUVCoords(pnear, uvnear);

     offset = nx - 1;
     currvertex = nodeHandles[offset];
     err = mesh->getVtxCoord(currvertex, p3d[0], p3d[1], p3d[2]);
     getUVCoords(p3d, uvnear, uv);
     u[offset] = uv[0];
     v[offset] = uv[1];

     p0     = linear_interpolation01( corners[3], corners[2], t );
     p1     = linear_interpolation01( corners[3], corners[0], t );
     pnear  = linear_interpolation01( p0, p1, 0.50);
     getUVCoords(pnear, uvnear);

     offset = (ny - 1) * nx;
     currvertex = nodeHandles[offset];
     mesh->getVtxCoord(currvertex, p3d[0], p3d[1], p3d[2]);
     getUVCoords(p3d, uvnear, uv);
     u[offset] = uv[0];
     v[offset] = uv[1];

     p0     = linear_interpolation01( corners[2], corners[1], t );
     p1     = linear_interpolation01( corners[2], corners[3], t );
     pnear  = linear_interpolation01( p0, p1, 0.50);
     getUVCoords(pnear, uvnear);

     offset = nx * ny - 1;
     currvertex = nodeHandles[offset];
     mesh->getVtxCoord(currvertex, p3d[0], p3d[1], p3d[2]);
     getUVCoords(p3d, uvnear, uv);
     u[offset] = uv[0];
     v[offset] = uv[1];

     uvnear[0] = u[0];
     uvnear[1] = v[0];
     for (int i = 1; i < nx - 1; i++) {
          offset = i;
          currvertex = nodeHandles[offset];
          mesh->getVtxCoord(currvertex, p3d[0], p3d[1], p3d[2]);
          getUVCoords(p3d, uvnear, uv);
          u[offset] = uv[0];
          v[offset] = uv[1];
          uvnear    = uv;
     }

     uvnear[0] = u[nx*(ny-1)];
     uvnear[1] = v[nx*(ny-1)];
     for (int i = 1; i < nx - 1; i++) {
          offset = i + (ny - 1) * nx;
          currvertex = nodeHandles[offset];
          mesh->getVtxCoord(currvertex, p3d[0], p3d[1], p3d[2]);
          getUVCoords(p3d, uvnear, uv);
          u[offset] = uv[0];
          v[offset] = uv[1];
          uvnear    = uv;
     }

     uvnear[0] = u[0];
     uvnear[1] = v[0];
     for (int j = 1; j < ny - 1; j++) {
          offset = j*nx;
          currvertex = nodeHandles[offset];
          mesh->getVtxCoord(currvertex, p3d[0], p3d[1], p3d[2] );
          getUVCoords(p3d, uvnear, uv);
          u[offset] = uv[0];
          v[offset] = uv[1];
     }

     uvnear[0] = u[(nx-1)];
     uvnear[1] = v[(nx-1)];
     for (int j = 1; j < ny - 1; j++) {
          offset = j * nx + (nx - 1);
          currvertex = nodeHandles[offset];
          mesh->getVtxCoord(currvertex, p3d[0], p3d[1], p3d[2] );
          getUVCoords(p3d, uvnear, uv);
          u[offset] = uv[0];
          v[offset] = uv[1];
     }

     TFIMap::blend_from_edges(u, gllnodes, gllnodes);
     TFIMap::blend_from_edges(v, gllnodes, gllnodes);

     Point3D pon;
     for (int j = 1; j < ny - 1; j++) {
          for (int i = 1; i < nx - 1; i++) {
               offset = j * nx + i;
               uv[0] = u[offset];
               uv[1] = v[offset];
               getXYZCoords(uv, pon);
               mesh->setVtxCoord(nodeHandles[offset], pon[0], pon[1], pon[2]);
          }
     }
}


///////////////////////////////////////////////////////////////////////////////

void GFace::saveAs(const string &filename) const
{
     ofstream ofile(filename.c_str(), ios::out);
     if (ofile.fail()) return;

     int nx = 51;
     int ny = 51;

     double du = (umax - umin) / (double) (nx - 1);
     double dv = (vmax - vmin) / (double) (ny - 1);

     double x, y, z;
     ofile << "#Nodes " << nx * ny << endl;
     int err, index = 0;
     for (int j = 0; j < ny; j++) {
          double v = vmin + j*dv;
          for (int i = 0; i < nx; i++) {
               double u = umin + i*du;
               err = geom->getEntUVtoXYZ(gFaceHandle, u, v, x, y, z);
               ofile << index++ << " " << x << " " << y << " " << z << endl;
          }
     }
}

