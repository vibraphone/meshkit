/*!
\example example_volice.cpp

\section volIce_cpp_title CAMALPaver and OneToOneSwept for Creating IceSheet Model


\subsection volIce_cpp_src Source Code
*/

#include "meshkit/MKCore.hpp"
#include "meshkit/OneToOneSwept.hpp"
#include "meshkit/SizingFunction.hpp"
#include "meshkit/ModelEnt.hpp"
#include "meshkit/Matrix.hpp"
#include "meshkit/VertexMesher.hpp"
#include "meshkit/CAMALPaver.hpp"
#include "meshkit/FBiGeom.hpp"


using namespace MeshKit;

MKCore *mk = NULL;

void test_ice();


int main(int argc, char **argv) 
{
    // start up MK and load the geometry
    mk = new MKCore();
    int num_fail = 0;\
    test_ice();
    return num_fail;
}

void test_ice()
{
    std::string file_name = (std::string) MESH_DIR +  "/volIce.h5m";

    FBiGeom * fbiGeom = new FBiGeom(); // true for smooth, false for linear
    int ix = mk->add_igeom_instance(fbiGeom);

    iRel::PairHandle * pair;
    mk->irel_instance()->createPair (
                fbiGeom->instance(), iRel::ENTITY, iRel::FBIGEOM_IFACE, iRel::ACTIVE,
                mk->imesh_instance()->instance(), iRel::SET, iRel::IMESH_IFACE, iRel::ACTIVE, pair);

    int ixrel = mk->add_irel_pair(pair);
    char opts[]="SMOOTH;";                                  // relate , populate
    mk->load_geometry(file_name.c_str(), opts, ix, 0, ixrel, false, true);
    // get the volumes
    MEntVector vols, surfs, curves, vertices;
    mk->get_entities_by_dimension(3, vols);
    std::cout << "Volume size = " << vols.size() << std::endl;

    ModelEnt *this_vol = (*vols.rbegin());

    // test getting surfaces
    this_vol->get_adjacencies(2, surfs);

    // test getting edges
    this_vol->get_adjacencies(1, curves);

    // test getting vertices
    this_vol->get_adjacencies(0, vertices);

    MEntVector s1;
    s1.push_back(surfs[0]);
    (CAMALPaver*)mk->construct_meshop("CAMALPaver", s1);
    //set up the size
    SizingFunction * pavSize = new SizingFunction(mk, -1, 600);

    s1[0]->sizing_function_index(pavSize->core_index());

    mk->setup_and_execute();

    //make a one-to-one sweeping
    OneToOneSwept *sw = (OneToOneSwept*) mk->construct_meshop("OneToOneSwept", vols);

    sw->SetSourceSurface(0);
    sw->SetTargetSurface(1);
    SizingFunction swSize(mk, 5, -1);

    this_vol->sizing_function_index(swSize.core_index());

    //mk->add_arc( cmp, sw);// it means paver will be setup first?
    //set up for the sweeping and sweep
    mk->setup_and_execute();

    //check the number of cells after OneToOneSwept
    moab::Range hex;
    mk->moab_instance()->get_entities_by_dimension(0, 3, hex);
    mk->save_mesh("OneToOneSwept.vtk");

    //delete sw;
    delete mk->vertex_mesher();
    mk->clear_graph();

}

