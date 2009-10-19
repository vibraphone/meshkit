#include "ExtrudeMesh.hpp"

#include "vec_utils.hpp"
#include <cassert>
#include <iostream>

static double * vtx_diff(double *res, iMesh_Instance mesh, iBase_EntityHandle a,
                         iBase_EntityHandle b)
{
  int err;
  double xa[3],xb[3];

  iMesh_getVtxCoord(mesh,a,xa+0,xa+1,xa+2,&err);
  ERROR("Couldn't get vertex coords");
  iMesh_getVtxCoord(mesh,b,xb+0,xb+1,xb+2,&err);
  ERROR("Couldn't get vertex coords");

  for(int i=0; i<3; i++)
    res[i] = xa[i]-xb[i];
  return res;
}


ExtrudeMesh::ExtrudeMesh(iMesh_Instance mesh)
  : impl_(mesh), copy_(mesh), updated_set_(false)
{}

ExtrudeMesh::~ExtrudeMesh()
{
  for(std::vector<tag_data>::iterator i=extrude_tags_.begin();
      i!=extrude_tags_.end(); ++i) {
    free(i->value);
  }
}

int ExtrudeMesh::add_extrude_tag(const std::string &tag_name,
                 const char *tag_val)
{
  iBase_TagHandle tag_handle = 0;
  int err;
  iMesh_getTagHandle(impl_, tag_name.c_str(), &tag_handle, &err,
                     tag_name.length());
  if(err != iBase_SUCCESS)
    ERRORR("Failed to get handle for tag "+tag_name, iBase_FAILURE);

  return add_extrude_tag(tag_handle,tag_val);
}

int ExtrudeMesh::add_extrude_tag(iBase_TagHandle tag_handle,
                                 const char *tag_val)
{
  int err = iBase_SUCCESS;
  char *tmp = NULL;

  if(tag_val) {
    int tag_size;
    iMesh_getTagSizeBytes(impl_, tag_handle, &tag_size, &err);
    if (err != iBase_SUCCESS)
      ERRORR("Failed to get size of tag", iBase_FAILURE);
    tmp = static_cast<char*>(malloc(tag_size));
    memcpy(tmp, tag_val, tag_size);
  }

  extrude_tags_.push_back(tag_data(tag_handle, tmp));

  return err;
}

int ExtrudeMesh::update_sets()
{
  if(updated_set_)
    reset_sets();
  copy_.update_ce_lists();

  int err;
  iBase_EntitySetHandle root;
  iMesh_getRootSet(impl_, &root, &err);
  ERRORR("Trouble getting root set", iBase_FAILURE);
  
  if(!extrude_tags_.empty())
  {
    iBase_EntitySetHandle *tmp_sets;
    int tmp_alloc, tmp_size;
    int err;
    for(std::vector<tag_data>::iterator i=extrude_tags_.begin();
        i!=extrude_tags_.end(); ++i) {
      tmp_sets = NULL;
      tmp_alloc = 0;
      iMesh_getEntSetsByTagsRec(impl_, root, &i->tag,
                                (i->value ? &i->value:NULL), 1, 0,
                                &tmp_sets, &tmp_alloc, &tmp_size, &err);
      ERRORR("Couldn't get tagged sets.", iBase_FAILURE);

      for(int j=0; j<tmp_size; j++) 
        extrude_sets_.insert(tmp_sets[j]);
      free(tmp_sets);
    }
  }

  updated_set_ = true;
  return iBase_SUCCESS;
}

int ExtrudeMesh::reset_sets()
{
  copy_.reset_ce_lists();
  extrude_sets_.clear();
  updated_set_ = false;

  return iBase_SUCCESS;
}

