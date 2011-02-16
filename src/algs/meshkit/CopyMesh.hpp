// Author : Rajeev Jain
// Feb 01, 2011
// CopyMesh is an algorithm is used to copy move mesh. 
// It takes care of the meta-data propagation on the final mesh.

#ifndef COPYMESH_HPP
#define COPYMESH_HPP

#include <vector>
#include <map>
#include <sys/resource.h>

#include "meshkit/Types.hpp"
#include "meshkit/Error.hpp"
#include "meshkit/MeshScheme.hpp"
#include "meshkit/ModelEnt.hpp"

#include "meshkit/CESets.hpp"
#include "meshkit/LocalTag.hpp"
#include "meshkit/Transform.hpp"

#include "meshkit/iMesh.hpp"
#include "iMesh_extensions.h"


namespace MeshKit {

  class MKCore;
    

  /** \class CopyMesh CopyMesh.hpp "meshkit/CopyMesh.hpp"
   * \brief A simple class for meshing copy moving meshes
   *
   * INPUT: ModelEnts representing meshes
   * MESH TYPE(S): ALL TYPES
   * OUTPUT: Copied mesh along with the existing mesh
   * DEPENDENCIES: (none)
   * CONSTRAINTS: (none)
   *
   * This class performs the trivial task of copy moving meshes.  There can be 
   * multiple instances of this class, and therefore it is pointed to and managed by ....
   *
   * Each instance of this class stores all the ModelEnt's representing the mesh data,
   * and after execution after meshing new entities are created and tag propagation happens.
   */
  class CopyMesh : public MeshScheme
  {
  public:
    //! Bare constructor
    CopyMesh(MKCore *mkcore, const MEntVector &me_vec);
  
    //! Destructor
    virtual ~CopyMesh();
  
    /**\brief Get class name */
    static const char* name() 
      { return "CopyMesh"; }
      
    /**\brief Function returning whether this scheme can mesh entities of t
     *        the specified dimension.
     *\param dim entity dimension
     */
    static bool can_mesh(iBase_EntityType dim)
      { return iBase_REGION == dim || iBase_FACE == dim; }
  
    /** \brief Function returning whether this scheme can mesh the specified entity
     * 
     * Used by MeshOpFactory to find scheme for an entity.
     * \param me ModelEnt being queried
     * \return If true, this scheme can mesh the specified ModelEnt
     */
    static bool can_mesh(ModelEnt *me);
    
    /**\brief Get list of mesh entity types that can be generated.
     *\return array terminated with \c moab::MBMAXTYPE
     */
    static const moab::EntityType* output_types();
  
    /** \brief Return the mesh entity types operated on by this scheme
     * \return array terminated with \c moab::MBMAXTYPE
     */
    virtual const moab::EntityType* mesh_types_arr() const
      { return output_types(); }
  
    /** \brief Re-implemented here so we can check topological dimension of model_ent
     * \param model_ent ModelEnt being added
     */
    virtual bool add_modelent(ModelEnt *model_ent);

    //! Setup is a no-op, but must be provided since it's pure virtual
    virtual void setup_this();

    //! The only setup/execute function we need, since meshing vertices is trivial
    virtual void execute_this();

    // CopyMesh function local //////////////////
    /* \brief Return the copyTag used to indicate set copies
     */
    iBase_TagHandle copy_tag() {return copyTag;}

    void set_transform(const Copy::AnyTransform &transform);

    void update_sets();

    /* \brief Reset the copy and expand set lists
     */
    void reset_sets();

    /* \brief Add tag which should have unique values
     */
    void add_unique_tag(const std::string &tag_name);

    /* \brief Add tag which should have unique values
     */
    void add_unique_tag(iMesh::TagHandle tag_handle);

    /* \brief Return reference to copy sets
     */
    CESets &copy_sets();

    /* \brief Return reference to expand sets
     */
    CESets &expand_sets();

    /* \brief Return reference to unique sets
     */
    std::set<iMesh::EntitySetHandle> &unique_sets();
  
    /* \brief Tag copied sets with indicated tag from original set
     */
    void tag_copied_sets(const char **tag_names, const char **tag_vals,
                         const int num_tags);

    /* \brief Tag copied sets with indicated tag from original set
     */
    void tag_copied_sets(iMesh::TagHandle *tags, const char **tag_vals,
                         const int num_tags);
  private:
    void do_copy(iMesh::EntitySetHandle set_handle,
                 iMesh::EntityHandle **new_ents = 0,
                 int *new_ents_allocated = 0,
                 int *new_ents_size = 0,
                 bool do_merge = true);

    //- get the copy/expand sets based on copy/expand tags
    void get_copy_expand_sets(iMesh::EntitySetHandle *&copy_sets,
                              int &num_copy_sets,
                              iMesh::EntitySetHandle *&expand_sets,
                              int &num_expand_sets);

    //- get the sets tagged with the given vector of tags/values
    void get_tagged_sets(iMesh::EntitySetHandle from_set,
                         iMesh::TagHandle *tag_handles,
                         const char **tag_vals,
                         int num_tags,
                         iMesh::EntitySetHandle *&tagged_sets,
                         int &num_tagged_sets);

    iMesh mesh;                   // mesh instance
    LocalTag copyTag;             // tag storing copy-to tag
    Copy::AnyTransform transform; // transform function for copy-move

    CESets copySets;
    CESets expandSets;

    std::vector<iMesh::TagHandle> uniqueTags;
    std::set<iMesh::EntitySetHandle> uniqueSets;
  };

  inline void CopyMesh::set_transform(const Copy::AnyTransform &trans)
  {
    transform = trans;
  }

  inline void CopyMesh::add_unique_tag(const std::string &tag_name)
  {
    iMesh::TagHandle tag_handle;
    IBERRCHK(mesh.getTagHandle(tag_name.c_str(), tag_handle), "");
    add_unique_tag(tag_handle);
  }

  inline void CopyMesh::add_unique_tag(iMesh::TagHandle tag_handle)
  {
    assert(tag_handle != NULL);
    uniqueTags.push_back(tag_handle);
  }

  inline void CopyMesh::reset_sets()
  {
    copySets.clear();
    expandSets.clear();
  }

  inline CESets &CopyMesh::copy_sets()
  {
    return copySets;
  }
  
  inline CESets &CopyMesh::expand_sets()
  {
    return expandSets;
  }
  
  inline std::set<iMesh::EntitySetHandle> &CopyMesh::unique_sets()
  {
    return uniqueSets;
  }
} // namespace MeshKit
#endif

  
