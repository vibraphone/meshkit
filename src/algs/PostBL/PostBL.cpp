#include "meshkit/PostBL.hpp"

namespace MeshKit
{

// static registration of this mesh scheme
moab::EntityType PostBL_tps[] = {moab::MBHEX,
                                 moab::MBMAXTYPE};
const moab::EntityType* PostBL::output_types()
{ return PostBL_tps; }

PostBL::PostBL( MKCore *mk, const MEntVector &me_vec)
    : MeshScheme( mk, me_vec),
      igeom(mk->igeom_instance()), imesh(mk->imesh_instance()),
      mb (mk->moab_instance())
    // ---------------------------------------------------------------------------
    //! Function: Constructor \n
    //! Input:    Initialize mesh and geometry instances and parameters \n
    //! Output:   none
    // ---------------------------------------------------------------------------
{
    tri_sch = 2;
    m_Conn = 0;
    m_BElemNodes = 0;
    m_SurfId = -1;
    check_bl_edge_length = false;
    debug = false;
    hybrid = false;
    m_NeumannSet = -1;
    m_Material = 999;
    m_nLineNumber = 0;
    szComment = "!";
    MAXCHARS = 300;
    m_JacCalls = 0;
    m_JLo = 0.0;
    m_JHi = 0.0;
    err = 0;
    fixmat = -1;
}

PostBL::~PostBL()
// ---------------------------------------------------------------------------
//! Function: Destructor, does nothing..\n
//! Input:    none \n
//! Output:   none \n
// ---------------------------------------------------------------------------
{}

bool PostBL::add_modelent(ModelEnt *model_ent)
// ---------------------------------------------------------------------------
//! Function: Adds entities for PosBL graph node.\n
//! Input:    ModelEnt \n
//! Output:   none \n
// ---------------------------------------------------------------------------
{
    return MeshOp::add_modelent(model_ent);
}

void PostBL::setup_this()
// ---------------------------------------------------------------------------
//! Function: Setup the graph node for PostBL \n
//! Input:    none \n
//! Output:   none \n
// ---------------------------------------------------------------------------
{
    if (debug) {
        m_LogFile <<  "\nIn setup this : " <<  std::endl;
    }

}

void PostBL::execute_this()
// ---------------------------------------------------------------------------
//! Function: Read user input from file and run the PostBL algorithm \n
//! Input:     Uses the file name (.inp) with keywords predefined by PosBL algorithm. \n
//! Output:    Resulting mesh file is saved. \n
// ---------------------------------------------------------------------------
{

    //////////////ALGORITHM BEGINS /////////////
    /// Set algorithm number
    int algo_no = 2;
    if(algo_no == 1){
        m_LogFile << "\nIn execute this : creating boundary layer elements.." <<  std::endl;
        // start the timer
        CClock Timer;
        clock_t sTime = clock();
        std::string szDateTime;
        Timer.GetDateTime (szDateTime);

        m_LogFile <<  "\nStarting out at : " << szDateTime << std::endl;
        m_LogFile <<  "\n Loading meshfile: " << m_MeshFile << ".." << std::endl;

        // load specified mesh file
        IBERRCHK(imesh->load(0, m_MeshFile.c_str(),0), *imesh);
        // m_GD = imesh->getGeometricDimension(); Doesn't work !
        moab::Range all_elems, all_verts;
        MBERRCHK(mb->get_entities_by_dimension(0, 3, all_elems,true),mb);
        if (all_elems.size() == 0)
            m_GD = 2;
        else if (all_elems.size() > 0)
            m_GD = 3;
        else
            exit(0);
        all_elems.clear();
        m_LogFile << "Geometric dimension of meshfile = "<< m_GD <<std::endl;

        // obtain existing tag handles
        moab::Tag GDTag, GIDTag, NTag, MTag; //, STag, FTag, MNTag
        MBERRCHK(mb->tag_get_handle("GEOM_DIMENSION", 1, moab::MB_TYPE_INTEGER, GDTag),mb);
        MBERRCHK(mb->tag_get_handle("NEUMANN_SET", 1, moab::MB_TYPE_INTEGER, NTag),mb);
        MBERRCHK(mb->tag_get_handle("MATERIAL_SET", 1, moab::MB_TYPE_INTEGER, MTag),mb);
        MBERRCHK(mb->tag_get_handle("GLOBAL_ID", 1, moab::MB_TYPE_INTEGER, GIDTag),mb);
        // create smoothset and fixed tag for mesquite
        //        MBERRCHK(mb->tag_get_handle("SMOOTHSET", 1, moab::MB_TYPE_INTEGER, STag,
        //                                    moab::MB_TAG_SPARSE|moab::MB_TAG_CREAT),mb);
        //        MBERRCHK(mb->tag_get_handle("fixed", 1, moab::MB_TYPE_INTEGER, FTag,
        //                                    moab::MB_TAG_SPARSE|moab::MB_TAG_CREAT),mb);
        //        MBERRCHK(mb->tag_get_handle("mnode", 1, moab::MB_TYPE_INTEGER, MNTag,
        //                                    moab::MB_TAG_SPARSE|moab::MB_TAG_CREAT),mb);

        // get all the entity sets with boundary layer geom dimension, neumann sets and material sets
        moab::Range sets, n_sets, m_sets;
        m_BLDim = m_GD - 1;

        const void* gdim[] = {&m_BLDim};

        MBERRCHK(mb->get_entities_by_type_and_tag(0, moab::MBENTITYSET, &GDTag,
                                                  gdim, 1 , sets, moab::Interface::INTERSECT, false), mb);
        MBERRCHK(mb->get_entities_by_type_and_tag(0, moab::MBENTITYSET, &NTag, 0, 1 , n_sets),mb);
        MBERRCHK(mb->get_entities_by_type_and_tag(0, moab::MBENTITYSET, &MTag, 0, 1 , m_sets),mb);

        // Handling NeumannSets (if BL surf in input via NS)
        moab::Range::iterator set_it;
        moab::EntityHandle this_set = 0;
        for (set_it = n_sets.begin(); set_it != n_sets.end(); set_it++)  {

            this_set = *set_it;

            // get entity handle of NS specified in the input file
            int set_id;
            MBERRCHK(mb->tag_get_data(NTag, &this_set, 1, &set_id), mb);
            if(set_id == m_NeumannSet)
                break;
            this_set = 0;
        }
        if (debug && m_NeumannSet != -1 && this_set != 0){
            m_LogFile <<  "Looking for NS with id " << m_NeumannSet <<
                          ". Total NS found are: "<< n_sets.size() << std::endl;
        }

        // For specified surface: get the  all the quads and nodes in a range
        moab::EntityHandle s1;
        moab::Range quads, nodes, fixmat_ents;
        int dims; // variable to store global id of boundary layer specified in the input file

        // Method 1: INPUT by NeumannSet
        if(m_NeumannSet != -1 && this_set != 0){
            MBERRCHK(mb->get_entities_by_dimension(this_set, m_BLDim, quads,true),mb);
            if (quads.size() <=0){
                m_LogFile <<  " No quads found, aborting.. " << std::endl;
                exit(0);
            }

            if(mb->type_from_handle(*quads.rbegin()) != MBQUAD){
                m_Conn = 4;
                m_BElemNodes = 3;
            }

            MBERRCHK(mb->get_adjacencies(quads, 0, false, nodes, moab::Interface::UNION),mb);
            if (debug) {
                m_LogFile <<  "Found NeumannSet with id : " << m_NeumannSet <<  std::endl;
                m_LogFile <<  "#Quads in this surface: " << quads.size() << std::endl;
                m_LogFile <<  "#Nodes in this surface: " << nodes.size() << std::endl;
                m_LogFile << "#New nodes to be created:" << m_Intervals*nodes.size() << std::endl;
            }
        }
        // Method 2: INPUT by surface id (geom dimension)
        else if (m_SurfId !=-1){
            for(moab::Range::iterator rit=sets.begin(); rit != sets.end(); ++rit){
                s1 = *rit;
                MBERRCHK(mb->tag_get_data(GIDTag, &s1, 1, &dims),mb);

                if(dims == m_SurfId && m_SurfId != -1){
                    MBERRCHK(mb->get_entities_by_dimension(s1, m_BLDim, quads,true),mb);
                    if (quads.size() <=0){
                        m_LogFile <<  " No quads found, aborting.. " << std::endl;
                        exit(0);
                    }

                    MBERRCHK(mb->get_adjacencies(quads, 0, false, nodes, moab::Interface::UNION),mb);
                    if (debug) {
                        m_LogFile <<  "Found surface with id : " << m_SurfId <<  std::endl;
                        m_LogFile <<  "#Quads in this surface: " << quads.size() << std::endl;
                        m_LogFile <<  "#Nodes in this surface: " << nodes.size() << std::endl;
                        m_LogFile << "#New nodes to be created:" << m_Intervals*nodes.size() << std::endl;
                    }
                }
            }
        }

        if (quads.size() == 0 || nodes.size() == 0) {
            m_LogFile <<  "Invalid boundary layer specification, aborting.." <<  std::endl;
            exit(0);
        }

        // set fixed tag on all the BL nodes
        //        MBERRCHK(mb->get_entities_by_dimension(0, 0, all_verts, true),mb);
        //        std::vector<int> all_node_data(all_verts.size(), 0);
        //        MBERRCHK(mb->tag_set_data(FTag, all_verts, &all_node_data[0]), mb);

        // Handling MaterialSet
        moab::Range::iterator mset_it;
        moab::EntityHandle mthis_set;
        int mset_id = 0, found = 0;
        for (mset_it = m_sets.begin(); mset_it != m_sets.end(); mset_it++)  {

            mthis_set = *mset_it;

            // get entity handle of MS specified in the input file
            MBERRCHK(mb->tag_get_data(MTag, &mthis_set, 1, &mset_id), mb);
            if(mset_id == m_Material){
                found = 1;
                break;
            }
            else if(mset_id == fixmat){
                MBERRCHK(mb->get_entities_by_dimension(mthis_set, m_GD, fixmat_ents ,true),mb);
            }


            //            // get all the nodes in the material and tag bl nodes
            //            moab::Range mat_nodes, mat_hexes;
            //            if(m_GD == 3)
            //              MBERRCHK(mb->get_entities_by_type(mthis_set, moab::MBHEX, mat_hexes, true), mb);
            //            MBERRCHK(mb->get_adjacencies(mat_hexes, 0, false, mat_nodes, Interface::UNION), mb);

            //            moab::Range mat_b_nodes = intersect(nodes, mat_nodes);
            //            std::cout << "MAT NO:" << mset_id << " has " << mat_hexes.size() << "HEXES " <<  mat_nodes.size() << "NODES AND "
            //                      << mat_b_nodes.size() << " NODES ON BOUNDARY" << std::endl;

            //            std::vector<int> bl_node_data(mat_b_nodes.size(), 0);
            //            std::vector<int> node_tag_data(mat_b_nodes.size(),-1);
            //            // don't error check, as it is supposed to give error when multiple material case is encountered
            //            mb->tag_get_data(MNTag, mat_b_nodes, &node_tag_data[0]);
            //            for(int i=0; i<mat_b_nodes.size(); i++){
            //                std::cout << node_tag_data[i] << std::endl;
            //                // already a part of some material
            //                if(node_tag_data[i] != -1){
            //                    bl_node_data[i] = node_tag_data[i]+1;
            //                  }
            //              }
            //            MBERRCHK(mb->tag_set_data(MNTag, mat_b_nodes, &bl_node_data[0]), mb);
            //            mat_hexes.clear();
            //            mat_b_nodes.clear();
            //            mat_nodes.clear();
            mthis_set = 0;
        }
        // get tag data and print
        //        std::vector<int> all_bl(nodes.size(),-100);
        //        mb->tag_get_data(MNTag, nodes, &all_bl[0]);
        //        for(int i=0; i<nodes.size(); i++){
        //            std::cout << " Node " << i << " tag value " << all_bl[i] << std::endl;
        //          }
        if(found == 1 && m_Material !=999){
            m_LogFile << "Found material set with id " << m_Material << std::endl;
        }
        else{
            // No material set found, creating material set 999 for BL elements
            m_LogFile << "Creating material set with id " << m_Material << std::endl;
            MBERRCHK(mb->create_meshset(moab::MESHSET_SET, mthis_set, 1), mb);
            MBERRCHK(mb->tag_set_data(MTag, &mthis_set, 1, &m_Material), mb);
        }

        //        // placeholder for storing smoothing entities
        //        moab::EntityHandle smooth_set;
        //        int s_id = 100;
        //        MBERRCHK(mb->create_meshset(moab::MESHSET_SET, smooth_set, 1), mb);
        //        MBERRCHK(mb->tag_set_data(STag, &smooth_set, 1, &s_id), mb);

        // placeholder for storing gd on new entities
        moab::EntityHandle geom_set;
        MBERRCHK(mb->create_meshset(moab::MESHSET_SET, geom_set, 1), mb);
        MBERRCHK(mb->tag_set_data(GDTag, &geom_set, 1, &m_GD), mb);

        // declare variables before starting BL creation
        std::vector <bool> node_status(false); // size of verts of bl surface
        node_status.resize(nodes.size());
        moab::Range edges, hexes, hex_edge, quad_verts;
        double coords_bl_quad[3], coords_new_quad[3], xdisp = 0.0, ydisp = 0.0, zdisp = 0.0;
        moab::EntityHandle hex, hex1;
        int qcount = 0;

        //size of the following is based on element type
        std::vector<moab::EntityHandle> conn, qconn, adj_qconn, tri_conn,
                new_vert(m_Intervals*nodes.size()), old_hex_conn, adj_hexes, adj_quads, adj_hex_nodes1;
        moab::CartVect surf_normal(3);

        // Now start creating New elements
        for (Range::iterator kter = quads.begin(); kter != quads.end(); ++kter){
            qcount++;

            if (debug){
                m_LogFile <<  "\n\n*** QUAD: " << qcount << std::endl;
            }

            std::vector<moab::EntityHandle> old_hex;
            MBERRCHK(mb->get_adjacencies(&(*kter), 1, m_GD, false, old_hex),mb);
            if((int) old_hex.size() == 0){
                m_LogFile << "unable to find adjacent hex for BL quad, aborting...";
                exit(0);
            }

            // if fixmat specified, filter old hex, we don't have to correct both sides of the boundary
            if (fixmat !=0 && (int) old_hex.size() > 1){
                moab::EntityHandle old_hex_set;
                MBERRCHK(mb->create_meshset(moab::MESHSET_SET, old_hex_set, 1), mb);
                MBERRCHK(mb->add_entities(old_hex_set,&old_hex[0], (int) old_hex.size()), mb);
                // TODO: Find a faster way of doing this
                MBERRCHK(mb->remove_entities(old_hex_set, fixmat_ents), mb);
                old_hex.clear();
                old_hex.empty();
                MBERRCHK(mb->get_entities_by_dimension(old_hex_set, m_GD, old_hex), mb);
            }
            // allocate space for connectivity/adjacency during the first pass of this loop
            if(qcount ==1){
                if(mb->type_from_handle(old_hex[0]) == MBHEX){
                    m_Conn = 8;
                    m_BElemNodes = 4;
                    m_HConn = 8;
                    //allocating based on element type
                    conn.resize(m_Intervals*m_Conn), qconn.resize(m_BElemNodes), adj_qconn.resize(m_BElemNodes),
                            old_hex_conn.resize(m_Conn), adj_hex_nodes1.resize(m_Conn);
                }
                else if(mb->type_from_handle(old_hex[0]) == MBTET){
                    m_Conn = 4;
                    m_HConn = 6;
                    m_BElemNodes = 3;
                    //allocating based on element type - thrice the number of elements
                    if(hybrid)
                        conn.resize(m_Intervals*6);
                    else
                        conn.resize(2*m_Intervals*m_Conn);
                    qconn.resize(m_BElemNodes), adj_qconn.resize(m_BElemNodes),
                            old_hex_conn.resize(m_Conn), adj_hex_nodes1.resize(m_Conn);
                }
                else if(mb->type_from_handle(old_hex[0]) == MBQUAD){
                    m_Conn = 4;
                    m_HConn = 4;
                    m_BElemNodes = 2;
                    //allocating based on element type
                    conn.resize(m_Intervals*m_Conn), qconn.resize(m_BElemNodes), adj_qconn.resize(m_BElemNodes),
                            old_hex_conn.resize(m_Conn), adj_hex_nodes1.resize(m_Conn);
                }
                else if(mb->type_from_handle(old_hex[0]) == MBTRI){
                    m_Conn = 3;
                    m_HConn = 4;
                    m_BElemNodes = 2;
                    //allocating based on element type - twice the number of elements
                    if(hybrid){
                        m_HConn = 4;
                        conn.resize(m_Intervals*m_HConn);
                    }
                    else{
                        tri_conn.resize(2*m_Intervals*m_Conn);
                        conn.resize(2*m_Intervals*m_Conn);
                    }
                    qconn.resize(m_BElemNodes), adj_qconn.resize(m_BElemNodes),
                            old_hex_conn.resize(m_Conn), adj_hex_nodes1.resize(m_Conn);
                }
                else if(m_Conn == 0 || m_BElemNodes == 0){
                    m_LogFile << "MeshType is not supported by this tool" << std::endl;
                    exit(0);
                }
            }
            qconn.clear();
            old_hex_conn.clear();
            MBERRCHK(mb->get_connectivity(&(*kter), 1, qconn),mb);
            MBERRCHK(mb->get_connectivity(&old_hex[0], 1, old_hex_conn),mb);

            // get the normal to the plane of the 2D mesh
            if(m_GD==2 && qcount == 1)
                get_normal_quad (old_hex_conn, surf_normal);
            for (int i=0; i<m_BElemNodes; i++){
                MBERRCHK(mb->get_coords(&qconn[i], 1, coords_bl_quad),mb);

                // check to see if this node is already dealt with
                int blNodeId = 0;
                for(int n=0; n< (int) nodes.size(); n++){
                    if(nodes[n] == qconn[i]){
                        blNodeId = n;
                        break;
                    }
                    else if(n == (int) nodes.size()){
                        m_LogFile << "QUAD doesn't have a node in BL NODES, aborting.." << std::endl;
                        exit(0);
                    }
                }
                if (debug){
                    m_LogFile <<  "\n*Working on Node: " << blNodeId << " coords: " << coords_bl_quad[0]
                               << ", " << coords_bl_quad[1] << ", " << coords_bl_quad[2]  << std::endl;
                }
                double temp;
                //create new nodes
                if(node_status[blNodeId] == false){
                    adj_hexes.clear();
                    MBERRCHK(mb->get_adjacencies(&qconn[i], 1, m_GD, false, adj_hexes, moab::Interface::UNION), mb);
                    MBERRCHK(mb->get_adjacencies(&qconn[i], 1, m_BLDim, false, adj_quads, moab::Interface::UNION), mb);

                    // if fixmat specified, filter old hex, we don't have to correct both sides of the boundary
                    if (fixmat !=0 && (int) adj_hexes.size() > 1){
                        moab::EntityHandle adj_hex_set;
                        MBERRCHK(mb->create_meshset(moab::MESHSET_SET, adj_hex_set, 1), mb);
                        MBERRCHK(mb->add_entities(adj_hex_set,&adj_hexes[0], (int) adj_hexes.size()), mb);
                        MBERRCHK(mb->remove_entities(adj_hex_set, fixmat_ents), mb);
                        adj_hexes.clear();
                        adj_hexes.empty();
                        MBERRCHK(mb->get_entities_by_dimension(adj_hex_set, m_GD, adj_hexes), mb);
                    }

                    int side_number = 0, sense = 1, offset = 0;
                    MBERRCHK(mb->side_number(old_hex[0], (*kter), side_number, sense, offset), mb);

                    moab::CartVect rt(0.0, 0.0, 0.0), v(3);
                    moab::Range adj_qconn_r;
                    // TODO: Add feature to add element on both sides of the boundary layer
                    // find the normal direction where new nodes are to be created - xdisp, ydisp and zdisp
                    for (int q=0; q < (int) quads.size(); q++){
                        for(int r=0; r < (int) adj_quads.size(); r++){
                            if (adj_quads[r] == quads[q]){
                                // it's a BL quad, get the normal and prepare to compute average
                                MBERRCHK(mb->get_connectivity(&adj_quads[r], 1, adj_qconn),mb);
                                if(m_GD==3){
                                    get_normal_quad (adj_qconn, v);
                                    if (sense == -1)
                                        v=-v;
                                }
                                else if(m_GD==2){
                                    if(sense == 1)
                                        get_normal_edge(adj_qconn, surf_normal, v);
                                    else
                                        get_normal_edge(adj_qconn, -surf_normal, v);
                                }
                                //TODO: Check to make sure that normal is inwards
                                rt = rt + v;
                                xdisp=rt[0]/rt.length();
                                ydisp=rt[1]/rt.length();
                                zdisp=rt[2]/rt.length();
                            }
                            else{
                                // it's not a BL quad, instead of shooting the normal find the max. BL distance allowable
                                if(check_bl_edge_length){
                                    MBERRCHK(mb->get_connectivity(&adj_quads[r],1,adj_qconn_r), mb);
                                    find_min_edge_length(adj_qconn_r, qconn[i], nodes, m_MinEdgeLength);
                                }
                            }
                            adj_qconn_r.clear();
                        }
                    }
                    // TODO: shoot normal in direction xdisp, yd.. from point coords_bl_quad
                    // and find the point of intersection and distance with existing mesh
                    // range nodes, coords_bl_quad and the nodes of the hex of interest here is old_hex_conn

                    // Half check of thickness validity
                    if(m_Thickness > m_MinEdgeLength && m_MinEdgeLength > 0){
                        // This BL creation might fail
                        m_LogFile << "Specified thickess is " << m_Thickness
                                  << " and BL elements edge length is " << m_MinEdgeLength
                                  << "  BL creation might fail, "
                                  << "\n Set check_bl_edge_length to false to avoid this check, aborting.. " << std::endl;
                        exit(0);
                    }


                    adj_quads.clear();
                    double num = m_Thickness*(m_Bias-1)*(pow(m_Bias, m_Intervals -1));
                    double deno = pow(m_Bias, m_Intervals) - 1;
                    if (deno !=0)
                        temp = num/deno;
                    else
                        temp = m_Thickness/m_Intervals;

                    // created BL nodes
                    double move = 0.0;
                    for(int j=0; j< m_Intervals; j++){

                        move+= temp/pow(m_Bias,j);
                        if (debug){
                            m_LogFile <<  " move:" << move;
                        }
                        // now compute the coords of the new vertex
                        coords_new_quad[0] = coords_bl_quad[0]-move*xdisp;
                        coords_new_quad[1] = coords_bl_quad[1]-move*ydisp;
                        coords_new_quad[2] = coords_bl_quad[2]-move*zdisp;

                        int nid = blNodeId*m_Intervals+j;
                        // Possible TODO's
                        //TODO: Check if this vertex is possible (detect possible collision with geometry)
                        // TODO: See posibility of using ray tracing
                        // TODO: Parallize: Profile T-junction model and try to device an algorithm
                        // TODO: Modularize node creation part and use doxygen for all code and design of code, python design and test cases - current functions in code:
                        // Setup this, Execute this -- break info sub functions and classes,
                        // prepareIO --make this optional when using python,
                        // get normal (2d and 3d) -- can be combined to one function
                        // get det jacobian (hex elements) --needs check for other elements
                        //
                        MBERRCHK(mb->create_vertex(coords_new_quad, new_vert[nid]), mb);
                        if (debug){
                            m_LogFile << std::setprecision (3) << std::scientific << " : created node:" << (nid + 1)
                                      << " of " << new_vert.size() << " new nodes:- coords: " << coords_new_quad[0]
                                      << ", " << coords_new_quad[1] << ", " << coords_new_quad[2]  << std::endl;
                        }
                    }
                    node_status[blNodeId] = true;
                }

                //populate the connectivity after creating nodes for this BL node
                for(int j=0; j< m_Intervals; j++){
                    if(m_Conn == 8 && m_BElemNodes == 4){
                        int nid = blNodeId*m_Intervals+j;
                        if(j==0) // set connectivity of boundary layer hex
                            conn[m_Conn*j + i+m_BElemNodes] = qconn[i];
                        else
                            conn[m_Conn*j + i+m_BElemNodes] = new_vert[nid-1];
                        conn[m_Conn*j +i] = new_vert[nid];
                    }
                    else if(m_Conn == 4 && m_BElemNodes == 2){
                        int nid = blNodeId*m_Intervals+j;
                        if(j==0) // set connectivity of boundary layer hex
                            conn[m_Conn*j + i+m_BElemNodes] = qconn[m_BElemNodes-i-1];
                        else
                            conn[m_Conn*j + m_BElemNodes + 1 - i] = new_vert[nid-1];
                        conn[m_Conn*j +i] = new_vert[nid];
                    }
                    else if(m_Conn == 4 && m_BElemNodes == 3 && hybrid == true){ // make wedges aka prisms for tet mesh
                        int nid = blNodeId*m_Intervals+j;
                        if(j==0) // set connectivity of boundary layer hex
                            conn[m_HConn*j + i+m_BElemNodes] = qconn[i];
                        else
                            conn[m_HConn*j + i+m_BElemNodes] = new_vert[nid-1];
                        conn[m_HConn*j +i] = new_vert[nid];
                    }
                    else if(m_Conn == 3 && m_BElemNodes == 2){ // make quads for tri mesh
                        int nid = blNodeId*m_Intervals+j;
                        if(j==0) // set connectivity of boundary layer hex
                            conn[m_HConn*j + i+m_BElemNodes] = qconn[m_BElemNodes-i-1];
                        else
                            conn[m_HConn*j + m_BElemNodes + 1 - i] = new_vert[nid-1];
                        conn[m_HConn*j +i] = new_vert[nid];
                    }

                    // Another block for setting up fixed tag for smoothing using Mesquite
                    //                    int fix0 = 0;
                    //                    int nid = blNodeId*m_Intervals+j;
                    //  MBERRCHK(mb->tag_set_data(FTag, &new_vert[nid], 1, &fix0), mb);
                }

                // if a hex does have a quad on BL, but, has a node or edge on the boundary layer
                for(int k=0; k < (int) adj_hexes.size(); k++){
                    adj_hex_nodes1.clear();
                    MBERRCHK(mb->get_connectivity(&adj_hexes[k], 1, adj_hex_nodes1), mb);
                    std::vector<EntityHandle> inodes;
                    int var = 0;
                    for(int n = 0; n < (int) nodes.size(); n++){
                        for(int m = 0; m < m_Conn; m++){
                            if(nodes[n] == adj_hex_nodes1[m]){
                                inodes.push_back(adj_hex_nodes1[m]);
                                var++;
                            }
                        }
                    }
                    // doesn't have a quad or when inodes is 0 it mean this is a newly created hex
                    if((int) inodes.size() != m_BElemNodes && (int) inodes.size() != 0){
                        if(debug){
                            m_LogFile << "Hex on BL surface is connected by a node or edge" <<
                                         "\n nodes on BL - " << inodes.size() << std::endl;
                        }
                        // mark this hex and set it's connectivity later
                        for(int p = 0; p < m_Conn; p++){
                            if(qconn[i] == adj_hex_nodes1[p]){
                                adj_hex_nodes1[p] = conn[m_HConn*(m_Intervals-1)+i];
                            }
                        }
                        // check to see if this node position change also changes lower dimension entities
                        std::vector<EntityHandle> adj1, adj2;
                        EntityHandle one = qconn[i];
                        EntityHandle two = adj_hexes[k];
                        std::vector<EntityHandle> from_entities;
                        from_entities.push_back(one);
                        from_entities.push_back(two);
                        if(m_BLDim == 2){
                            MBERRCHK(mb->get_adjacencies(&from_entities[0], 2, 1, false, adj1), mb);
                            MBERRCHK(mb->delete_entities(&adj1[0], (int) adj1.size()), mb);
                        }
                        MBERRCHK(mb->get_adjacencies(&from_entities[0], 2, m_BLDim, false, adj2), mb);
                        MBERRCHK(mb->delete_entities(&adj2[0], (int) adj2.size()), mb);
                        MBERRCHK(mb->set_connectivity(adj_hexes[k], &adj_hex_nodes1[0], m_Conn), mb);
                    }
                    double j_ahex = 0.0;
                    get_det_jacobian(adj_hex_nodes1, 0, j_ahex);
                    // mark entities for smoothing
                    //         MBERRCHK(mb->add_entities(smooth_set, &adj_hexes[k], 1), mb);
                    inodes.clear();
                }
            } // Loop thru BL element nodes ends

            //TODO: Set Connectivity of tet's, break prisms into 3 tets, Another loop is required.
            if(m_Conn == 3 && m_BElemNodes == 2 && hybrid == false){
                for(int c=0; c<m_Intervals; c++){
                    if(tri_sch == 1){
                        // lower triangle
                        tri_conn[m_Conn*c*2] =     conn[c*m_HConn];
                        tri_conn[m_Conn*c*2 + 1] = conn[c*m_HConn + 1];
                        tri_conn[m_Conn*c*2 + 2] = conn[c*m_HConn + 3];
                        // upper triangle
                        tri_conn[m_Conn*c*2 + 3] = conn[c*m_HConn + 1];
                        tri_conn[m_Conn*c*2 + 4] = conn[c*m_HConn + 2];
                        tri_conn[m_Conn*c*2 + 5] = conn[c*m_HConn + 3];
                    }
                    else if(tri_sch == 2){
                        // lower triangle
                        tri_conn[m_Conn*c*2] =     conn[c*m_HConn];
                        tri_conn[m_Conn*c*2 + 1] = conn[c*m_HConn + 1];
                        tri_conn[m_Conn*c*2 + 2] = conn[c*m_HConn + 2];
                        // upper triangle
                        tri_conn[m_Conn*c*2 + 3] = conn[c*m_HConn + 2];
                        tri_conn[m_Conn*c*2 + 4] = conn[c*m_HConn + 3];
                        tri_conn[m_Conn*c*2 + 5] = conn[c*m_HConn];
                    }
                }
            }

            if (debug)
            {
                m_LogFile <<  "\nsetting connectivity of the old BL hex " << std::endl;
            }
            // First replace BL nodes (part of old hex) with newly created nodes, then set connectivity of the old_hex
            // deleting old connectivities
            //TODO: Check to see if this step can be avoided for some cases
            for(int p=0; p<m_Conn; p++){
                for(int q=0; q<m_BElemNodes; q++){
                    if (old_hex_conn[p] == qconn[q]){
                        old_hex_conn[p] = conn[m_HConn*(m_Intervals-1) + q];
                        // check to see if this node position change also changes lower dimension entities
                        std::vector<EntityHandle> adj1, adj2;
                        EntityHandle one = qconn[q];
                        EntityHandle two = old_hex[0];
                        std::vector<EntityHandle> from_entities;
                        from_entities.push_back(one);
                        from_entities.push_back(two);
                        if(m_BLDim == 2){
                            MBERRCHK(mb->get_adjacencies(&from_entities[0], 2, 1, false, adj1), mb);
                            MBERRCHK(mb->delete_entities(&adj1[0], (int) adj1.size()), mb);
                        }
                        MBERRCHK(mb->get_adjacencies(&from_entities[0], 2, m_BLDim, false, adj2), mb);
                        MBERRCHK(mb->delete_entities(&adj2[0], (int) adj2.size()), mb);
                    }
                }
            }
            double j_old_hex = 0.0;
            get_det_jacobian(old_hex_conn, 0, j_old_hex);
            MBERRCHK(mb->set_connectivity(old_hex[0], &old_hex_conn[0], m_Conn), mb);
            old_hex.clear();

            // mark entities for smoothing
            //        MBERRCHK(mb->add_entities(smooth_set, &old_hex[0], 1), mb);

            if (debug){
                m_LogFile <<  "creating new boundary layer hexes" << std::endl;
            }
            // create boundary layer hexes
            for(int j=0; j< m_Intervals; j++){
                double j_hex = 0.0;
                get_det_jacobian(conn, j*m_Conn, j_hex);
                if(m_Conn == 8){
                    MBERRCHK(mb->create_element(MBHEX, &conn[j*m_Conn], m_Conn, hex),mb);
                }
                else if(m_Conn==4 && m_GD ==3 && hybrid == true){
                    MBERRCHK(mb->create_element(MBPRISM, &conn[j*6], 6, hex),mb);
                }
                else if(m_Conn==4 && m_GD ==2){
                    MBERRCHK(mb->create_element(MBQUAD, &conn[j*m_Conn], m_Conn, hex),mb);
                }
                else if(m_Conn==3 && m_GD ==2 && hybrid == true){
                    MBERRCHK(mb->create_element(MBQUAD, &conn[j*m_HConn], m_HConn, hex),mb);
                }
                else if(m_Conn==3 && m_GD ==2 && hybrid == false){
                    MBERRCHK(mb->create_element(MBTRI, &tri_conn[j*m_Conn*2], m_Conn, hex),mb);
                    MBERRCHK(mb->create_element(MBTRI, &tri_conn[j*m_Conn*2+3], m_Conn, hex1),mb);
                    MBERRCHK(mb->add_entities(mthis_set, &hex1, 1), mb);
                    //        MBERRCHK(mb->add_entities(smooth_set, &hex1, 1), mb);
                }
                // add this hex to a block
                MBERRCHK(mb->add_entities(mthis_set, &hex, 1), mb);
                // mark entities for smoothing
                //                MBERRCHK(mb->add_entities(smooth_set, &hex, 1), mb);
                // add geom dim tag
                MBERRCHK(mb->add_entities(geom_set, &hex, 1), mb);
                // TODO: Add Local Smooting
            }
        } // Loop thru quads ends

        // TODO: Global smoothing works after PostBL step; make global smoothing available as an option inside
        // get the skin of the entities
        //        moab::Range skin_verts;
        //        all_elems.clear();
        //        MBERRCHK(mb->get_entities_by_dimension(0, 3, all_elems,true),mb);

        //        moab::Skinner skinner(mb);
        //        skinner.find_skin(all_elems, 0, skin_verts);

        //        m_LogFile << "setting 'fixed'' tag = 1 on verts in the skin = " <<  skin_verts.size() << std::endl;

        //        // set fixed tag = 1 on all skin verts
        //        std::vector<int> all_skin_data(skin_verts.size(), 1);
        //        MBERRCHK(mb->tag_set_data(FTag, skin_verts, &all_skin_data[0]), mb);

        m_LogFile << "\nTotal Jacobian calls/Min/Max: " << m_JacCalls << ", " << m_JLo << ", " << m_JHi << std::endl;

        // save the final boundary layer mesh
        MBERRCHK(mb->write_mesh(m_OutFile.c_str()),mb);
        m_LogFile <<  "\n\nWrote Mesh File: " << m_OutFile << std::endl;
        // get the current date and time
        Timer.GetDateTime (szDateTime);
        m_LogFile << "Ending at : " << szDateTime;
        // report/compute the elapsed time
        m_LogFile <<  "Elapsed wall clock time: " << Timer.DiffTime ()
                   << " seconds or " << (Timer.DiffTime ())/60.0 << " mins\n";
        m_LogFile <<  "Total CPU time used: " << (double) (clock() - sTime)/CLOCKS_PER_SEC \
                   << " seconds" << std::endl;
    }
    else{
        Algo2();
    }
}

void PostBL::PrepareIO (int argc, char *argv[], std::string  TestDir)
// ---------------------------------------------------------------------------
//! Function: Parser for reading the PostBL specification (.inp) file. \n
//! Input:    Command line arguments. \n
//! Output:   none \n
// ---------------------------------------------------------------------------
{
    // set and open input output files
    bool bDone = false;
    do{
        if (2 == argc) {
            m_InputFile = argv[1];
            m_LogName = m_InputFile + ".log";
        }
        else if (1 == argc){
            m_LogFile << "\nRunning default case:\n" << std::endl;

            m_InputFile = TestDir + "/" + (char *)DEFAULT_TEST_POSTBL;
            m_LogName = m_InputFile + ".log";
        }

        // open input file for reading
        m_FileInput.open (m_InputFile.c_str(), std::ios::in);
        if (!m_FileInput){
            m_LogFile << "Unable to open file: " << m_InputFile << std::endl;
            m_FileInput.clear ();
            exit(1);
        }
        else
            bDone = true; // file opened successfully

        // open the log file for dumping debug/output statements
        m_LogFile.coss.open (m_LogName.c_str(), std::ios::out);
        if (!m_LogFile.coss){
            m_LogFile <<  "Unable to open file: " << m_LogName << std::endl;
            m_LogFile.coss.clear ();
            exit(1);
        }
        else
            bDone = true; // file opened successfully
        m_LogFile <<  '\n';
        m_LogFile <<  "\t\t---------------------------------------------------------" << '\n';
        m_LogFile <<  "\t\t         Tool to generate Post-mesh Boundary Layers      " << '\n';
        m_LogFile <<  "\t\t\t\tArgonne National Laboratory" << '\n';
        m_LogFile <<  "\t\t\t\t        2012         " << '\n';
        m_LogFile <<  "\t\t---------------------------------------------------------" << '\n';
        m_LogFile <<  "\nsee README file for using the program and details on various cards.\n"<< std::endl;

    }while (!bDone);

    // Get the meshfile name, surface(s), thickness, intervals and bias
    CParser Parse;

    // count the total number of cylinder commands in each pincellh
    for(;;){
        if (!Parse.ReadNextLine (m_FileInput, m_nLineNumber, szInputString,
                                 MAXCHARS, szComment))
            IOErrorHandler (INVALIDINPUT);

        // Get tri scheme
        if (szInputString.substr(0,9) == "trischeme"){
            std::istringstream szFormatString (szInputString);
            szFormatString >> m_Card >> tri_sch;
            if(szFormatString.fail())
                IOErrorHandler(INVALIDINPUT);
            m_LogFile <<  m_Card << " name read: "<< tri_sch << std::endl;
        }
        // Get hybrid
        if (szInputString.substr(0,6) == "hybrid"){
            std::istringstream szFormatString (szInputString);
            szFormatString >> m_Card >> hybrid;
            if(szFormatString.fail())
                IOErrorHandler(INVALIDINPUT);
            m_LogFile <<  m_Card << " name read: "<< hybrid << std::endl;
        }
        // Get hybrid
        if (szInputString.substr(0,6) == "fixmat"){
            std::istringstream szFormatString (szInputString);
            szFormatString >> m_Card >> fixmat;
            if(szFormatString.fail())
                IOErrorHandler(INVALIDINPUT);
            m_LogFile <<  m_Card << " name read: "<< fixmat << std::endl;
        }
        // Get MeshFile name
        if (szInputString.substr(0,8) == "meshfile"){
            std::istringstream szFormatString (szInputString);
            szFormatString >> m_Card >> m_MeshFile;
            if(szFormatString.fail())
                IOErrorHandler(INVALIDINPUT);
            m_LogFile <<  m_Card << " name read: "<< m_MeshFile << std::endl;
            if (argc == 1){
                m_MeshFile = TestDir + "/" + m_MeshFile;
            }
        }
        // Get BL surface
        if (szInputString.substr(0,8) == "surfaces"){
            std::istringstream szFormatString (szInputString);
            szFormatString >> m_Card >> m_SurfId;
            if(m_SurfId < 0 || szFormatString.fail())
                IOErrorHandler(INVALIDINPUT);
            m_LogFile <<  m_Card << " read: "<< m_SurfId <<std::endl;
        }
        // Get BL surface via neumann set or sideset
        if (szInputString.substr(0,10) == "neumannset"){
            std::istringstream szFormatString (szInputString);
            szFormatString >> m_Card >> m_NeumannSet;
            if(m_NeumannSet < 0 || szFormatString.fail())
                IOErrorHandler(INVALIDINPUT);
            m_LogFile <<  m_Card << " read: "<< m_NeumannSet <<std::endl;
        }
        // Get BL material (block) number
        if (szInputString.substr(0,8) == "material"){
            std::istringstream szFormatString (szInputString);
            szFormatString >> m_Card >> m_Material;
            if(m_Material < 0 || szFormatString.fail())
                IOErrorHandler(INVALIDINPUT);
            m_LogFile <<  m_Card << " read: "<< m_Material <<std::endl;
        }

        // Get thickness
        if (szInputString.substr(0,9) == "thickness"){
            std::istringstream szFormatString (szInputString);
            szFormatString >> m_Card >> m_Thickness;
            if(m_Thickness < 0 || szFormatString.fail())
                IOErrorHandler(INVALIDINPUT);
            m_LogFile <<  m_Card << " read: "<< m_Thickness <<std::endl;
        }
        // Get intervals
        if (szInputString.substr(0,9) == "intervals"){
            std::istringstream szFormatString (szInputString);
            szFormatString >> m_Card >> m_Intervals;
            if(m_Intervals < 0 || szFormatString.fail())
                IOErrorHandler(INVALIDINPUT);
            m_LogFile <<  m_Card << " read: "<< m_Intervals <<std::endl;
        }
        // Get bias
        if (szInputString.substr(0,4) == "bias"){
            std::istringstream szFormatString (szInputString);
            szFormatString >> m_Card >> m_Bias;
            if(m_Bias < 0 || szFormatString.fail())
                IOErrorHandler(INVALIDINPUT);
            m_LogFile <<  m_Card << " read: "<< m_Bias <<std::endl;
        }
        // Output file name
        if (szInputString.substr(0,7) == "outfile"){
            std::istringstream szFormatString (szInputString);
            szFormatString >> m_Card >> m_OutFile;
            if(szFormatString.fail())
                IOErrorHandler(INVALIDINPUT);
            m_LogFile <<  m_Card << " name read: "<< m_OutFile <<std::endl;
        }
        // Debug flag
        if (szInputString.substr(0,5) == "debug"){
            std::istringstream szFormatString (szInputString);
            szFormatString >> m_Card >> debug;
            if(szFormatString.fail())
                IOErrorHandler(INVALIDINPUT);
            m_LogFile <<  m_Card << " name read: "<< debug <<std::endl;
        }
        if (szInputString.substr(0,3) == "end"){
            break;
        }
    }
}

void PostBL::IOErrorHandler (ErrorStates ECode) const
// ---------------------------------------------------------------------------
//! Function: Displays error messages related to input data \n
//! Input:    Error code \n
//! Output:   none \n
// ---------------------------------------------------------------------------
{
    std::cerr << '\n';
    if (ECode == INVALIDINPUT) // invalid input
        std::cerr << "Invalid input.";
    else
        std::cerr << "Unknown error ...?";

    std::cerr << '\n' << "Error in input file line : " << m_nLineNumber;
    std::cerr << std::endl;
    exit (1);
}

void PostBL::get_normal_quad (std::vector<EntityHandle>conn, moab::CartVect &v)
// ---------------------------------------------------------------------------
//! Function: Get normal of a quad \n
//! Input:    conn \n
//! Output:   moab::CartVect v \n
// ---------------------------------------------------------------------------
{
    moab::CartVect coords[3];
    MBERRCHK(mb->get_coords(&conn[0], 3, (double*) &coords[0]), mb);
    moab::CartVect AB(coords[1] - coords[0]);
    moab::CartVect BC(coords[2] - coords[1]);
    moab::CartVect normal = AB*BC;
    normal = normal/normal.length();
    v = normal;
}

void PostBL::get_normal_edge (std::vector<EntityHandle>conn, moab::CartVect BC, moab::CartVect &v)
// ---------------------------------------------------------------------------
//! Function: Get normal of a edge along its quad \n
//! Input:    conn of edge, normal to the surf \n
//! Output:   moab::CartVect v \n
// ---------------------------------------------------------------------------
{
    moab::CartVect coords[2];
    MBERRCHK(mb->get_coords(&conn[0], 2, (double*) &coords[0]), mb);
    moab::CartVect AB(coords[1] - coords[0]);
    moab::CartVect normal = AB*BC;
    normal = normal/normal.length();
    v = normal;
}

void PostBL::find_min_edge_length(moab::Range adj_qn, moab::EntityHandle node , moab::Range bl_nodes, double &e_len)
// ---------------------------------------------------------------------------
//! Function: Get minimum edge length from several adjacent quads/edges specified \n
//! Input:    verts of quads, BL vert \n
//! Output:   distance b/w BL vert and inner vert \n
// ---------------------------------------------------------------------------
{
    // get nodes adj(a) to BL node
    moab::CartVect coords[1];
    double len = 0; e_len = 0;
    MBERRCHK(mb->get_coords(&node, 1, (double*) &coords[0]), mb);
    moab::Range non_bl;
    non_bl = subtract(adj_qn, bl_nodes);
    int n_non_bl = (int) non_bl.size();

    // if there are no bl nodes - this case has already been dealt with
    if(non_bl.size() > 0){
        moab::CartVect non_bl_coords[4];
        for(int i=0; i< n_non_bl; i++){
            MBERRCHK(mb->get_coords(non_bl,(double*) &non_bl_coords[0]), mb);
            moab::CartVect edge(coords[0] - non_bl_coords[0]);
            len = edge.length();
            if(i==0)
                e_len = len;
            if(len < e_len)
                e_len = len;
        }
    }
    // m_LogFile << " node minimum edge length" << e_len << std::endl;
}

void PostBL::get_det_jacobian(std::vector<moab::EntityHandle> conn, int offset, double &AvgJ)
// ---------------------------------------------------------------------------
//! Function: Get determinant of jacobian \n
//! Input:    conn \n
//! Output:   vector x, y and z \n
// ---------------------------------------------------------------------------
{
    //TODO: Add quality check for tri/quad and pyramids
    if(m_Conn ==8){
        ++m_JacCalls;
        moab::CartVect vertex[8], xi;
        mstream m_LogFile;
        MBERRCHK(mb->get_coords(&conn[offset], 8, (double*) &vertex[0]), mb);

        double corner[8][3] = { { -1, -1, -1 },
                                {  1, -1, -1 },
                                {  1,  1, -1 },
                                { -1,  1, -1 },
                                { -1, -1,  1 },
                                {  1, -1,  1 },
                                {  1,  1,  1 },
                                { -1,  1,  1 } };

        for (unsigned j = 0; j < 8; ++j) {
            xi[0] = corner[j][0];
            xi[1] = corner[j][1];
            xi[2] = corner[j][2];
            Matrix3 J(0.0);
            double detJ = 0;
            for (unsigned i = 0; i < 8; ++i) {
                const double   xi_p = 1 + xi[0]*corner[i][0];
                const double  eta_p = 1 + xi[1]*corner[i][1];
                const double zeta_p = 1 + xi[2]*corner[i][2];
                const double dNi_dxi   = corner[i][0] * eta_p * zeta_p;
                const double dNi_deta  = corner[i][1] *  xi_p * zeta_p;
                const double dNi_dzeta = corner[i][2] *  xi_p *  eta_p;
                J(0,0) += dNi_dxi   * vertex[i][0];
                J(1,0) += dNi_dxi   * vertex[i][1];
                J(2,0) += dNi_dxi   * vertex[i][2];
                J(0,1) += dNi_deta  * vertex[i][0];
                J(1,1) += dNi_deta  * vertex[i][1];
                J(2,1) += dNi_deta  * vertex[i][2];
                J(0,2) += dNi_dzeta * vertex[i][0];
                J(1,2) += dNi_dzeta * vertex[i][1];
                J(2,2) += dNi_dzeta * vertex[i][2];
            }
            J *= 0.125;
            detJ = J.determinant();
            if(detJ <= 0.0){
                m_LogFile << "We've negative jacobian at the hex corner: "<< j+1 << std::endl;
                exit(0);
            }
            AvgJ+=detJ;
        }
        AvgJ/=8;
        if(m_JacCalls == 1){
            m_JLo = AvgJ;
            m_JHi = AvgJ;
        }
        else if(AvgJ < m_JLo){
            m_JLo = AvgJ;
        }
        else if(AvgJ > m_JHi){
            m_JHi = AvgJ;
        }
    }
}
} // namespace MeshKit