void ExtrudeMesh::process_sets(iBase_TagHandle local_tag, new_sets_t &new_sets)
{
  int err;

  new_sets.reserve(extrude_sets_.size());
  bool first = new_sets.empty();
  new_sets_t::iterator curr=new_sets.begin();

  for(std::set<iBase_EntitySetHandle>::iterator i=extrude_sets_.begin();
      i!=extrude_sets_.end(); ++i) {
    iBase_EntityHandle *ents = NULL;
    int ent_alloc=0,ent_size;

    iMesh_getEntities(impl_, *i, iBase_ALL_TYPES, iMesh_ALL_TOPOLOGIES, &ents,
                      &ent_alloc, &ent_size, &err);
    ERROR("Couldn't get entities.");

    std::vector<iBase_EntityHandle> tags(ent_size, iBase_EntityHandle(0));
    iBase_EntityHandle *tag_ptr = &tags[0];
    int tag_alloc=tags.size(),tag_size;
    iMesh_getEHArrData(impl_, ents, ent_size, local_tag, &tag_ptr, &tag_alloc,
                       &tag_size, &err);
    free(ents);
    ERROR("Couldn't get entity handle tags.");

    std::remove_if(tags.begin(), tags.end(), std::bind2nd(
                     std::equal_to<iBase_EntityHandle>(),
                     iBase_EntityHandle(0)
                     ));

    if(!tags.empty()) {
      iBase_EntitySetHandle set;
      if(first) {
        iMesh_createEntSet(impl_, false, &set, &err);
        new_sets.push_back(set);
      }
      else
        set = *curr;

      for(std::vector<iBase_EntityHandle>::iterator j=tags.begin();
          j!=tags.end(); ++j) {
        iMesh_addEntToSet(impl_, *j, set, &err);
        ERROR("Couldn't add entity to extrude set.");
      }
      ++curr;
    }
  }
}

int ExtrudeMesh::translate(iBase_EntityHandle *src, int size, int steps,
                           const double *dx, bool copy_faces)
{
  return extrude(src, size, steps, CopyMoveVerts(impl_, dx, steps), copy_faces);
}

int ExtrudeMesh::translate(iBase_EntitySetHandle src, int steps,
                           const double *dx, bool copy_faces)
{
  return extrude(src, steps, CopyMoveVerts(impl_, dx, steps) ,copy_faces);
}

int ExtrudeMesh::translate(iBase_EntityHandle *src, iBase_EntityHandle *dest,
                           int size, int steps)
{
  int err;
  iBase_EntitySetHandle src_set, dest_set;
  iMesh_createEntSet(impl_, false, &src_set, &err);
  ERRORR("Couldn't create source entity set.", err);
  iMesh_createEntSet(impl_, false, &dest_set, &err);
  ERRORR("Couldn't create target entity set.", err);
  
  iMesh_addEntArrToSet(impl_, src, size, src_set, &err);
  ERRORR("Couldn't add entities to source entity set.", err);
  iMesh_addEntArrToSet(impl_, dest, size, dest_set, &err);
  ERRORR("Couldn't add entities to target entity set.", err);

  int ret = translate(src_set, dest_set, steps);

  iMesh_destroyEntSet(impl_, src_set, &err);
  ERRORR("Couldn't destroy source entity set.", err);
  iMesh_destroyEntSet(impl_, dest_set, &err);
  ERRORR("Couldn't destroy target entity set.", err);

  return ret;
}

int ExtrudeMesh::translate(iBase_EntitySetHandle src, 
                           iBase_EntitySetHandle dest, int steps)
{
  int err;

  // Deduce the per-step displacement vector "dx"
  // Note: we assume that src and dest are the same shape, etc.
  double dx[3];
  double coords[2][3];

  iBase_EntitySetHandle ends[] = { src, dest };
  for(int i=0; i<2; i++) {
    iMesh_EntityIterator iter;
    iMesh_initEntIter(impl_, ends[i], iBase_FACE, iMesh_ALL_TOPOLOGIES, &iter,
                      &err);
    ERRORR("Couldn't create entity iterator.", err);

    iBase_EntityHandle face;
    int has_data;
    iMesh_getNextEntIter(impl_, iter, &face, &has_data, &err);
    ERRORR("Couldn't get next element of iterator.", err);
    assert(has_data); // TODO: don't use assert

    iMesh_endEntIter(impl_, iter, &err);
    ERRORR("Couldn't destroy entity iterator.", err);

    iBase_EntityHandle *verts=0;
    int verts_alloc=0,verts_size=0;
    iMesh_getEntAdj(impl_, face, iBase_VERTEX, &verts, &verts_alloc,
                    &verts_size, &err);
    ERRORR("Couldn't get adjacencies.", err);

    iMesh_getVtxCoord(impl_, verts[0], coords[i]+0, coords[i]+1, coords[i]+2,
                      &err);
    free(verts);
    ERRORR("Couldn't get vertex coordinates.", err);
  }

  for(int i=0; i<3; i++)
    dx[i] = (coords[1][i]-coords[0][i]) / steps;

  return extrude(src, dest, steps, CopyMoveVerts(impl_, dx));
}

