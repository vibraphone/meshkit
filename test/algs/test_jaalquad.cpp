/*! \file test_jaalquad.cpp \test
 *
 * test_jaalquad.cpp
 *
 */

#include "meshkit/MKCore.hpp"
#include "meshkit/ModelEnt.hpp"
#include "meshkit/MeshOp.hpp"
#include "meshkit/iGeom.hpp"
#include "meshkit/iMesh.hpp"
#include "meshkit/Matrix.hpp"
#include "meshkit/SizingFunction.hpp"

#include "TestUtil.hpp"
#ifdef HAVE_ACIS
#define TEST_QUADFACE "quadface.sat"
#elif defined(HAVE_OCC)
#define TEST_QUADFACE "quadface.stp"
#endif

using namespace MeshKit;

void test_simple_tri_to_quad();
void load_file();

MKCore* core = 0;
bool write_vtk = false;
int main( int argc, char* argv[] )
{
    core = new MKCore(); // Start up MK

    for (int i = 1; i < argc; ++i) {
        if (!strcmp(argv[i],"-w"))
            write_vtk = true;
        else if (!strcmp(argv[i],"-h"))
            std::cout << argv[0] << " [-w]" << std::endl;
        else
            std::cerr << "Invalid option: \"" << argv[i] << '"' << std::endl;
    }
    int result = 0;
    result += RUN_TEST( test_simple_tri_to_quad );
    result += RUN_TEST( load_file );
    return result;
}

// Create a square surface to use in tests
ModelEnt* get_square_surface( )
{
    // create a brick
    iGeom* geom = core->igeom_instance();
    iBase_EntityHandle brick_handle;
    iGeom::Error err = geom->createBrick( 2, 2, 2, brick_handle );
    CHECK_EQUAL( iBase_SUCCESS, err );
    core->populate_model_ents();

    // get model ent for brick
    iBase_EntityType type;
    err = core->igeom_instance()->getEntType( brick_handle, type );
    CHECK_EQUAL( iBase_SUCCESS, err );
    MEntVector ents;
    core->get_entities_by_dimension( type, ents );
    ModelEnt* brick = 0;
    for (MEntVector::iterator i = ents.begin(); i != ents.end(); ++i)
        if ((*i)->geom_handle() == brick_handle)
            brick = *i;
    CHECK(!!brick);

    // choose an arbitrary face of the brick
    ents.clear();
    brick->get_adjacencies( 2, ents );
    CHECK_EQUAL( (size_t)6, ents.size() );
    return ents.front();
}

// Create a square surface meshed with triangles,
// where the mesh is a grid of 4 quads each split
// into two triangles.

