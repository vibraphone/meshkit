# Don't require GNU-standard files (Changelog, README, etc.)
AUTOMAKE_OPTIONS = foreign

# Things to build
noinst_LTLIBRARIES = libMeshKitutils.la

# Some variables
AM_CPPFLAGS += -DSRCDIR=$(srcdir)

# The directory in which to install headers
libMeshKitutils_la_includedir = $(includedir)

# The list of source files, and any header files that do not need to be installed
libMeshKitutils_la_SOURCES = \
        iMesh.cpp \
        clock.cpp \
        mstream.cpp \
        parser.cpp


# The list of header files which are to be installed
nobase_libMeshKitutils_la_include_HEADERS = \
    meshkit/iBase.hpp \
        meshkit/iGeom.hpp \
        meshkit/iMesh.hpp \
        meshkit/iRel.hpp \
        meshkit/LocalSet.hpp \
        meshkit/LocalTag.hpp \
        meshkit/RegisterMeshOp.hpp \
        meshkit/SimpleArray.hpp \
        meshkit/parser.hpp \
        meshkit/clock.hpp \
        meshkit/vectortemplate.hpp \
        meshkit/matrixtemplate.hpp \
        meshkit/mstream.hpp \
        meshkit/ReadPolyLine.hpp
if HAVE_FBIGEOM
nobase_libMeshKitutils_la_include_HEADERS += \
        meshkit/FBiGeom.hpp
endif


# Boilerplate stuff that doesn't depend on what the targets are
AM_CPPFLAGS += $(DEFINES) -DSRCDIR=$(srcdir) \
              -I$(top_srcdir) -I$(top_builddir) -I$(top_srcdir)/src/core \
              -I$(top_srcdir)/src/lemon

if HAVE_MOAB
AM_CPPFLAGS += -DMOAB
endif