int ExtrudeMesh::rotate(iBase_EntityHandle *src, int size, int steps,
                        const double *origin, const double *z, double angle,
                        bool copy_faces)
{
  return extrude(src, size, steps,
                 CopyRotateVerts(impl_, origin, z, angle, steps), copy_faces);
}

int ExtrudeMesh::rotate(iBase_EntitySetHandle src, int steps,
                        const double *origin, const double *z, double angle,
                        bool copy_faces)
{
  return extrude(src, steps, CopyRotateVerts(impl_, origin, z, angle),
                 copy_faces);
}

int ExtrudeMesh::rotate(iBase_EntityHandle *src, iBase_EntityHandle *dest,
                        int size, int steps, const double *origin,
                        const double *z, double angle)
{
  return extrude(src, dest, size, steps, CopyRotateVerts(impl_, origin, z,
                                                         angle));
}

int ExtrudeMesh::rotate(iBase_EntitySetHandle src, iBase_EntitySetHandle dest,
                        int steps, const double *origin, const double *z,
                        double angle)
{
  return extrude(src, dest, steps, CopyRotateVerts(impl_, origin, z, angle));
}

int ExtrudeMesh::extrude(iBase_EntityHandle *src, int size, int steps,
                         const CopyVerts &trans, bool copy_faces)
{
  int err;
  iBase_EntitySetHandle set;
  iMesh_createEntSet(impl_, false, &set, &err);
  ERRORR("Couldn't create source entity set.", err);
  
  iMesh_addEntArrToSet(impl_, src, size, set, &err);
  ERRORR("Couldn't add entities to source entity set.", err);

  int ret = extrude(set, steps, trans, copy_faces);

  iMesh_destroyEntSet(impl_, set, &err);
  ERRORR("Couldn't destroy source entity set.", err);

  return ret;
}

int ExtrudeMesh::extrude(iBase_EntitySetHandle src, int steps,
                         const CopyVerts &trans, bool copy_faces)
{
  if(copy_faces) {
    int err;

    iBase_EntitySetHandle parent;
    iMesh_createEntSet(impl_, false, &parent, &err);
    ERRORR("Couldn't create parent entity set.", err);
    iMesh_addEntSet(impl_, src, parent, &err);
    ERRORR("Couldn't add source entity set to parent.", err);
    
    copy_.add_copy_expand_list(&src,    1, CopyMesh::COPY);
    copy_.add_copy_expand_list(&parent, 1, CopyMesh::EXPAND);
    copy_.copy_transform_entities(src, trans, 0, 0, 0);

    iMesh_rmvEntSet(impl_, src, parent, &err);
    ERRORR("Couldn't remove source entity set from parent.", err);

    iBase_EntitySetHandle dest;
    iBase_EntitySetHandle *tmp = &dest;
    int tmp_alloc=1,tmp_size=0;
    iMesh_getEntSets(impl_ ,parent, 0, &tmp, &tmp_alloc, &tmp_size, &err);
    ERRORR("Couldn't get entity sets in parent.", err);

    int ret = do_extrusion(src, dest, true, steps-1, trans);

    iMesh_destroyEntSet(impl_, parent, &err);
    ERRORR("Couldn't destroy parent entity set.", err);
    iMesh_destroyEntSet(impl_, dest, &err);
    ERRORR("Couldn't destroy target entity set.", err);

    return ret;
  }
  else
    return do_extrusion(src, 0, false, steps, trans);
}