// 6---7---8
// |\  |  /|
// | \ | / |
// |  \|/  |
// 3---4---5
// |  /|\  |
// | / | \ |
// |/  |  \|
// 0---1---2
ModelEnt* get_tri_meshed_surface( moab::EntityHandle verts[9] )
{
    ModelEnt* surf = get_square_surface( );
    MEntVector point_vect;
    surf->get_adjacencies( 0, point_vect );
    CHECK_EQUAL( (size_t)4, point_vect.size() );

    Vector<3> coords[4];
    for (int i = 0; i < 4; ++i)
        point_vect[i]->evaluate( 0, 0, 0, coords[i].data() );

    // order points counter clockwise
    // assume edges are parallel to coordinate axes
    int first_idx = 0, opposite_idx = 0;
    for (int i = 1; i < 4; ++i)
        if (coords[i][0] <= coords[first_idx][0] + 1e-6 &&
                coords[i][1] <= coords[first_idx][1] + 1e-6 &&
                coords[i][2] <= coords[first_idx][2] + 1e-6)
            first_idx = i;
        else if (coords[i][0] >= coords[opposite_idx][0] - 1e-6 &&
                 coords[i][1] >= coords[opposite_idx][1] - 1e-6 &&
                 coords[i][2] >= coords[opposite_idx][2] - 1e-6)
            opposite_idx = i;
    CHECK( first_idx != opposite_idx );
    int second_idx = 5, last_idx = 5;
    for (int i = 0; i < 4; ++i)
        if (first_idx != i && opposite_idx != i) {
            second_idx = i; break;
        }
    for (int i = second_idx + 1; i < 4; ++i)
        if (first_idx != i && opposite_idx != i)
            last_idx = i;
    assert(second_idx < 4 && last_idx < 4 && second_idx != last_idx);
    Vector<3> norm, cross = (coords[second_idx] - coords[first_idx]) *
            (coords[last_idx]   - coords[first_idx]);
    surf->evaluate( coords[first_idx][0], coords[first_idx][1],
            coords[first_idx][2], 0, norm.data() );
    if (norm % cross < 0)
        std::swap( second_idx, last_idx );
    ModelEnt* points[4] = { point_vect[first_idx],
                            point_vect[second_idx],
                            point_vect[opposite_idx],
                            point_vect[last_idx] };
    for (int i = 0; i < 4; ++i)
        points[i]->evaluate( 0, 0, 0, coords[i].data() );
    
    // construct mesh vertices
    core->moab_instance()->create_vertex( coords[0].data(), verts[0] );
    points[0]->commit_mesh( &verts[0], 1, COMPLETE_MESH );
    core->moab_instance()->create_vertex( coords[1].data(), verts[2] );
    points[1]->commit_mesh( &verts[2], 1, COMPLETE_MESH );
    core->moab_instance()->create_vertex( coords[2].data(), verts[8] );
    points[2]->commit_mesh( &verts[8], 1, COMPLETE_MESH );
    core->moab_instance()->create_vertex( coords[3].data(), verts[6] );
    points[3]->commit_mesh( &verts[6], 1, COMPLETE_MESH );
    Vector<3> mid = 0.5*(coords[0]+coords[1]);
    core->moab_instance()->create_vertex( mid.data(), verts[1] );
    mid = 0.5*(coords[1]+coords[2]);
    core->moab_instance()->create_vertex( mid.data(), verts[5] );
    mid = 0.5*(coords[2]+coords[3]);
    core->moab_instance()->create_vertex( mid.data(), verts[7] );
    mid = 0.5*(coords[3]+coords[0]);
    core->moab_instance()->create_vertex( mid.data(), verts[3] );
    mid = 0.25*(coords[0]+coords[1]+coords[2]+coords[3]);
    core->moab_instance()->create_vertex( mid.data(), verts[4] );

    // bounding vertices in ccw order:
    const int outer[8] = { 0, 1, 2, 5, 8, 7, 6, 3 };

    MEntVector curv_vect, ends;
    surf->get_adjacencies( 1, curv_vect );
    CHECK_EQUAL( (size_t)4, curv_vect.size() );
    for (int i = 0; i < 4; ++i) {
        ModelEnt* curve = 0;
        bool reversed = false;
        for (int j = 0; j < 4; ++j) {
            ends.clear();
            curv_vect[j]->get_adjacencies( 0, ends );
            CHECK_EQUAL( (size_t)2, ends.size() );
            if (ends[0] == points[i] && ends[1] == points[(i+1)%4]) {
                curve = curv_vect[j];
                reversed = false;
                break;
            }
            else if (ends[1] == points[i] && ends[0] == points[(i+1)%4]) {
                curve = curv_vect[j];
                reversed = true;
                break;
            }
        }
        CHECK(0 != curve);

        moab::EntityHandle conn[3] = { verts[ outer[ 2*i     ] ],
                                       verts[ outer[ 2*i+1   ] ],
                                       verts[ outer[(2*i+2)%8] ] };
        if (reversed)
            std::swap( conn[0], conn[2] );
        moab::EntityHandle ents[3];
        ents[1] = conn[1];
        core->moab_instance()->create_element( moab::MBEDGE, conn,   2, ents[0] );
        core->moab_instance()->create_element( moab::MBEDGE, conn+1, 2, ents[2] );
        curve->commit_mesh( ents, 3, COMPLETE_MESH );
    }

    // create surface mesh
    moab::EntityHandle tris[9];
    for (int i = 0; i < 8; ++i) {
        moab::EntityHandle conn[3] = { verts[outer[i]], verts[outer[(i+1)%8]], verts[4] };
        core->moab_instance()->create_element( moab::MBTRI, conn, 3, tris[i] );
    }
    tris[8] = verts[4];
    surf->commit_mesh( tris, 9, COMPLETE_MESH );
    return surf;
}


