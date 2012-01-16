#include "meshkit/RegisterMeshOp.hpp"
#include "meshkit/CopyMesh.hpp"
#include "meshkit/CopyGeom.hpp"
#include "meshkit/MergeMesh.hpp"
#include "meshkit/EBMesher.hpp"
#include "meshkit/EdgeMesher.hpp"
#include "meshkit/TFIMapping.hpp"
#include "meshkit/ExtrudeMesh.hpp"
#include "meshkit/OneToOneSwept.hpp"
#include "meshkit/SCDMesh.hpp"
#include "meshkit/VertexMesher.hpp"
#include "meshkit/QslimMesher.hpp"
#include "meshkit/QuadMesh.hpp"
#include "meshkit/AssyGen.hpp"
#include "meshkit/TriangleMesher.hpp"
#ifdef HAVE_FBIGEOM
#include "meshkit/MBGeomOp.hpp"
#include "meshkit/MBSplitOp.hpp"
#endif
#ifdef USE_MPI
#ifdef HAVE_PARALLEL_MOAB
#ifdef HAVE_PARALLEL_CGM
#include "meshkit/ParallelMesher.hpp"
#include "meshkit/ParExchangeMesh.hpp"
#include "meshkit/ParPostRecv.hpp"
#endif
#endif
#endif

namespace MeshKit {

/**\brief Dummy function to force load from static library */
extern int register_algs_mesh_ops() { return 1; }

/**\brief Register a MeshOp type during initialization
 *\param NAME Class name for MeshOp subclass
 */
#define REGISTER_MESH_OP(NAME) \
  RegisterMeshOp<NAME> NAME ## _GLOBAL_PROXY

REGISTER_MESH_OP(VertexMesher);
REGISTER_MESH_OP(EdgeMesher);
REGISTER_MESH_OP(OneToOneSwept);
REGISTER_MESH_OP(TFIMapping);
REGISTER_MESH_OP(SCDMesh);
REGISTER_MESH_OP(CopyMesh);
REGISTER_MESH_OP(MergeMesh);
REGISTER_MESH_OP(CopyGeom);
REGISTER_MESH_OP(ExtrudeMesh);
REGISTER_MESH_OP(EBMesher);
REGISTER_MESH_OP(QslimMesher);
REGISTER_MESH_OP(QuadMesher);
REGISTER_MESH_OP(AssyGen);
REGISTER_MESH_OP(TriangleMesher);
#ifdef HAVE_FBIGEOM
REGISTER_MESH_OP(MBGeomOp);
REGISTER_MESH_OP(MBSplitOp);
#endif

#ifdef USE_MPI
#ifdef HAVE_PARALLEL_MOAB
#ifdef HAVE_PARALLEL_CGM
REGISTER_MESH_OP(ParallelMesher);
REGISTER_MESH_OP(ParExchangeMesh);
REGISTER_MESH_OP(ParPostRecv);
#endif
#endif
#endif

} // namespace MeshKit