int ExtrudeMesh::extrude(iBase_EntityHandle *src, iBase_EntityHandle *dest,
                         int size, int steps, const CopyVerts &trans)
{
  int err;
  iBase_EntitySetHandle src_set, dest_set;
  iMesh_createEntSet(impl_, false, &src_set, &err);
  ERRORR("Couldn't create source entity set.", err);
  iMesh_createEntSet(impl_, false, &dest_set, &err);
  ERRORR("Couldn't create target entity set.", err);
  
  iMesh_addEntArrToSet(impl_, src, size, src_set, &err);
  ERRORR("Couldn't add entities to source entity set.", err);
  iMesh_addEntArrToSet(impl_, dest, size, dest_set, &err);
  ERRORR("Couldn't add entities to target entity set.", err);

  int ret = extrude(src_set, dest_set, steps, trans);

  iMesh_destroyEntSet(impl_, src_set, &err);
  ERRORR("Couldn't destroy source entity set.", err);
  iMesh_destroyEntSet(impl_, dest_set, &err);
  ERRORR("Couldn't destroy target entity set.", err);

  return ret;
}

int ExtrudeMesh::extrude(iBase_EntitySetHandle src, iBase_EntitySetHandle dest,
                         int new_rows, const CopyVerts &trans)
{
  return do_extrusion(src, dest, true, new_rows-1, trans);
}

int ExtrudeMesh::do_extrusion(iBase_EntitySetHandle src,
                              iBase_EntitySetHandle dest, bool use_dest,
                              int new_rows, const CopyVerts &trans)
{
  assert(new_rows > 0 || use_dest);

  if(!updated_set_)
    update_sets();

  int err;

  iBase_EntityHandle *ents=0; int ent_alloc=0, ent_size=0;
  iBase_EntityHandle *adj=0;  int adj_alloc=0, adj_size=0;
  int *indices=0;       int ind_alloc=0, ind_size=0;
  int *offsets=0;       int off_alloc=0, off_size=0;

  iMesh_getAdjEntIndices(impl_,src, iBase_ALL_TYPES, iMesh_ALL_TOPOLOGIES,
                         iBase_VERTEX,
                         &ents,    &ent_alloc, &ent_size,
                         &adj,     &adj_alloc, &adj_size,
                         &indices, &ind_alloc, &ind_size,
                         &offsets, &off_alloc, &off_size,
                         &err);
  ERRORR("Trouble getting source adjacencies.", err);

  double dx[3];
  iBase_EntityHandle *curr = 0;
  iBase_EntityHandle *next = adj;
  int *normals = 0;

  iBase_TagHandle local_tag;
  const char *tag_name = "local_copy";
  iMesh_createTag(impl_, tag_name, 1, iBase_ENTITY_HANDLE, &local_tag, &err,
                  strlen(tag_name));
  ERRORR("Couldn't create local copy tag.", err);
  new_sets_t new_sets;

  if(new_rows > 0) {
    int row_alloc = adj_size, row_size;
    curr = new iBase_EntityHandle[row_alloc];
    next = new iBase_EntityHandle[row_alloc];
    trans(1, adj, adj_size, &next, &row_alloc, &row_size);

    vtx_diff(dx, impl_, next[0], adj[0]);
    normals = get_normals(adj, indices, offsets, ent_size, dx);

    // Make the first set of volumes
    connect_the_dots(ents, ent_size, local_tag, new_sets,
                     normals, indices, offsets, adj,
                     normals, indices, offsets, next);

    // Make the inner volumes
    for(int i=2; i<=new_rows; i++) {
      std::swap(curr, next);
      trans(i, adj, adj_size, &next, &row_alloc, &row_size);
      connect_the_dots(ents, ent_size, local_tag, new_sets,
                       normals, indices, offsets, curr,
                       normals, indices, offsets, next);
    }
  }

  if(use_dest) {
    iBase_EntityHandle *ents2=0; int ent2_alloc=0, ent2_size=0;
    iBase_EntityHandle *adj2=0;  int adj2_alloc=0, adj2_size=0;
    int *indices2=0;             int ind2_alloc=0, ind2_size=0;
    int *offsets2=0;             int off2_alloc=0, off2_size=0;

    iMesh_getAdjEntIndices(impl_, dest, iBase_FACE, iMesh_ALL_TOPOLOGIES,
                           iBase_VERTEX,
                           &ents2,    &ent2_alloc, &ent2_size,
                           &adj2,     &adj2_alloc, &adj2_size,
                           &indices2, &ind2_alloc, &ind2_size,
                           &offsets2, &off2_alloc, &off2_size,
                           &err);
    ERRORR("Trouble getting target adjacencies.", err);

    vtx_diff(dx, impl_, adj2[indices2[ offsets2[0] ]],
                        next[indices [ offsets [0] ]]);
    if(!normals)
      normals = get_normals(adj, indices, offsets, ent_size, dx);
    int *normals2 = get_normals(adj2, indices2, offsets2, ent2_size, dx);

    connect_the_dots(ents, ent_size, local_tag, new_sets,
                     normals,  indices,  offsets,  next,
                     normals2, indices2, offsets2, adj2);

    free(normals2);
    free(ents2);
    free(adj2);
    free(indices2);
    free(offsets2);
  }

  iMesh_destroyTag(impl_, local_tag, true, &err);
  ERRORR("Couldn't destroy local tag.", err);

  if(new_rows > 0) {
    delete curr;
    delete next;
  }

  free(normals);
  free(ents);
  free(adj);
  free(indices);
  free(offsets);

  return 0;
}

