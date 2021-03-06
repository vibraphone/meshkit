#include "CopyMesh.hpp"

#include "CopyUtils.hpp"
#include "LocalSet.hpp"
#include "SimpleArray.hpp"

#include <algorithm>
#include <cstdlib>
#include <cstring>
#include <functional>
#include <string>

CopyMesh::CopyMesh(iMesh_Instance impl) 
  : imeshImpl(impl), copyTag(impl, "__CopyMeshTag"),
    copySets(impl), expandSets(impl)
{}

CopyMesh::~CopyMesh()
{}

void CopyMesh::copy(iBase_EntityHandle *ent_handles,
                    int num_ents,
                    const copy::Transform &trans,
                    iBase_EntityHandle **new_ents,
                    int *new_ents_allocated,
                    int *new_ents_size,
                    bool do_merge)
{
  int err;

  LocalSet set(imeshImpl);
  
  iMesh_addEntArrToSet(imeshImpl, ent_handles, num_ents, set, &err);
  check_error(imeshImpl, err);

  copy(set, trans, new_ents, new_ents_allocated, new_ents_size, do_merge);
}

void CopyMesh::copy(iBase_EntitySetHandle set_handle,
                    const copy::Transform &trans,
                    iBase_EntityHandle **new_ents,
                    int *new_ents_allocated,
                    int *new_ents_size,
                    bool do_merge)
{
  int err;
  LocalTag local_tag(imeshImpl);

  SimpleArray<iBase_EntityHandle> ents;
  SimpleArray<iBase_EntityHandle> verts;
  SimpleArray<int> indices;
  SimpleArray<int> offsets;
  
  iMesh_getStructure(imeshImpl, set_handle, ARRAY_INOUT(ents),
                     ARRAY_INOUT(verts), ARRAY_INOUT(indices),
                     ARRAY_INOUT(offsets), &err);
  check_error(imeshImpl, err);

  // copy the vertices
  SimpleArray<iBase_EntityHandle> new_verts;
  trans(imeshImpl, ARRAY_IN(verts), ARRAY_INOUT(new_verts));
  assert(new_verts.size() == verts.size());

  // set the local copy tags on vertices
  // XXX: Should this really happen? Doing so adds more entities to copy sets
  // than explicitly passed into this function. This may be a domain-specific
  // question.
  iMesh_setEHArrData(imeshImpl, ARRAY_IN(verts), local_tag,
                     ARRAY_IN(new_verts), &err);
  check_error(imeshImpl, err);

  // now connect the new vertices to make the higher-dimension entities
  connect_the_dots(imeshImpl, ARRAY_IN(ents), local_tag, &indices[0],
                   &offsets[0], &new_verts[0]);

  // take care of copy/expand sets
  update_sets();

  link_expand_sets(expandSets, local_tag);

  process_ce_sets(imeshImpl, copySets.sets(), local_tag);
  process_ce_sets(imeshImpl, expandSets.sets(), local_tag);

  tag_copy_sets(copySets, local_tag, copyTag);

  // get all the copies
  if (new_ents) {
    iMesh_getEHArrData(imeshImpl, ARRAY_IN(ents), local_tag, 
                       new_ents, new_ents_allocated, new_ents_size, &err);
    check_error(imeshImpl, err);
  }
}

void CopyMesh::update_sets()
{
  copySets.update_tagged_sets();
  expandSets.update_tagged_sets();
}

void CopyMesh::tag_copied_sets(const char **tag_names, const char **tag_vals,
                               const int num_tags) 
{
  int err;
  
  for (int t = 0; t < num_tags; t++) {
    iBase_TagHandle tag;
    iMesh_getTagHandle(imeshImpl, tag_names[t], &tag, &err,
                       strlen(tag_names[t]));
    check_error(imeshImpl, err);

    tag_copy_sets(imeshImpl, copyTag, copySets.sets(), tag,
                  tag_vals ? tag_vals[t] : NULL);
  }
}

void CopyMesh::tag_copied_sets(iBase_TagHandle *tags, const char **tag_vals,
                              const int num_tags) 
{
  for (int t = 0; t < num_tags; t++)
    tag_copy_sets(imeshImpl, copyTag, copySets.sets(), tags[t],
                  tag_vals ? tag_vals[t] : NULL);
}