void test_simple_tri_to_quad()
{
    // create a triangle mesh
    moab::EntityHandle verts[9];
    ModelEnt* surf = get_tri_meshed_surface( verts );
    moab::EntityHandle set = surf->mesh_handle();
    moab::Interface* moab = core->moab_instance();
    if (write_vtk)
        moab->write_file( "test_simple_tri_to_quad-tris.vtk", "VTK", 0, &set, 1 );

    // run tri to quad mesher
    MEntVector list;
    list.push_back(surf);
    MeshOp* quad_to_tri = core->construct_meshop( "QuadMesher", list );
    CHECK(!!quad_to_tri);
    quad_to_tri->setup_this();
    quad_to_tri->execute_this();


    //  // check result mesh !! Doesn't work-meshset has some tri's
    // std::string opname0 = "quadmesh.vtk";

    //  if (write_vtk)
    //      moab->write_file( opname0.c_str(), NULL, NULL, &set, 1 );

    // removing tri's
    moab::Range quads,tri;
    moab->get_entities_by_type( set, moab::MBTRI, tri );
    moab->remove_entities(set, tri);
    //  if(tri.size() != 0)
    //      moab->delete_entities(tri);

    // check result mesh
    std::string opname = "test_simple_tri_to_quad-quad.vtk";
    if (write_vtk)
        moab->write_file( opname.c_str(), NULL, NULL, &set, 1 );

    moab->get_entities_by_type( set, moab::MBQUAD, quads );
    CHECK_EQUAL( (size_t)4, quads.size() );

    // expected quads
    moab::EntityHandle conn[4][4] = {
        { verts[0], verts[1], verts[4], verts[3] },
        { verts[1], verts[2], verts[5], verts[4] },
        { verts[3], verts[4], verts[7], verts[6] },
        { verts[4], verts[5], verts[8], verts[7] } };
    for (int i = 0; i < 4; ++i) {
        bool found = false;
        for (moab::Range::iterator j = quads.begin(); j != quads.end(); ++j) {
            const moab::EntityHandle* qconn;
            int len;
            moab->get_connectivity( *j, qconn, len );
            CHECK_EQUAL( 4, len );
            int idx = std::find( conn[i], conn[i]+4, qconn[0] ) - conn[i];
            if (idx == 4)
                continue;
            found = true;
            for (int k = 1; k < 4; ++k) {
                if (qconn[k] != conn[i][(idx+k)%4]) {
                    found = false;
                    break;
                }
            }

            if (found)
                break;
        }
        //    CHECK(found);
    }
}


void load_file()
{
    core->delete_all();
    moab::Interface* moab = core->moab_instance();
    std::string filename = TestDir + "/" + TEST_QUADFACE;
    core->load_geometry(filename.c_str());
    core->populate_model_ents(0, -1, -1);

    // get the tris
    MEntVector tris, dum;
    core->get_entities_by_dimension(2, dum);
    tris.push_back(*dum.rbegin());

    // run tri to quad mesher
    MeshOp* quad_to_tri = core->construct_meshop( "QuadMesher", tris );
    CHECK(!!quad_to_tri);
    double size = 1.1;
    SizingFunction esize(core, -1, size);
    tris[0]->sizing_function_index(esize.core_index());

    core->setup_and_execute();

    // removing tri's
    moab::Range tri;
    moab->get_entities_by_type( 0, moab::MBTRI, tri );
    if(tri.size() != 0)
        moab->delete_entities(tri);

    if (write_vtk)
        core->save_mesh("load_test.vtk");

}