// calculate the normals for each face (1 = towards v, -1 = away from v)
// TODO: this can fail with non-convex faces
int * ExtrudeMesh::get_normals(iBase_EntityHandle *verts, int *indices,
                               int *offsets, int size, double *dv)
{
  int err;
  int *normals = (int*)malloc(size*sizeof(int));

  for(int i=0; i<size; i++) {
    double res[3], a[3], b[3];
    double *coords=0;
    int coord_alloc=0, coord_size=0;

    iBase_EntityHandle curr_verts[3];
    if(offsets[i+1] - offsets[i] > 2) { // face
      for(int j=0; j<3; j++)
        curr_verts[j] = verts[indices[ offsets[i]+j ]];

      iMesh_getVtxArrCoords(impl_, curr_verts, 3, iBase_INTERLEAVED,
                            &coords, &coord_alloc, &coord_size, &err);
      ERROR("Couldn't get vertex coordinates.");

      for(int j=0; j<3; j++) {
        a[j] = coords[1*3 + j] - coords[0*3 + j];
        b[j] = coords[2*3 + j] - coords[1*3 + j];
      }

      normals[i] = (dot( cross(res,a,b),dv ) > 0) ? 1:-1;
    }
    else if(offsets[i+1] - offsets[i] == 2) { // line
      normals[i] = 1; // TODO: figure out a way of distinguishing swapped
                      // lines
    }
    else // vertex
      normals[i] = 1;
  }

  return normals;
}

void ExtrudeMesh::connect_the_dots(
  iBase_EntityHandle *src, int size, iBase_TagHandle local_tag,
  new_sets_t &sets,
  int *pre_norms,  int *pre_inds,  int *pre_offs,  iBase_EntityHandle *pre,
  int *post_norms, int *post_inds, int *post_offs, iBase_EntityHandle *post)
{
  int err;
  using namespace std;

  for(int i=0; i<size; i++) {
    int count = pre_offs[i+1] - pre_offs[i];

    // If the normal is facing in the wrong direction (away from the
    // translation) we add the vertices in reverse order. Otherwise, we go
    // in the usual order. If count is 2, then we are creating quads and so
    // need to swap the order of the post set of verts.
    
    int dx = pre_norms [i];
    int dy = post_norms[i] * (count == 2 ? -1:1);
    int x  = (dx == 1) ? pre_offs [i] : pre_offs [i+1]-1;
    int y  = (dy == 1) ? post_offs[i] : post_offs[i+1]-1;

    iBase_EntityHandle *nodes = new iBase_EntityHandle[count*2];
    for(int j=0; j<count; j++) {
      nodes[j]     = pre [ pre_inds [x + dx*j] ];
      nodes[j+count] = post[ post_inds[y + dy*j] ];
    }

    int status;
    iBase_EntityHandle out;

    if(count == 4)    // quad
      iMesh_createEnt(impl_, iMesh_HEXAHEDRON, nodes, 8,&out, &status, &err);
    else if(count == 3) // tri
      iMesh_createEnt(impl_, iMesh_PRISM, nodes, 6, &out, &status, &err);
    else if(count == 2) // line
      iMesh_createEnt(impl_, iMesh_QUADRILATERAL, nodes, 4, &out, &status,
                      &err);
    else if(count == 1) // vertex
      iMesh_createEnt(impl_, iMesh_LINE_SEGMENT, nodes, 2, &out, &status, &err);
    else
      std::cerr << "Couldn't extrude face; unusual shape." << std::endl;

    ERROR("Couldn't create extruded face.");

    iMesh_setEHData(impl_, src[i], local_tag, out, &err);
    ERROR("Couldn't set local tag data.");
    delete[] nodes;
  }

  process_sets(local_tag, sets);
}

