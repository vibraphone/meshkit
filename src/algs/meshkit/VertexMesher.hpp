#ifndef VERTEXMESHER_HPP
#define VERTEXMESHER_HPP

#include "meshkit/Types.h"
#include "meshkit/Error.hpp"
#include "meshkit/MeshScheme.hpp"
#include "meshkit/ModelEnt.hpp"
#include "iGeom.h"
#include "moab/Interface.hpp"
#include <vector>

namespace MeshKit {

class MKCore;
    

/** \class VertexMesher VertexMesher.hpp "meshkit/VertexMesher.hpp"
 * \brief A simple class for meshing geometric vertices
 *
 * INPUT: one or more ModelEnts representing geometric vertices
 * MESH TYPE(S): MBVERTEX
 * OUTPUT: one mesh vertex for each ModelEnt
 * DEPENDENCIES: (none)
 * CONSTRAINTS: ModelEnts must be geometric vertices, i.e. with dimension() == 0
 *
 * This class performs the trivial task of meshing geometric vertices.  Typically there will
 * only be a single instance of this class, and therefore it is pointed to and managed by MKCore.
 * It will also be inserted into the meshing graph during the setup phase of most edge meshers.
 *
 * The single instance of this class stores all the ModelEnt's representing geometric vertices,
 * and after execution, an MEntSelection entry for each geometric vertex and mesh vertex pair.
 */
class VertexMesher : public MeshScheme
{
public:

    //! Bare constructor
  VertexMesher(MKCore *mkcore, const MEntVector &me_vec = MEntVector());

    //! Destructor
  virtual ~VertexMesher();

    /** \brief Factory function registered with MeshOpFactory
     * \param mkcore MKCore instance for the factory
     * \param me_vec ModelEnts to which this scheme will be applied
     */
  static MeshOp *factory(MKCore *mkcore, const MEntVector &me_vec);
  
    /** \brief Return the mesh entity types operated on by this scheme
     * \param tps Entity types generated by this scheme
     */
  virtual void mesh_types(std::vector<moab::EntityType> &tps);
  
    /** \brief Re-implemented here so we can check topological dimension of model_ent
     * \param model_ent ModelEnt being added
     */
  virtual bool add_modelent(ModelEnt *model_ent);

    //! Setup is a no-op, but must be provided since it's pure virtual
  virtual void setup_this();

    //! The only setup/execute function we need, since meshing vertices is trivial
  virtual void execute_this();

protected:
  
private:

    //! No copy constructor, since there's only meant to be one of these
  VertexMesher(const VertexMesher &);
  
    //! No operator=, since there's only meant to be one of these
  VertexMesher &operator=(const VertexMesher &);

};

}

#endif

  
