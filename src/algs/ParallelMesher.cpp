#include "meshkit/ParallelMesher.hpp"
#include "meshkit/ModelEnt.hpp"
#include "meshkit/ParExchangeMesh.hpp"

#include "MKDefines.h"
#include "MBTagConventions.hpp"
#include "MBEntityType.h"
#include "moab/Range.hpp"
#include "moab/Types.hpp"

#include "CGMParallelComm.hpp"
#include "RefEntity.hpp"
#include "RefVolume.hpp"
#include "RefFace.hpp"
#include "RefEdge.hpp"
#include "RefVertex.hpp"
#include "TDParallel.hpp"
#include "CADefines.hpp"
#include "GeometryQueryTool.hpp"

const bool debug_parallelmesher = false;

namespace MeshKit 
{
// static registration of this mesh scheme
moab::EntityType ParallelMesher_tps[] = {moab::MBVERTEX, moab::MBTRI, moab::MBTET, moab::MBMAXTYPE};
const moab::EntityType* ParallelMesher::output_types()
  { return ParallelMesher_tps; }

ParallelMesher::ParallelMesher(MKCore *mkcore, const MEntVector &me_vec)
  : MeshScheme(mkcore, me_vec)
{
  // get information related to MOAB parallel communication
  m_mpcomm = moab::ParallelComm::get_pcomm(mk_core()->moab_instance(), 0);
  if (NULL == m_mpcomm) m_mpcomm = new ParallelComm(mk_core()->moab_instance());
  m_rank = m_mpcomm->proc_config().proc_rank();

  // create tag
  iMesh::Error err = mk_core()->imesh_instance()->createTag("PARALLEL_UNIQUE_ID", 1, iBase_INTEGER, m_mPuniqueIDTag);
  IBERRCHK(err, "Trouble create a parallel unique id tag handle.");

  m_sEntity.resize(7);
}

ParallelMesher::~ParallelMesher()
{
  delete m_mpcomm;
}

MeshOp *ParallelMesher::get_mesher(PARALLEL_OP_TYPE type)
{
  MeshOpProxy* proxy = NULL;
  std::vector<MeshOpProxy *> proxies;
  if (type == MESH_VERTEX) {
    mk_core()->meshop_by_mesh_type(moab::MBVERTEX, proxies);
    proxy = *proxies.begin();  }
  else if (type == MESH_EDGE) {
    mk_core()->meshop_by_mesh_type(moab::MBEDGE, proxies);
    proxy = *proxies.begin();
  }
  else if (type == MESH_SURF) {
    mk_core()->meshop_by_mesh_type(moab::MBTRI, proxies);
    proxy = *proxies.begin();
  }
  else if (type == MESH_VOLUME) {
    mk_core()->meshop_by_mesh_type(moab::MBTET, proxies);
    proxy = *proxies.begin();
  }
  else if (type == EXCHANGE_VERTEX || type == EXCHANGE_EDGE ||
           type == EXCHANGE_SURF) {
    proxy = MKCore::meshop_proxy("ParExchangeMesh");
  }

  if (proxy == NULL) throw Error(MK_FAILURE, "Couldn't find a MeshOp capable of producing the given mesh type.");
  
  return mk_core()->construct_meshop(proxy);
}

void ParallelMesher::setup_this()
{
  // make partitioned and send and receive model entity sets
  for (MEntSelection::iterator mit = mentSelection.begin();
       mit != mentSelection.end(); mit++) {
    ModelEnt *me_vol = (*mit).first;
    if (me_vol->dimension() != 3) throw Error(MK_BAD_INPUT, "Parallel mesher assigned to an entity with dimension != 3.");

    RefEntity* vol = reinterpret_cast<RefEntity*> (me_vol->geom_handle());
    TDParallel *td_par_vol = (TDParallel *) vol->get_TD(&TDParallel::is_parallel);
    if (td_par_vol == NULL) ECERRCHK(MK_FAILURE, "Volume should have partitioned information.");

    unsigned int charge_proc = td_par_vol->get_charge_proc();
    MEntVector children;

    if (m_rank == charge_proc) { // partititoned vols
      m_sEntity[MESH_VOLUME].insert(me_vol); // volume

      for (int i = 2; i > -1; i--) { // child entities
        children.clear();
        me_vol->get_adjacencies(i, children);

        for (MEntVector::iterator cit = children.begin(); cit != children.end(); cit++) {
          RefEntity* child = reinterpret_cast< RefEntity* > ((*cit)->geom_handle());
          TDParallel *td_par_child = (TDParallel *) child->get_TD(&TDParallel::is_parallel);

          if (i == 2) { // surface : make local, send/non-send surface sets
            if (td_par_child != NULL) {
              charge_proc = td_par_child->get_charge_proc();
              if (m_rank == charge_proc) {
                m_sEntity[MESH_SURF].insert(*cit);
              }
              m_sEntity[EXCHANGE_SURF].insert(*cit);
            }
          }
          else if (i == 1) { // edge
            if (td_par_child != NULL) {
              charge_proc = td_par_child->get_charge_proc();
              if (m_rank == charge_proc) {
                m_sEntity[MESH_EDGE].insert(*cit);
              }
              m_sEntity[EXCHANGE_EDGE].insert(*cit);
            }
          }
          else if (i == 0) { // vertex
            if (td_par_child != NULL) {
              charge_proc = td_par_child->get_charge_proc();
              if (m_rank == charge_proc) {
                m_sEntity[MESH_VERTEX].insert(*cit);
              }
              m_sEntity[EXCHANGE_VERTEX].insert(*cit);
            }
          }

          if (td_par_child != NULL) {
            // set geometry unique id to corresponding surface entityset
            unsigned int unique_id = td_par_child->get_unique_id();
            iBase_EntitySetHandle entityset = reinterpret_cast<iBase_EntitySetHandle> ((*cit)->mesh_handle());
            iMesh::Error err = mk_core()->imesh_instance()->setEntSetIntData(entityset, m_mPuniqueIDTag, unique_id);
            IBERRCHK(err, "Couldn't set geometry unique id to corresponding mesh entityset.");
          }
        }
      }
    }
  }
  add_parallel_mesh_op(MESH_VERTEX);
  add_parallel_mesh_op(EXCHANGE_VERTEX);
  add_parallel_mesh_op(MESH_EDGE);
  add_parallel_mesh_op(EXCHANGE_EDGE);
  add_parallel_mesh_op(MESH_SURF);
  add_parallel_mesh_op(EXCHANGE_SURF);
  add_parallel_mesh_op(MESH_VOLUME);
}

void ParallelMesher::print_geom_info(ModelEnt* me, const int dim,
                                   const bool local)
{
  RefEntity* ent = reinterpret_cast<RefEntity*> (me->geom_handle());
  CubitVector edge_geom = ent->center_point();
  moab::EntityHandle entityset = me->mesh_handle();
  TDParallel *td_par = (TDParallel *) ent->get_TD(&TDParallel::is_parallel);
  std::cout << "Partitioned_dim=" << dim;
  if (local) std::cout << ",local";
  else std::cout << ",remote";
  std::cout << ",geom_center=" << edge_geom.x()
            << " "<< edge_geom.y() << " " << edge_geom.z() << ", meshset="
            << entityset << ",uid=" << td_par->get_unique_id() << std::endl;
}

void ParallelMesher::add_parallel_mesh_op(PARALLEL_OP_TYPE type, bool after)
{
  bool inserted = false;
  MeshOp *mesher = NULL;
  std::vector<MeshOp*> meshops;
  MEntSet::iterator iter = m_sEntity[type].begin();
  MEntSet::iterator end_iter = m_sEntity[type].end();

  for (; iter != end_iter; iter++) {
    bool found = false;
    if (!mesher) mesher = get_mesher(type); // get a mesher if we haven't already
    meshops.clear();
    (*iter)->get_meshops(meshops);
    int n_meshops = meshops.size();

    if (n_meshops > 0) {
      for (int i = 0; i < n_meshops; i++) {
        if (meshops[i] == mesher) {
          found = true;
          break;
        }
      }
    }
    
    if (!found) { // if no specified meshop
      // add this entity to it, and if first, make sure it's added upstream
      mesher->add_modelent(*iter);
      if (!inserted) {
        if (after) mk_core()->insert_node(mesher, mk_core()->leaf_node(), this);
        else mk_core()->insert_node(mesher, this);
        inserted = true;
      }
    }
  }
}

void ParallelMesher::execute_this()
{
}

void ParallelMesher::print_mesh()
{
  moab::ErrorCode rval;
  int tmp_procs[MAX_SHARING_PROCS];
  moab::EntityHandle tmp_hs[MAX_SHARING_PROCS];
  unsigned char pstat;
  int num_ps;
  moab::Range entities;

  for (moab::EntityType type = MBVERTEX; type != MBMAXTYPE; type++) {
    entities.clear();
    rval = mk_core()->moab_instance()->get_entities_by_type(NULL, type, entities);
    MBERRCHK(rval, mk_core()->moab_instance());

    for (moab::Range::iterator rit = entities.begin(); rit != entities.end(); rit++) {
      rval = m_mpcomm->get_sharing_data(*rit, tmp_procs, tmp_hs, pstat, num_ps);
      MBERRCHK(rval, mk_core()->moab_instance());

      if (type != MBENTITYSET) {
        std::cout << "ParallelMesher::entity=" << *rit << ", type=" << type;
        if (type == MBVERTEX) {
          double coord[3];
          rval = mk_core()->moab_instance()->get_coords(&(*rit), 1, coord);
          MBERRCHK(rval, mk_core()->moab_instance());
          std::cout << ", coords=" << coord[0] << " " << coord[1] << " " << coord[2];
        }
        else {
          std::vector<moab::EntityHandle> connect;
          rval = mk_core()->moab_instance()->get_connectivity(&(*rit), 1, connect);
          MBERRCHK(rval, mk_core()->moab_instance());
          int n_conn = connect.size();
          std::cout << ", connect=";
          for (int j = 0; j < n_conn; j++) {
            std::cout << connect[j] << " ";
          }
        }
        
        std::cout << ", shared_info=";
        for (int ii = 0; ii < num_ps; ii++) {
          std::cout << tmp_procs[ii] << ":" << tmp_hs[ii] << ", ";
        }
        std::cout << std::endl;
      }
    }
  }    
}
} // namespace MeshKit