#ifdef TEST
#include <cmath>

int test1()
{
  int err;
  iMesh_Instance mesh;
  iMesh_newMesh("", &mesh, &err, 0);
  ERRORR("Couldn't create mesh.", 1);

  iBase_EntitySetHandle root_set;
  iMesh_getRootSet(mesh, &root_set, &err);
  ERRORR("Couldn't get root set.", 1);

  ExtrudeMesh *ext = new ExtrudeMesh(mesh);

  double verts[] = {
    0, 0, 0,
    1, 0, 0,
    1, 1, 0,
    0, 1, 0,
    2, 1, 0,
    9, 9, 9,
  };
  iBase_EntityHandle *ents=0;
  int size=0, alloc=0;

  iMesh_createVtxArr(mesh, 6, iBase_INTERLEAVED, verts, 3*6, &ents, &size,
                     &alloc, &err);
  ERRORR("Couldn't create vertex array", 1);

  iBase_EntityHandle quad;
  int status;
  iMesh_createEnt(mesh, iMesh_QUADRILATERAL, ents, 4, &quad, &status, &err);
  ERRORR("Couldn't create entity", 1);

  iBase_EntityHandle line;
  iMesh_createEnt(mesh, iMesh_LINE_SEGMENT, ents, 2, &line, &status, &err);
  ERRORR("Couldn't create entity", 1);

  iBase_EntityHandle tri;
  iBase_EntityHandle tri_ents[] = { ents[1], ents[2], ents[4] };
  iMesh_createEnt(mesh, iMesh_TRIANGLE, tri_ents, 3, &tri, &status, &err);
  ERRORR("Couldn't create entity",1);

  iBase_EntityHandle faces[] = {quad, tri, line};
  double v[] = { 0, 0, 1 };
  int steps = 5;
  err = ext->translate(faces, 3, steps, v);
  ERRORR("Couldn't extrude mesh", 1);

  int count;
  iMesh_getNumOfType(mesh, 0, iBase_VERTEX, &count, &err);
  ERRORR("Couldn't get number of vertices.", 1);
  if(count != 5*(steps+1)+1)
    return 1;

  iMesh_getNumOfType(mesh, 0, iBase_FACE, &count, &err);
  ERRORR("Couldn't get number of faces.", 1);
  if(count != 2+1*steps)
    return 1;

  iMesh_getNumOfType(mesh, 0, iBase_REGION, &count, &err);
  ERRORR("Couldn't get number of regions.", 1);
  if(count != steps*2)
    return 1;

#ifdef TESTSAVE
  // VisIt doesn't like 1-d objects in pseudocolor volume graphs
  iMesh_deleteEnt(mesh, line, &err);
  assert(err == 0);

  const char *file = "test1.vtk";
  iMesh_save(mesh, root_set, file, "", &err, strlen(file), 0);
  assert(err == 0);
#endif

  delete ext;
  iMesh_dtor(mesh, &err);
  ERRORR("Couldn't destroy mesh.", 1);

  return 0;
}

