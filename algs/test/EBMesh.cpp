#include <iostream>
#include <time.h>
#include <stdlib.h>
#include <string.h>
#include <sstream>
#include <stdio.h>

#include "EBMesh.hpp"

#define STRINGIFY_(X) #X
#define STRINGIFY(X) STRINGIFY_(X)

#define DEFAULT_TEST_FILE STRINGIFY(SRCDIR) "/sphere.stp"
#define ERROR(a) {if (iBase_SUCCESS != err) std::cerr << a << std::endl;}
#define ERRORR(a,b) {if (iBase_SUCCESS != err) {std::cerr << a << std::endl; return b;}}

int load_and_mesh(const char *input_filename,
		  const char *output_filename,
		  double size, int input_geom,
		  int vol_frac_res);

int main(int argc, char* argv[])
{
#ifdef HAVE_OCC
  // check command line arg
  const char *input_filename = NULL;
  const char *output_filename = NULL;
  int input_geom = 1; // if input file is geometry format
  double size = -1.;
  int add_layer = 3;
  int vol_frac_res = 0;

  if (argc > 2 && argc < 8) {
    input_filename = argv[1];
    input_geom = atoi(argv[2]);
    if (argc > 3) size = atof(argv[3]);
    if (argc > 4) output_filename = argv[4];
    if (argc > 5) add_layer = atoi(argv[5]);
    if (argc > 6) vol_frac_res = atoi(argv[6]);
  }
  else {
    std::cout << "Usage: " << argv[0] << "<input_geom_filename> <input_solid> {interval_size} {output_mesh_filename} {#_add_layer} {vol_frac_res}" << std::endl;
    std::cout << "<input_geom_filename> : input geometry file name" << std::endl;
    std::cout << "<input_solid> : if the input file is solid model geometry file, 1(solid model geometry) or 0(facet based geometry)" << std::endl;
    std::cout << "{interval_size} : optional argument, interval size, if it is not set, EBMesh find a interval size from # of facet triangles." << std::endl;
    std::cout << "{output_mesh_filename} : optional argument, can output mesh file (e.g. output.vtk.)" << std::endl;
    std::cout << "{#_add_layer} : optional argument, # of additional outside layers, should be > 3, default (3)" << std::endl;
    std::cout << "{vol_frac_res} : optional argument, volume fraction resolution of boundary cells for each material, you can specify it as # of divisions (e.g. 4)." << std::endl;
    std::cout << std::endl;
    if (argc != 1) return 1;
    std::cout << "No file specified.  Defaulting to: " << DEFAULT_TEST_FILE << std::endl;
    input_filename = DEFAULT_TEST_FILE;
  }
  
  if (load_and_mesh(input_filename, output_filename,
		    size, input_geom, vol_frac_res)) return 1;
#endif

  return 0;
}

int load_and_mesh(const char *input_filename,
		  const char *output_filename,
		  double size, int input_geom,
		  int vol_frac_res)
{
  // initialize the Mesh
  int err;
  bool result;
  time_t start_time, load_time, mesh_time, vol_frac_time,
    export_time, query_time_techX, query_time;
  iMesh_Instance mesh;
  iMesh_newMesh("", &mesh, &err, 0);
  ERRORR("Couldn't create mesh.", 1);

  iBase_EntitySetHandle root_set;
  iMesh_getRootSet(mesh, &root_set, &err);
  ERRORR("Couldn't get root set.", 1);

  // read geometry and establish facet mesh
  time(&start_time);
  iMesh_load(mesh, root_set, input_filename, NULL,
	     &err, strlen(input_filename), 0);
  ERRORR("Couldn't load mesh file.", 1);
  time(&load_time);

  // make EB mesher
  EBMesh *ebm = new EBMesh(mesh, root_set, size, input_geom);

  // do mesh
  err = ebm->do_mesh();
  ERRORR("Couldn't EB mesh.", 1);
  time(&mesh_time);

  // caculate volume fraction, only for geometry input
  if (vol_frac_res > 0 && input_geom) {
    result = ebm->get_volume_fraction(vol_frac_res);
    if (!result) {
      std::cerr << "Couldn't get volume fraction." << std::endl;
      return 1;
    }
  }
  time(&vol_frac_time);

  // export mesh
  if (output_filename != NULL) {
    result = ebm->export_mesh(output_filename);
    if (!result) {
      std::cerr << "Couldn't export mesh." << std::endl;
      return 1;
    }
  }
  time(&export_time);

  // techX query function test
  double boxMin[3], boxMax[3];
  int nDiv[3];
  std::map< CutCellSurfEdgeKey, std::vector<double>, LessThan > mdCutCellSurfEdge;
  std::vector<int> vnInsideCellTechX;
  
  result = ebm->get_grid_and_edges_techX(boxMin, boxMax, nDiv,
					 mdCutCellSurfEdge, vnInsideCellTechX);
  if (!result) {
    std::cerr << "Couldn't get  mesh information for TechX." << std::endl;
    return 1;
  }
  time(&query_time_techX);

  // multiple intersection fraction query test
  std::map< CutCellSurfEdgeKey, std::vector<double>, LessThan > mdCutCellEdge;
  std::vector<int> vnInsideCell;
  result = ebm->get_grid_and_edges(boxMin, boxMax, nDiv,
				   mdCutCellEdge, vnInsideCell);
  if (!result) {
    std::cerr << "Couldn't get mesh information." << std::endl;
    return 1;
  }
  time(&query_time);

  std::cout << "EBMesh is succesfully finished." << std::endl;
  std::cout << "# of TechX cut-cell surfaces: " << mdCutCellSurfEdge.size() 
	    << ", # of nInsideCell: " << vnInsideCell.size()/3 << std::endl;
  std::cout << "Time including loading: "
	    << difftime(mesh_time, start_time)
	    << " secs, Time excluding loading: "
	    << difftime(mesh_time, load_time)
	    << " secs, Time volume fraction: "
	    << difftime(vol_frac_time, mesh_time)
	    << " secs, Time export mesh: "
	    << difftime(export_time, vol_frac_time)
	    << " secs, TechX query time: "
	    << difftime(query_time_techX, export_time)
	    << " secs, normal query time(elems, edge-cut fractions): "
	    << difftime(query_time, query_time_techX)
	    << " secs." << std::endl;

  delete ebm;
  iMesh_dtor(mesh, &err);
  ERRORR("Couldn't destroy mesh.", 1);

  return 0;
}
