SUBDIRS = . 

LDADD = $(top_builddir)/src/libMeshKit.la

DATADIR = $(top_srcdir)/data
if HAVE_TRIANGLE
AM_LDFLAGS += $(TRIANGLE_LIBS)
endif

AM_CPPFLAGS += -DDATADIR=$(DATADIR)

AM_CPPFLAGS += -I$(top_srcdir)/test \
               -I$(top_srcdir)/src \
               -I$(top_srcdir)/src/utils \
               -I$(top_srcdir)/src/core \
               -I$(top_srcdir)/src/extern/CAMAL \
               -I$(top_srcdir)/src/algs \
               -I$(top_srcdir)/src/algs/Sweep \
               -I$(top_srcdir)/src/algs/Qslim \
               -I$(top_srcdir)/src/algs/PostBL \
               -I$(top_srcdir)/src/algs/CoreGen \
               -I$(top_srcdir)/src/algs/AssyGen \
               -I$(top_srcdir)/src/algs/QuadMesher \
               -I$(top_srcdir)/src/lemon -I$(top_srcdir) \
               -I$(top_srcdir)/utils

#BROKEN_TESTS = test_jaalquad test_assygen edmond_matching hamiltonian mreport qualestm SimpleTri2Quad test_quadmeshcleanup
BROKEN_TESTS = test_jaalquad test_assygen test_quadmeshcleanup

TESTS = test_edgemesher \
        test_tfimapping \
        test_onetooneswept \
        test_scdmesh \
        test_copymesh \
        test_copygeom \
        test_meshoptemplate \
        test_extrudemesh \
        test_qslimmesher \
        test_ebmesh \
        test_assygen \
        test_postbl
if HAVE_LPSOLVER
TESTS += test_submapping
endif


if HAVE_FBIGEOM
TESTS += \
        test_fbgeom \
        test_mbgeomop \
        test_mbsplitop \
        test_mbvolop
# some of these tests require CAMAL and FBiGeom, too
if HAVE_CAMAL_PAVER
TESTS += test_volice
if HAVE_TRIANGLE
TESTS += test_setpnt2quad
endif
endif
endif

if ENABLE_intassign
TESTS += test_intassign
endif


check_PROGRAMS = $(TESTS) $(BROKEN_TESTS)

MOSTLYCLEANFILES = 5bout.h5m AfterOneSetup.eps AfterTFISetup.eps AfterWinslow.vtk BeforeHex.vtk BeforeVolumeImprove.h5m InitialMapping.vtk OneToOneSwept.h5m SCDmesh.h5m SCDmesh1.vtk SCDmesh2.vtk SCDmesh3.vtk TFIMapping.vtk assygen_default.jou assygen_default.sat assygen_default.template.jou graph.eps intassign.exo mesquite.gpt test.h5m volumesIce.h5m

if HAVE_MOAB
AM_CPPFLAGS += -DMOAB
endif

# Tests and such
#edmond_matching_SOURCES = edmond_matching.cpp
#hamiltonian_SOURCES = hamiltonian.cpp
#qualestm_SOURCES = qualestm.cpp
#SimpleTri2Quad_SOURCES = SimpleTri2Quad.cpp
test_quadmeshcleanup_SOURCES = test_quadmeshcleanup.cpp

test_meshoptemplate_SOURCES = test_meshoptemplate.cpp
test_edgemesher_SOURCES = test_edgemesher.cpp
test_tfimapping_SOURCES = test_tfimapping.cpp
test_onetooneswept_SOURCES = test_onetooneswept.cpp
if HAVE_LPSOLVER
test_submapping_SOURCES = test_submapping.cpp
endif
test_ebmesh_SOURCES = test_ebmesh.cpp
# test_ebmesh_LDADD = -lcubit_geom $(LDADD)
test_copymesh_SOURCES = test_copymesh.cpp
test_copygeom_SOURCES = test_copygeom.cpp
test_extrudemesh_SOURCES = test_extrudemesh.cpp
test_scdmesh_SOURCES = test_scdmesh.cpp
test_qslimmesher_SOURCES = test_qslimmesher.cpp
test_jaalquad_SOURCES = test_jaalquad.cpp
test_assygen_SOURCES = test_assygen.cpp
test_postbl_SOURCES = test_postbl.cpp
if HAVE_FBIGEOM
test_fbgeom_SOURCES = test_fbgeom.cpp
test_mbgeomop_SOURCES = test_mbgeomop.cpp
test_mbsplitop_SOURCES = test_mbsplitop.cpp
if HAVE_TRIANGLE
test_setpnt2quad_SOURCES = test_setpnt2quad.cpp
endif
test_volice_SOURCES = test_volice.cpp
test_mbvolop_SOURCES = test_mbvolop.cpp
endif
if ENABLE_intassign
test_intassign_SOURCES = test_intassign.cpp
endif

# Utility target: build but don't run tests
build-check:
	$(MAKE) 'TESTS_ENVIRONMENT=: ' check