int test2()
{
  int err;
  iMesh_Instance mesh;
  iMesh_newMesh("", &mesh, &err, 0);
  ERRORR("Couldn't create mesh.", 1);

  iBase_EntitySetHandle root_set;
  iMesh_getRootSet(mesh, &root_set, &err);
  ERRORR("Couldn't get root set.", 1);

  ExtrudeMesh *ext = new ExtrudeMesh(mesh);

  double verts[] = {
    0, 0, 0,
    1, 0, 0,
    1, 1, 0,
    0, 1, 0,
    2, 1, 0,
    9, 9, 9,

    0, 0, 1,
    1, 0, 1,
    1, 1, 1,
    0, 1, 1,
    2, 1, 1,
  };
  iBase_EntityHandle *ents=0;
  int size=0, alloc=0;

  iMesh_createVtxArr(mesh, 11, iBase_INTERLEAVED, verts, 3*11, &ents, &size,
                     &alloc, &err);
  ERRORR("Couldn't create vertex array", 1);

  iBase_EntityHandle quad[2];
  int status;
  iMesh_createEnt(mesh, iMesh_QUADRILATERAL, ents+0, 4, quad+0, &status, &err);
  ERRORR("Couldn't create entity", 1);

  iMesh_createEnt(mesh, iMesh_QUADRILATERAL, ents+6, 4, quad+1, &status, &err);
  ERRORR("Couldn't create entity", 1);

  iBase_EntityHandle tri[2];
  iBase_EntityHandle tri_ents[] = { ents[1], ents[2], ents[4],
                                    ents[7], ents[8], ents[10] };
  iMesh_createEnt(mesh, iMesh_TRIANGLE, tri_ents+0, 3, tri+0, &status, &err);
  ERRORR("Couldn't create entity",1);

  iMesh_createEnt(mesh, iMesh_TRIANGLE, tri_ents+3, 3, tri+1, &status, &err);
  ERRORR("Couldn't create entity",1);

  iBase_EntityHandle pre[]  = {quad[0], tri[0]};
  iBase_EntityHandle post[] = {quad[1], tri[1]};
  double v[] = { 0, 0, 1 };
  int steps = 5;
  err = ext->translate(pre, post, 2, steps);
  ERRORR("Couldn't extrude mesh", 1);

  int count;
  iMesh_getNumOfType(mesh, 0, iBase_VERTEX, &count, &err);
  ERRORR("Couldn't get number of vertices.", 1);
  if(count != 5*(steps+1)+1)
    return 1;

  iMesh_getNumOfType(mesh, 0, iBase_FACE, &count, &err);
  ERRORR("Couldn't get number of faces.", 1);
  if(count != 4)
    return 1;

  iMesh_getNumOfType(mesh, 0, iBase_REGION, &count, &err);
  ERRORR("Couldn't get number of regions.", 1);
  if(count != steps*2)
    return 1;

#ifdef TESTSAVE
  const char *file = "test2.vtk";
  iMesh_save(mesh, root_set, file, "", &err, strlen(file), 0);
  assert(err == 0);
#endif

  delete ext;
  iMesh_dtor(mesh, &err);
  ERRORR("Couldn't destroy mesh.", 1);

  return 0;
}

int test3()
{
  int err;
  iMesh_Instance mesh;
  iMesh_newMesh("", &mesh, &err, 0);
  ERRORR("Couldn't create mesh.", 1);

  iBase_EntitySetHandle root_set;
  iMesh_getRootSet(mesh, &root_set, &err);
  ERRORR("Couldn't get root set.", 1);

  ExtrudeMesh *ext = new ExtrudeMesh(mesh);

  double verts[] = {
    0, 0, 0,
    1, 0, 0,
    1, 1, 0,
    0, 1, 0,
  };
  iBase_EntityHandle *ents=0;
  int size=0, alloc=0;

  iMesh_createVtxArr(mesh, 4, iBase_INTERLEAVED, verts, 3*4, &ents, &size,
                     &alloc, &err);
  ERRORR("Couldn't create vertex array",1);

  iBase_EntityHandle quad;
  int status;
  iMesh_createEnt(mesh, iMesh_QUADRILATERAL, ents, 4, &quad, &status, &err);
  ERRORR("Couldn't create entity", 1);

  iBase_EntityHandle faces[] = { quad };
  double v[] = { 0, 0, 1 };
  int steps = 5;
  err = ext->translate(faces, 1, steps, v);
  ERRORR("Couldn't extrude mesh", 1);

  int count;
  iMesh_getNumOfType(mesh, 0, iBase_VERTEX, &count, &err);
  ERRORR("Couldn't get number of vertices.", 1);
  if(count != 4*(steps+1))
    return 1;

  iMesh_getNumOfType(mesh, 0, iBase_FACE, &count, &err);
  ERRORR("Couldn't get number of faces.", 1);
  if(count != 1)
    return 1;

  iMesh_getNumOfType(mesh, 0, iBase_REGION, &count, &err);
  ERRORR("Couldn't get number of regions.", 1);
  if(count != steps)
    return 1;

#ifdef TESTSAVE
  const char *file = "test3.vtk";
  iMesh_save(mesh, root_set, file, "", &err, strlen(file), 0);
  assert(err == 0);
#endif

  delete ext;
  iMesh_dtor(mesh, &err);
  ERRORR("Couldn't destroy mesh.", 1);

  return 0;
}

