# Don't require GNU-standard files (Changelog, README, etc.)
AUTOMAKE_OPTIONS = foreign

# Things to build
noinst_LTLIBRARIES = libIntervalAssignment.la
         
SUBDIRS = 

if HAVE_INTASSIGN
  AM_CPPFLAGS += $(IPOPT_CPPFLAGS)
  LDFLAGS += $(IPOPT_LDFLAGS)
endif

# The directory in which to install headers
libIntervalAssignment_la_includedir = $(includedir)

libIntervalAssignment_la_LIBADD = $(IPOPT_LIBS)

# The list of source files, and any header files that do not need to be installed
libIntervalAssignment_la_SOURCES = \
IAData.hpp \
IADataBuilder.cpp \
IADataBuilder.hpp \
IAInterface.cpp \
IAIntWaveNlp.cpp \
IAIntWaveNlp.hpp \
IAIntParabolaNlp.cpp \
IAIntParabolaNlp.hpp \
IANlp.cpp \
IANlp.hpp \
IARoundingNlp.cpp \
IARoundingNlp.hpp \
IASolution.hpp \
IASolver.cpp \
IASolver.hpp \
IASolverTool.cpp \
IASolverTool.hpp \
IASolverToolInt.cpp \
IASolverToolInt.hpp \
IASolverInt.cpp \
IASolverInt.hpp \
IASolverRelaxed.cpp \
IASolverRelaxed.hpp \
IAWeights.cpp \
IAWeights.hpp \
IAWeights.cpp \
IPData.cpp \
IPData.hpp \
IPBend.cpp \
IPBend.hpp \
IASolverBend.cpp \
IASolverBend.hpp \
IABendNlp.cpp \
IABendNlp.hpp


# The list of header files which are to be installed
nobase_libIntervalAssignment_la_include_HEADERS = \
        meshkit/IAInterface.hpp \
		meshkit/IAVariable.hpp

AM_CPPFLAGS += $(DEFINES) -DSRCDIR=$(srcdir) \
              -I$(top_srcdir)/src/core \
              -I$(top_srcdir)/src/utils \
              -I$(top_srcdir) \
              -I$(top_builddir) \
              -I$(top_srcdir)/src/lemon \
              -I$(top_srcdir)/src/algs/QuadMesher \
              -I$(top_srcdir)/src/algs/Sweep \
	      -I$(top_srcdir)/src/algs/AssyGen

LDADD = $(top_builddir)/src/libMeshKit.la

