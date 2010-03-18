#include <iostream>
#include <time.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include "CutCellMesh.hpp"

#define DEFAULT_TEST_FILE_GEOM "sphere.stp"
#define ERROR(a) {if (iBase_SUCCESS != err) std::cerr << a << std::endl;}
#define ERRORR(a,b) {if (iBase_SUCCESS != err) {std::cerr << a << std::endl; return b;}}

int load_and_cutcell(const char *filename_geom, double size,
		     int exp, int exp_type);

int main(int argc, char* argv[])
{
  // check command line arg
  const char *filename_geom = 0;
  double size = -1.;
  int exp = 0;
  int exp_type = 0;

  if (argc == 5) {
    filename_geom = argv[1];
    size = atof(argv[2]);
    exp = atof(argv[3]);
    exp_type = atof(argv[4]);
  }
  else {
    printf("Usage: %s <geom_filename> <interval_size> <export[0 1]> <export_file_type[0:vtk 1:h5m]>\n", argv[0]);
    if (argc != 1) return 1;
    printf("No file specified.  Defaulting to: %s\n", DEFAULT_TEST_FILE_GEOM);
    filename_geom = DEFAULT_TEST_FILE_GEOM;
  }
  
  if (load_and_cutcell(filename_geom, size, exp, exp_type)) return 1;
  
  return 0;
}

int load_and_cutcell(const char *filename_geom, double size,
		     int exp, int exp_type)
{
  // initialize the Mesh
  int err;
  iMesh_Instance mesh;
  iMesh_newMesh("", &mesh, &err, 0);
  ERRORR("Couldn't create mesh.", 1);

  iBase_EntitySetHandle root_set;
  iMesh_getRootSet(mesh, &root_set, &err);
  ERRORR("Couldn't get root set.", 1);

  // read geometry and establish facet mesh
  clock_t start_time = clock();
  iMesh_load(mesh, root_set, filename_geom, NULL, &err, strlen(filename_geom), 0);
  ERRORR("Couldn't load mesh file.", 1);
  clock_t load_time = clock();

  // make cut cell mesher
  CutCellMesh *ccm = new CutCellMesh(mesh, root_set, size);

  // do mesh
  err = ccm->do_mesh(exp, exp_type);
  ERRORR("Couldn't cut-cell mesh.", 1);
  clock_t mesh_time = clock();

  // techX query function test
  double boxMin[3], boxMax[3];
  int nDiv[3];
  std::map< CutCellSurfKey, std::vector<double>, LessThan > mdCutCellSurfEdge;
  std::vector<int> vnInsideCell;
  
  bool rVal = ccm->get_grid_and_edges(boxMin, boxMax, nDiv,
				      mdCutCellSurfEdge, vnInsideCell);
  if (!rVal) {
    std::cerr << "Couldn't get mesh information." << std::endl;
    return 1;
  }
  clock_t query_time = clock();

  std::cout << "Cut-cell mesh is succesfully produced." << std::endl;
  std::cout << "# of cut-cell surfaces: " << mdCutCellSurfEdge.size() 
	    << ", # of nInsideCell: " << vnInsideCell.size()/3 << std::endl;
  std::cout << "Time including loading: "
	    << (double) (mesh_time - start_time)/CLOCKS_PER_SEC
	    << " secs, Time excluding loading: "
	    << (double) (mesh_time - load_time)/CLOCKS_PER_SEC
	    << " secs, TechX query time: "
	    << (double) (query_time - mesh_time)/CLOCKS_PER_SEC
	    << " secs." << std::endl;

  delete ccm;
  iMesh_dtor(mesh, &err);
  ERRORR("Couldn't destroy mesh.", 1);

  return 0;
}