int test4()
{
  int err;
  iMesh_Instance mesh;
  iMesh_newMesh("", &mesh, &err, 0);
  ERRORR("Couldn't create mesh.", 1);

  iBase_EntitySetHandle root_set;
  iMesh_getRootSet(mesh, &root_set, &err);
  ERRORR("Couldn't get root set.", 1);

  ExtrudeMesh *ext = new ExtrudeMesh(mesh);

  double verts[] = {
    0, 0, 0,
    0, 1, 0,
    0, 1, 1,
    0, 0, 1,
  };
  iBase_EntityHandle *ents=0;
  int size=0, alloc=0;

  iMesh_createVtxArr(mesh, 4, iBase_INTERLEAVED, verts, 3*4, &ents, &size,
                     &alloc, &err);
  ERRORR("Couldn't create vertex array", 1);

  iBase_EntityHandle quad;
  int status;
  iMesh_createEnt(mesh, iMesh_QUADRILATERAL, ents, 4, &quad, &status, &err);
  ERRORR("Couldn't create entity", 1);

  iBase_EntityHandle faces[] = { quad };

  iBase_EntitySetHandle set;
  iMesh_createEntSet(mesh, false, &set, &err);
  ERRORR("Couldn't create entity set", 1);
  
  iMesh_addEntArrToSet(mesh, faces, 1, set, &err);
  ERRORR("Couldn't add entities to set", 1);

  iBase_TagHandle tag;
  iMesh_createTag(mesh, "my_tag", 1, iBase_BYTES, &tag, &err, 6);
  iMesh_setEntSetData(mesh, set, tag, "x", 1, &err);
  ext->add_extrude_tag(tag, "x");

  int steps = 200;
  double origin[] = { 0, -3, 0 };
  double z[] = { 1, 1, 1 };
  double angle = 2*3.14159/steps;
  err = ext->rotate(set, steps, origin, z, angle);
  ERRORR("Couldn't extrude mesh", 1);

  int count;
  iMesh_getNumOfType(mesh, 0, iBase_VERTEX, &count, &err);
  ERRORR("Couldn't get number of vertices.", 1);
  if(count != 4*(steps+1))
    return 1;

  iMesh_getNumOfType(mesh, 0, iBase_FACE, &count, &err);
  ERRORR("Couldn't get number of faces.", 1);
  if(count != 1)
    return 1;

  iMesh_getNumOfType(mesh, 0, iBase_REGION, &count, &err);
  ERRORR("Couldn't get number of regions.", 1);
  if(count != steps)
    return 1;

  iMesh_getNumEntSets(mesh, root_set, 0, &count, &err);
  ERRORR("Couldn't get number of entity sets.", 1);
  if(count != 2)
    return 1;

#ifdef TESTSAVE
  const char *file = "test4.vtk";
  iMesh_save(mesh, root_set, file, "", &err, strlen(file), 0);
  assert(err == 0);
#endif

  delete ext;
  iMesh_dtor(mesh, &err);
  ERRORR("Couldn't destroy mesh.", 1);

  return 0;
}

int main()
{
  if(test1())
    return 1;
  if(test2())
    return 1;
  if(test3())
    return 1;
  if(test4())
    return 1;

  return 0;
}

#endif