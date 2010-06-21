/*********************************************
June,10
Reactor Assembly Mesh Assembler
Argonne National Laboratory

CCrgen class declaration
*********************************************/
#ifndef __RGG_MESH_H__
#define __RGG_MESH_H__

#include <iostream>
#include <cfloat>
#include <math.h>
#include "MKUtils.hpp"
#include "MergeMesh.hpp"
#include "CopyMesh.hpp"
#include "parser.hpp"
#include "fileio.hpp"
#include "clock.hpp"
#include <sstream>
#include <string>
#include "matrixtemplate.hpp"

class CCrgen
{
public:
  CCrgen ();    // ctor
  ~CCrgen ();   // dtor
  int prepareIO (int argc, char *argv[]);
  int load_meshes();
  int read_inputs_phase1 ();
  int read_inputs_phase2 ();
  int write_makefile ();
  int find_assm(const int i, int &assm_index);
  int banner();
  int copy_move(); 
  int merge_nodes();
  int assign_gids();
  int save();
  int move_verts(iBase_EntitySetHandle set, const double *dx);
  int get_copy_expand_sets(CopyMesh *cm,
			   iBase_EntitySetHandle orig_set, 
			   const char **tag_names, const char **tag_vals, 
			   int num_tags, int copy_or_expand);

  int extend_expand_sets(CopyMesh *cm);

  int copy_move_hex_flat_assys(CopyMesh **cm,
			       const int nrings, const int pack_type,
			       const double pitch,
			       const int symm,
			       std::vector<std::string> &core_alias,
			       std::vector<iBase_EntitySetHandle> &assys);

  int copy_move_sq_assys(CopyMesh **cm,
			 const int nrings, const int pack_type,
			 const double pitch,
			 const int symm,
			 std::vector<std::string> &core_alias,
			 std::vector<iBase_EntitySetHandle> &assys);

  int copy_move_hex_full_assys(CopyMesh **cm,
			       const int nrings, const int pack_type,
			       const double pitch,
			       const int symm,
			       std::vector<std::string> &core_alias,
			       std::vector<iBase_EntitySetHandle> &assys);

  int copy_move_hex_vertex_assys(CopyMesh **cm,
				 const int nrings, const int pack_type,
				 const double pitch,
				 const int symm,
				 std::vector<std::string> &core_alias,
				 std::vector<iBase_EntitySetHandle> &assys);


  int copy_move_one_twelfth_assys(CopyMesh **cm,
				  const int nrings, const int pack_type,
				  const double pitch,
				  const int symm,
				  std::vector<std::string> &core_alias,
				  std::vector<iBase_EntitySetHandle> &assys);

  // phase 1's
  int copy_move_hex_vertex_assys_p1(CopyMesh **cm,
				    const int nrings, const int pack_type,
				    const double pitch,
				    const int symm,
				    std::vector<std::string> &core_alias,
				    std::vector<iBase_EntitySetHandle> &assys);

  int copy_move_hex_flat_assys_p1(CopyMesh **cm,
				  const int nrings, const int pack_type,
				  const double pitch,
				  const int symm,
				  std::vector<std::string> &core_alias,
				  std::vector<iBase_EntitySetHandle> &assys);

  int copy_move_sq_assys_p1(CopyMesh **cm,
			    const int nrings, const int pack_type,
			    const double pitch,
			    const int symm,
			    std::vector<std::string> &core_alias,
			    std::vector<iBase_EntitySetHandle> &assys);

  int copy_move_hex_full_assys_p1(CopyMesh **cm,
				  const int nrings, const int pack_type,
				  const double pitch,
				  const int symm,
				  std::vector<std::string> &core_alias,
				  std::vector<iBase_EntitySetHandle> &assys);

  int copy_move_one_twelfth_assys_p1(CopyMesh **cm,
				     const int nrings, const int pack_type,
				     const double pitch,
				     const int symm,
				     std::vector<std::string> &core_alias,
				     std::vector<iBase_EntitySetHandle> &assys);
private:

  iMesh_Instance impl;
  CopyMesh **cm;
  MergeMesh *mm;
  iBase_EntitySetHandle root_set;
  std::vector<iBase_EntitySetHandle> assys;

  // declare variables read in the inputs
  int err;
  int UNITCELL_DUCT, ASSY_TYPES ;
  int nrings, nringsx, nringsy, pack_type, symm;
  double pitch, pitchx, pitchy;
  bool global_ids, back_mesh;
  std::vector<std::string> files;
  std::string outfile;
  int nassys; // the number of mesh files
  int tot_assys; // total no. of assms forming core
  int set_DIM; // default is 3D
  double PI;

  // file related
  std::ifstream file_input;    // File Input
  std::ofstream make_file;    // File Output
  std::string iname, ifile, mfile, geometry, back_meshfile; 
  int linenumber;
  std::string card,geom_type, meshfile, mf_alias, temp_alias;
  std::vector<std::string> assm_alias;
  std::vector<std::string> core_alias;
  
  // parsing related
  std::string input_string;
  std::string comment ;
  int MAXCHARS ;

  // merge related
  double merge_tol;
  int do_merge;
  int update_sets;
  iBase_TagHandle merge_tag;

  // MKUtils obj, assigning gid's etc.
  MKUtils *mu;
};
#endif