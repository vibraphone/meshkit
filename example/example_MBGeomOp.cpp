/*!
\example example_MBGeomOp.cpp

\section example_MBGeomOp_cpp_title <pretty-name-of-this-file>

\subsection example_MBGeomOp_cpp_inf Misc. Information
\author <your-name-here>
\date 7-15-2013
\bug <placeholder>
\warning <placeholder>

\subsection example_MBGeomOp_cpp_goal Goal

\subsection example_MBGeomOp_cpp_cw Code Walkthrough

\subsection example_MBGeomOp_cpp_in Input
\image html example_MBGeomOp.in.jpg
There is no input.

\subsection example_MBGeomOp_cpp_out Output
\image html example_MBGeomOp.out.jpg

\subsection example_MBGeomOp_cpp_src Source Code
*/

#include <iostream>

#include <time.h>
#include <stdlib.h>
#include <cstring>
#include <cstring>

#include "meshkit/MKCore.hpp"
#include "meshkit/MeshOp.hpp"
#include "meshkit/ModelEnt.hpp"
#include "meshkit/MBGeomOp.hpp"

using namespace MeshKit;



MKCore *mk;

std::string usage_string =
    "<input file>      input file \n"
    "<output file>     Output final model to given file \n"
    "\n";

int main(int argc, char* argv[])
{
  // check command line arg
  const char *filename = 0;
  const char *outfile = 0;
  std::string fstr;
  if (argc==3)
  {
    filename = argv[1];
    outfile = argv[2];
  }
  else
  {
    std::cout << usage_string ;
    fstr=TestDir + "/partBed.smf";
    std::cout << "using default input file:" << fstr <<", no output\n";
    filename = fstr.c_str();
  }

  // initialize everything

  mk = new MKCore();
  mk->load_mesh(filename);
  MEntVector selection, dum;
  mk->get_entities_by_dimension(2, dum);
  selection.push_back(*dum.rbegin());// push just the last one retrieved from core

  mk->construct_meshop("MBGeomOp", selection);

  mk->setup_and_execute();

  if(outfile)
  {
    std::cout << "writing the set output to " << outfile << std::endl;
    moab::EntityHandle outset= (*dum.begin())->mesh_handle();
    mk->moab_instance()->write_mesh(outfile, &outset, 1);// write the original mesh set
    // it will write its children too
  }
  return 0;

}

  //process options
