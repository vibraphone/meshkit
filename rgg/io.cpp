/*********************************************
Feb,10
Reactor Geometry Generator
Argonne National Laboratory

file with input o/p funtions
*********************************************/
#include <sstream>
#include "nrgen.hpp"
#include "parser.hpp"
#include <fstream>
#include "math.h"
#include <string.h>
#include <stdlib.h>
#define DEFAULT_TEST_FILE "Twopin.inp"
#define DEFAULT_OUTPUT_FILE "Twopin.sat"


#define CHECK( STR ) if (err != iBase_SUCCESS) return print_error( STR, err, geom, __FILE__, __LINE__ )
#define ARRAY_INOUT( A ) A.ptr(), &A.capacity(), &A.size()
#define ARRAY_IN( A ) &A[0], A.size()

static bool print_error( const char* desc, 
			 int err,
			 iGeom_Instance geom,
			 const char* file,
			 int line );



// SIMPLE ARRAY TEMPLATE
/* Frees allocated arrays for us */
template <typename T> class SimpleArray
{
private:
  T* arr;
  int arrSize;
  int arrAllocated;
     
public:
  SimpleArray() : arr(0) , arrSize(0), arrAllocated(0) {}
  SimpleArray( unsigned s ) :arrSize(s), arrAllocated(s) {
    arr = (T*)malloc(s*sizeof(T));
    for (unsigned i = 0; i < s; ++i)
      new (arr+i) T();
  }
    
  ~SimpleArray() {
    for (int i = 0; i < size(); ++i)
      arr[i].~T();
    free(arr);
  }

  T**  ptr()            { return &arr; }
  int& size()           { return arrSize; }
  int  size()     const { return arrSize; }
  int& capacity()       { return arrAllocated; }
  int  capacity() const { return arrAllocated; }
    
  typedef T* iterator;
  typedef const T* const_iterator;
  iterator       begin()       { return arr; }
  const_iterator begin() const { return arr; }
  iterator         end()       { return arr + arrSize; }
  const_iterator   end() const { return arr + arrSize; }
    
    
  T& operator[]( unsigned idx )       { return arr[idx]; }
  T  operator[]( unsigned idx ) const { return arr[idx]; }
};

const int MAXCHARS = 300;
std::string szInputString;
std::string szComment = "!";
double pi = M_PI;

iGeom_Instance geom = NULL;
iBase_EntitySetHandle root_set= NULL;
iBase_EntitySetHandle set  = NULL;
iBase_EntityHandle* allvols = NULL;
int nVols = 0;


// NRGEN CLASS FUNCIONS:

void CNrgen::Banner (std::ostream& OF)
// ---------------------------------------------------------------------------
// Function: Prints program banner
// Input:    none
// Output:   none
// ---------------------------------------------------------------------------
{
  std::cout << '\n';
  std::cout << "\t\t--------------------------------------------" << '\n';
  std::cout << "\t\tProgram to Generate Nuclear Reactor Geometries      " << '\n';
  std::cout << "\t\t\tArgonne National Laboratory" << '\n';
  std::cout << "\t\t\t        2009-2010         " << '\n';
  std::cout << "\t\t--------------------------------------------" << '\n';
  std::cout << "\nInfo: Turn on switch for planar geometry in main program - nrgen_test.cpp\n"<< std::endl;
}

void CNrgen::PrepareIO (int argc, char *argv[])
// ---------------------------------------------------------------------------
// Function: Obtains file names and opens input/output files
// Input:    command line arguments
// Output:   none
// ---------------------------------------------------------------------------
{
  // set the geometry engine 
  char* input_file_name = NULL;
  char* output_file_name = NULL;

  int err;
  iGeom_newGeom( 0, &geom, &err, 0 ); // this is default way of specifying ACIS engine
  //  iGeom_newGeom( ";engine=OCC", &geom, &err, 12 );

  // set input file
  bool bDone = false;
  do{
 
    if (3 == argc) {
      input_file_name = argv[1];
      output_file_name = argv[2];
    }
    else {
      //  if (argc < 1) return 1;
      std::cerr << "Usage: " << argv[0] << " <input file>" << " <output file>" << std::endl;
      std::cout <<"  No file specified.  Defaulting to: " << DEFAULT_TEST_FILE
		<< "  " << DEFAULT_OUTPUT_FILE << std::endl;
      input_file_name = (char *)DEFAULT_TEST_FILE;
      output_file_name = (char *)DEFAULT_OUTPUT_FILE;
    }
    //  std::cin >> m_szFileNameIn ;
    m_szFileNameIn =input_file_name;// argv[1];//"sq1.inp";
    std::cout << "\nEntered input file name: " <<  input_file_name <<std::endl;
    // open the file
    m_FileInput.open (m_szFileNameIn.c_str(), std::ios::in); 
    if (!m_FileInput){
      std::cout << "Unable to open file" << std::endl;
      m_FileInput.clear ();
    }
    else
      bDone = true; // file opened successfully
  } while (!bDone);


  // set o/p file name
  do{

    // std::cin >> m_szFileNameOut;
    m_szFileNameOut = output_file_name;//argv[2];//"sq1.sat"; // set here for debugging
    std::cout << "Entered o/p file name: " <<  output_file_name 
	      << "\nInfo: o/p file extension must correspond to geometry engine used to build cgm " << std::endl;
  } while (!bDone);
}

void CNrgen::CountPinCylinders ()
// ---------------------------------------------------------------------------
// Function: reads the input file to count the no. of cyl in a pincell 
// Input:    none
// Output:   none
// ---------------------------------------------------------------------------
{
  CParser Parse;
  int nCyl =0, nCellMat=0;
  std::string card;
  std::string szVolId, szVolAlias;
  int nInputLines;
  m_nLineNumber =0;

  // count the total number of cylinder commands in each pincell
  for(;;){
    if (!Parse.ReadNextLine (m_FileInput, m_nLineNumber, szInputString, 
			     MAXCHARS, szComment))
      IOErrorHandler (INVALIDINPUT);
    if (szInputString.substr(0,8) == "pincells"){
      std::istringstream szFormatString (szInputString);
      szFormatString >> card >> m_nPincells;
      m_Pincell.SetSize(m_nPincells);

      // count the number of cylinder lines for each pincell
      for (int i=1; i<=m_nPincells; i++){
	// read the no. of input lines first pincell
	if (!Parse.ReadNextLine (m_FileInput, m_nLineNumber, szInputString, 
				 MAXCHARS, szComment))
	  IOErrorHandler (INVALIDINPUT);
	std::istringstream szFormatString1 (szInputString);
	szFormatString1 >> szVolId >> szVolAlias >> nInputLines;
	
	// loop thru the input lines of each pincell
	for(int l=1; l<=nInputLines; l++){	
	  if (!Parse.ReadNextLine (m_FileInput, m_nLineNumber, szInputString, 
				   MAXCHARS, szComment))
	    IOErrorHandler (INVALIDINPUT);
	  if (szInputString.substr(0,8) == "cylinder"){
	    ++nCyl;
	  }
	  if (szInputString.substr(0,12) == "cellmaterial"){
	    ++nCellMat;
	  }
	}

	// set the sizes
	if(nCyl>0){
	  if  (nCellMat!=0){
	    m_Pincell(i).SetCellMatSize(nCyl);
	  }
	  m_Pincell(i).SetNumCyl(nCyl);
	}
	else if(nCyl ==0){
	  m_Pincell(i).SetCellMatSize(nCellMat);
	}
	nCyl = 0;
	nCellMat = 0;
      }
    }
    // breaking condition
    if(szInputString.substr(0,3) == "end"){
      std::istringstream szFormatString (szInputString);
      break;
    }
  }
}

void CNrgen::ReadPinCellData (int i)
//---------------------------------------------------------------------------
//Function: reading file and storing data
//Input:    none
//Output:   none
//---------------------------------------------------------------------------
{
  CParser Parse;
  std::string card;
  std::string szVolId, szVolAlias;
  int nInputLines;
  double dLZ, dFlatF, dPX, dPY, dPZ;
  int nMaterials;
  CVector <std::string> szVMatName, szVMatAlias;
  int nCyl=0;
  int nRadii;
  CVector<double>dVCoor(2); // XY Pos of cylinder
  CVector<double> dVCylRadii,dVCylZPos;
  CVector<std::string> szVCylMat;
  CVector<std::string> szVCellMat;
  CVector<double> dZVStart, dZVEnd;
  int nCellMat;

  //loop over input lines
  if (m_szGeomType == "cartesian"){

    std::cout << "\ngetting volume id";
    if (!Parse.ReadNextLine (m_FileInput, m_nLineNumber, szInputString, 
			     MAXCHARS, szComment))
      IOErrorHandler (INVALIDINPUT);
    std::istringstream szFormatString (szInputString);
    szFormatString >> szVolId >> szVolAlias >> nInputLines;
    m_Pincell(i).SetLineOne (szVolId, szVolAlias, nInputLines);

    for(int l=1; l<=nInputLines; l++){	
      if (!Parse.ReadNextLine (m_FileInput, m_nLineNumber, szInputString, 
			       MAXCHARS, szComment))
	IOErrorHandler (INVALIDINPUT);
      if (szInputString.substr(0,5) == "pitch"){
	
	std::istringstream szFormatString (szInputString);
	std::cout << "\ngetting pitch data";
	szFormatString >> card >> dPX >> dPY >> dPZ;
	m_Pincell(i).SetPitch (dPX, dPY, dPZ);
      } 
      if (szInputString.substr(0,9) == "materials"){

	std::istringstream szFormatString (szInputString);
	szFormatString >> card >> nMaterials;

	//setting local arrays
	szVMatName.SetSize(nMaterials);
	szVMatAlias.SetSize(nMaterials);

	//set class variable sizes
	m_Pincell(i).SetMatArray(nMaterials);
	std::cout << "\ngetting material data";
	for(int j=1; j<= nMaterials; j++)
	  szFormatString >> szVMatName(j) >> szVMatAlias(j);
	m_Pincell(i).SetMat(szVMatName, szVMatAlias);
      }
      if (szInputString.substr(0,8) == "cylinder"){

	++nCyl;
	std::cout << "\ngetting cylinder data";
	std::istringstream szFormatString (szInputString);
	szFormatString >> card >> nRadii >> dVCoor(1) >> dVCoor(2);
	m_Pincell(i).SetCylSizes(nCyl, nRadii);
	m_Pincell(i).SetCylPos(nCyl, dVCoor);

	//set local array
	dVCylRadii.SetSize(nRadii);
	szVCylMat.SetSize(nRadii);
	dVCylZPos.SetSize(nRadii);
	//
	m_Pincell(i).SetCylSizes(nCyl, nRadii);

	// reading ZCoords
	for(int k=1; k<=2; k++)
	  szFormatString >> dVCylZPos(k);
	m_Pincell(i).SetCylZPos(nCyl, dVCylZPos);
	// reading Radii
	for(int l=1; l<= nRadii; l++)
	  szFormatString >> dVCylRadii(l);
	m_Pincell(i).SetCylRadii(nCyl, dVCylRadii);

	// reading Material alias
	for(int m=1; m<= nRadii; m++)
	  szFormatString >> szVCylMat(m);
	m_Pincell(i).SetCylMat(nCyl, szVCylMat);
      }
      if (szInputString.substr(0,12) == "cellmaterial"){

	std::cout << "\ngetting cell material data\n";
	std::istringstream szFormatString (szInputString);
	szFormatString >> card;

	//set local arrays
	m_Pincell(i).GetCellMatSize(nCellMat); // since size of cell material already set equal to number of cylinders
	dZVStart.SetSize(nCellMat);
	dZVEnd.SetSize(nCellMat);
	szVCellMat.SetSize(nCellMat);

	for(int k=1; k<=nCellMat; k++) 
	  szFormatString >> dZVStart(k)>> dZVEnd(k) >> szVCellMat(k);
	m_Pincell(i).SetCellMat(dZVStart, dZVEnd, szVCellMat);			
      }
    }
  }//if cartesian ends

  if (m_szGeomType == "hexagonal"){

    std::cout << "\ngetting volume id";
    if (!Parse.ReadNextLine (m_FileInput, m_nLineNumber, szInputString, 
			     MAXCHARS, szComment))
      IOErrorHandler (INVALIDINPUT);
    std::istringstream szFormatString (szInputString);
    szFormatString >> szVolId >> szVolAlias >> nInputLines;
    m_Pincell(i).SetLineOne (szVolId, szVolAlias, nInputLines);

    for(int l=1; l<=nInputLines; l++){	
      if (!Parse.ReadNextLine (m_FileInput, m_nLineNumber, szInputString, 
			       MAXCHARS, szComment))
	IOErrorHandler (INVALIDINPUT);
      if (szInputString.substr(0,5) == "pitch"){
	
	std::istringstream szFormatString (szInputString);
	std::cout << "\ngetting pitch data";
	szFormatString >> card >> dFlatF >> dLZ;
	m_Pincell(i).SetPitch (dFlatF, dLZ);			
      } 
      if (szInputString.substr(0,9) == "materials"){

	std::istringstream szFormatString (szInputString);
	szFormatString >> card >> nMaterials;

	//setting local arrays
	szVMatName.SetSize(nMaterials);
	szVMatAlias.SetSize(nMaterials);

	//set class variable sizes
	m_Pincell(i).SetMatArray(nMaterials);
	std::cout << "\ngetting material data";
	for(int j=1; j<= nMaterials; j++)
	  szFormatString >> szVMatName(j) >> szVMatAlias(j);
	m_Pincell(i).SetMat(szVMatName, szVMatAlias);
      }
      if (szInputString.substr(0,8) == "cylinder"){

	++nCyl;
	std::cout << "\ngetting cylinder data";
	std::istringstream szFormatString (szInputString);
	szFormatString >> card >> nRadii >> dVCoor(1) >> dVCoor(2);
	m_Pincell(i).SetCylSizes(nCyl, nRadii);
	m_Pincell(i).SetCylPos(nCyl, dVCoor);

	//set local array
	dVCylRadii.SetSize(nRadii);
	szVCylMat.SetSize(nRadii);
	dVCylZPos.SetSize(2);
	//
	m_Pincell(i).SetCylSizes(nCyl, nRadii);

	// reading ZCoords - max and min 2 always
	for(int k=1; k<=2; k++)
	  szFormatString >> dVCylZPos(k);
	m_Pincell(i).SetCylZPos(nCyl, dVCylZPos);

	// reading Radii
	for(int l=1; l<= nRadii; l++)
	  szFormatString >> dVCylRadii(l);
	m_Pincell(i).SetCylRadii(nCyl, dVCylRadii);

	// reading Material alias
	for(int m=1; m<= nRadii; m++)
	  szFormatString >> szVCylMat(m);
	m_Pincell(i).SetCylMat(nCyl, szVCylMat);
      }
      if (szInputString.substr(0,12) == "cellmaterial"){

	std::cout << "\ngetting cell material data";
	std::istringstream szFormatString (szInputString);
	szFormatString >> card;

	//set local arrays
	m_Pincell(i).GetCellMatSize(nCellMat); // since size of cell material already set equal to number of cylinders
	dZVStart.SetSize(nCellMat);
	dZVEnd.SetSize(nCellMat);
	szVCellMat.SetSize(nCellMat);
	
	for(int k=1; k<=nCellMat; k++) 
	  szFormatString >> dZVStart(k)>> dZVEnd(k) >> szVCellMat(k);
	m_Pincell(i).SetCellMat(dZVStart, dZVEnd, szVCellMat);			
      }
    }
  }// if hexagonal end
}


int CNrgen::ReadAndCreate()
//---------------------------------------------------------------------------
//Function: reads the input file and creates assembly
//Input:    none
//Output:   none
//---------------------------------------------------------------------------
{
  //Rewind the input file
  m_FileInput.clear (std::ios_base::goodbit);
  m_FileInput.seekg (0L, std::ios::beg);
	
  CParser Parse;
  std::string card, szVolId, szVolAlias;
  int nInputLines, nTempPin;
  double dX = 0.0, dY =0.0, dZ=0.0, dMoveX = 0.0, dMoveY = 0.0;
  int err;
  double  dP, dH, dSide, dHeight;
  // name tag handle
  iBase_TagHandle this_tag;
  char* tag_name =(char *)"NAME";
  std::string sMatName = "";

  iBase_EntityHandle assm = NULL, max_surf = NULL;
  SimpleArray<iBase_EntityHandle> assms(m_nDimensions); // setup while reading the problem size
  iBase_EntityHandle tmp_vol= NULL, tmp_new= NULL;
  SimpleArray<iBase_EntityHandle> in_pins, all_geom;
  
  // get tag handle for 'NAME' tag, already created as iGeom instance is created
  iGeom_getTagHandle(geom, tag_name, &this_tag, &err, 4);
  CHECK("getTagHandle failed");
  
  // start reading the input file break when encounter end
  for(;;){
    if (!Parse.ReadNextLine (m_FileInput, m_nLineNumber, szInputString, 
			     MAXCHARS, szComment))
      IOErrorHandler (INVALIDINPUT);
    if (szInputString.substr(0,12) == "geometrytype"){
      std::istringstream szFormatString (szInputString);
      szFormatString >> card >> m_szGeomType;
    }

    if (szInputString.substr(0,9) == "materials"){

      std::cout << "getting assembly material data" << std::endl;
      std::istringstream szFormatString (szInputString);
      szFormatString >> card >> m_nAssemblyMat;
      m_szAssmMat.SetSize(m_nAssemblyMat); m_szAssmMatAlias.SetSize(m_nAssemblyMat);
      
      for (int j=1; j<=m_nAssemblyMat; j++)
	szFormatString >> m_szAssmMat(j) >> m_szAssmMatAlias(j);
    }   

    if (szInputString.substr(0,10) == "dimensions"){

      std::cout << "getting assembly dimensions" << std::endl;
      if(m_szGeomType =="hexagonal"){
	std::istringstream szFormatString (szInputString);
	m_dVXYAssm.SetSize(2); m_dVZAssm.SetSize(2);

	szFormatString >> card >> m_nDimensions 
		       >> m_dVXYAssm(1) >> m_dVXYAssm(2)
		       >> m_dVZAssm(1) >> m_dVZAssm(2);

	m_dVAssmPitch.SetSize(m_nDimensions); m_szMAlias.SetSize(m_nDimensions);

	for (int i=1; i<=m_nDimensions; i++)
	  szFormatString >> m_dVAssmPitch(i);

	for (int i=1; i<=m_nDimensions; i++)
	  szFormatString >> m_szMAlias(i);
      }   
      if(m_szGeomType =="cartesian"){
	std::istringstream szFormatString (szInputString);
	m_dVXYAssm.SetSize(2); m_dVZAssm.SetSize(2);

	szFormatString >> card >> m_nDimensions 
		       >> m_dVXYAssm(1) >> m_dVXYAssm(2)
		       >> m_dVZAssm(1) >> m_dVZAssm(2);

	m_dVAssmPitchX.SetSize(m_nDimensions);	m_dVAssmPitchY.SetSize(m_nDimensions);
	m_szMAlias.SetSize(m_nDimensions);

	for (int i=1; i<=m_nDimensions; i++)
	  szFormatString >> m_dVAssmPitchX(i) >> m_dVAssmPitchY(i);

	for (int i=1; i<=m_nDimensions; i++)
	  szFormatString >> m_szMAlias(i);
      }  
    }

    if (szInputString.substr(0,8) == "pincells"){
      std::istringstream szFormatString (szInputString);
      szFormatString >> card >> m_nPincells >> m_dPitch;
      
      // loop thro' the pincells and read/store pincell data
      for (int i=1; i<=m_nPincells; i++){

	//get the number of cylinder in each pincell
	double dTotalHeight = m_dVZAssm(2)-m_dVZAssm(1);
	m_Pincell(i).SetPitch(m_dPitch, dTotalHeight);
	ReadPinCellData(i);
	std::cout << "\nread pincell " << i << std::endl;
      }
    }
    if (szInputString.substr(0,8) == "assembly"){
      std::cout << "\ngetting Assembly data and creating ..\n"<< std::endl;

      // read and create the assembly for hexagonal lattice
      if(m_szGeomType =="hexagonal"){
	std::istringstream szFormatString (szInputString);
	szFormatString >> card >> m_nPin;

	// width of the hexagon is n*n+1/2
	int nWidth =2*m_nPin -1;

	// creating a square array of size width
	m_Assembly.SetSize(nWidth, nWidth);

	int t; // index for hexagonal lattice

	for(int m=1; m<=nWidth; m++){
	  if (!Parse.ReadNextLine (m_FileInput, m_nLineNumber, szInputString, 
				   MAXCHARS, szComment))
	    IOErrorHandler (INVALIDINPUT);
	  if(m>m_nPin)
	    t = 2*m_nPin - m;
	  else
	    t = m;
	  std::istringstream szFormatString1 (szInputString);

	  for(int n=1; n<=(m_nPin + t - 1); n++){
	    szFormatString1 >> m_Assembly(m,n);

	    // find that pincell
	    for(int b=1; b<=m_nPincells; b++){
	      m_Pincell(b).GetLineOne(szVolId, szVolAlias, nInputLines);
	      if(m_Assembly(m,n) == szVolAlias)
		nTempPin = b;
	    }
	    // now compute the location and create it
	    ComputePinCentroid(nTempPin, m_Assembly, m, n, dX, dY, dZ);	    

	    // now create the pincell in the location found
	    std::cout << "\n--------------------------------------------------"<<std::endl;
	    std::cout << " m = " << m <<" n = " << n << std::endl;
	    std::cout << "creating pin: " << nTempPin;
	    std::cout << " at X Y Z " << dX << " " << dY << " " << dZ << std::endl;
	    
	    int error = CreatePinCell(nTempPin, dX, dY,dZ);
	    if (error!=1)
	      std::cout << "Error in function CreatePinCell";
  
	  }
	}

	if(m_nDimensions >0){

	  // get all the entities (in pins)defined so far, in an entity set - for subtraction later
	  iGeom_getEntities( geom, root_set, iBase_REGION, ARRAY_INOUT(in_pins),&err );
	  CHECK( "ERROR : getRootSet failed!" );

	  // create outermost hexes
	  std::cout << "\n\nCreating surrounding outer hexes .." << std::endl;
	  for(int n=1;n<=m_nDimensions; n++){
	    dSide = m_dVAssmPitch(n)/(sqrt(3));
	    dHeight = m_dVZAssm(2) - m_dVZAssm(1);

	    // creating coverings
	    iGeom_createPrism(geom, dHeight, 6, 
			      dSide, dSide,
			      &assm, &err); 
	    CHECK("Prism creation failed.");
	  
	    // rotate the prism
	    iGeom_rotateEnt (geom, assm, 30, 0, 0, 1, &err);
	    CHECK("Rotation failed failed.");

	    m_Pincell(1).GetPitch(dP, dH);
	    dX = m_nPin*dP;
	    dY = (m_nPin-1)*dP*sqrt(3.0)/2.0;
	    dZ = (m_dVZAssm(2) + m_dVZAssm(1))/2.0;

	    // position the prism
	    iGeom_moveEnt(geom, assm, dX,dY,dZ, &err);
	    CHECK("Move failed failed.");  

	    assms[n-1]=assm;
	  }
	} // if dimension ends
      } // if hexagonal ends


      if(m_szGeomType == "cartesian"){

	std::istringstream szFormatString (szInputString);
	szFormatString >> card >> m_nPinX >> m_nPinY;
	m_Assembly.SetSize(m_nPinX,m_nPinY);

	//read the next line to get assembly info &store assembly info
	for(int m=1; m<=m_nPinY; m++){
	  if (!Parse.ReadNextLine (m_FileInput, m_nLineNumber, szInputString, 
				   MAXCHARS, szComment))
	    IOErrorHandler (INVALIDINPUT);
	  std::istringstream szFormatString1 (szInputString);
	    
	  //store the line read in Assembly array and create / position the pin in the core
	  for(int n=1; n<=m_nPinX; n++){
	    szFormatString1 >> m_Assembly(m,n);
	      
	    // loop thro' all pins to get the type of pin
	    for(int b=1; b<=m_nPincells; b++){
	      m_Pincell(b).GetLineOne(szVolId, szVolAlias, nInputLines);
	      if(m_Assembly(m,n) == szVolAlias)
		nTempPin = b;
	    }

	    //now compute the location where the pin needs to be placed
	    ComputePinCentroid(nTempPin, m_Assembly, m, n, dX, dY, dZ);

	    // now create the pincell in the location found
	    std::cout << "\n--------------------------------------------------"<<std::endl;
	    std::cout << " m = " << m <<" n = " << n << std::endl;
	    std::cout << "creating pin: " << nTempPin;
	    std::cout << " at X Y Z " << dX << " " << dY << " " << dZ << std::endl;

	    int error = CreatePinCell(nTempPin, dX, dY, dZ);
	    if (error!=1)
	      std::cout << "Error in function CreatePinCell";

	    // dMoveX and dMoveY are stored for positioning the outer squares later
	    if(m == m_nPinY && n ==m_nPinX){
	      dMoveX = dX/2.0;
	      dMoveY = dY/2.0;
	    }
	  }
	}
	std::cout << "\n--------------------------------------------------"<<std::endl;
	
	if(m_nDimensions > 0){
	  // get all the entities (in pins)defined so far, in an entity set - for subtraction later
	  iGeom_getEntities( geom, root_set, iBase_REGION, ARRAY_INOUT(in_pins),&err );
	  CHECK( "ERROR : getRootSet failed!" );
	
	
	  // create outermost rectangular blocks
	  std::cout << "\nCreating surrounding outer blocks .." << std::endl;
	  for(int n=1;n<=m_nDimensions; n++){
	    dHeight = m_dVZAssm(2) - m_dVZAssm(1);
	  
	    iGeom_createBrick(geom, m_dVAssmPitchX(n),  m_dVAssmPitchY(n),dHeight, 
			      &assm, &err); 
	    CHECK("Prism creation failed.");	  
	  
	    //		  ComputePinCentroid(nTempPin, m_Assembly, m_nPinX, m_nPinY, dX, dY, dZ);
	    dX = m_dVAssmPitchX(n)/4.0;
	    dY =  m_dVAssmPitchY(n)/4.0;
	    dZ = (m_dVZAssm(2) + m_dVZAssm(1))/2.0;
	    std::cout << "Move " <<   dMoveX << " " << dMoveY <<std::endl;
	    iGeom_moveEnt(geom, assm, dMoveX,dMoveY,dZ, &err);
	    CHECK("Move failed failed.");  
	  
	    assms[n-1]=assm;
	  }
	} // if m_nDimensions ends
      } // if cartesian ends
      
	// name the innermost outer covering common for both cartesian and hexagonal assembliees   
      if(m_nDimensions >0){
	int tag_no;
	for(int p=1;p<=m_szAssmMatAlias.GetSize();p++){
	  if(strcmp ( m_szMAlias(1).c_str(), m_szAssmMatAlias(p).c_str()) == 0){
	    sMatName =  m_szAssmMat(p);
	    tag_no=p;
	  }
	}
	std::cout << "\ncreated innermost block: " << sMatName << std::endl;
	
	tmp_vol = assms[0];
	iGeom_setData(geom, tmp_vol, this_tag,
		      sMatName.c_str(), sMatName.size(), &err);
	CHECK("setData failed");
	int err = Get_Max_Surf(tmp_vol, &max_surf);
	if(max_surf !=0){
	  sMatName+="_surface";
	  iGeom_setData(geom, max_surf, this_tag,
			sMatName.c_str(), sMatName.size(), &err);
	  CHECK("setData failed");
	  std::cout << "created: " << sMatName << std::endl;
	}
	
	// now subtract the outermost hexes
	for(int n=m_nDimensions; n>1 ; n--){
	  
	  if(n>1){
	    
	    // copy cyl before subtract 
	    iGeom_copyEnt(geom, assms[n-2], &tmp_vol, &err);
	    CHECK("Couldn't copy inner duct wall prism.");
	    
	    // subtract outer most cyl from brick
	    iGeom_subtractEnts(geom, assms[n-1], tmp_vol, &tmp_new, &err);
	    CHECK("Subtract of inner from outer failed.");	  
	    
	    assms[n-1]=tmp_new;
	    
	    // name the vols by searching for the full name of the abbreviated Cell Mat
	    for(int p=1;p<=m_szAssmMatAlias.GetSize();p++){
	      if(strcmp ( m_szMAlias(n).c_str(), m_szAssmMatAlias(p).c_str()) == 0){
		sMatName =  m_szAssmMat(p);
	      }
	    }
	    std::cout << "created: " << sMatName << std::endl;
	    
	    iGeom_setData(geom, tmp_new, this_tag,
			  sMatName.c_str(), sMatName.size(), &err);
	    CHECK("setData failed");

	
	    int err = Get_Max_Surf(tmp_new, &max_surf);
	    if(max_surf !=0){
	      sMatName+="_surface";
	      iGeom_setData(geom, max_surf, this_tag,
			    sMatName.c_str(), sMatName.size(), &err);
	      CHECK("setData failed");
	      std::cout << "created: " << sMatName << std::endl;
	    }
	  }
	}
	tmp_vol = assms[0];
	iBase_EntityHandle tmp_new1 = NULL;

	std::cout << "\n--------------------------------------------------"<<std::endl;
	// subtract the innermost hex from the pins
	iBase_EntityHandle unite= NULL;
	SimpleArray<iBase_EntityHandle> copy_inpins(in_pins.size());
	std::cout << "Subtracting all pins from assembly .. " << std::endl;

	for (int i=0; i<in_pins.size();i++){
	  iGeom_copyEnt(geom, in_pins[i], &copy_inpins[i], &err);
	  CHECK("Couldn't copy inner duct wall prism.");		  
	}
	iGeom_uniteEnts(geom,ARRAY_IN(in_pins), &unite, &err);
	CHECK( "uniteEnts failed!" );	  
	iGeom_subtractEnts(geom, tmp_vol,unite, &tmp_new1, &err);
	CHECK("Couldn't subtract pins from block.");
      }

      std::cout << "\n--------------------------------------------------"<<std::endl;

      // getting all entities for merge and imprint
      SimpleArray<iBase_EntityHandle> entities;
      iGeom_getEntities( geom, root_set, iBase_REGION, ARRAY_INOUT(entities),&err );
      CHECK( "ERROR : getRootSet failed!" );
      double dTol = 0.01;


      
      // now imprint
      std::cout << "\n\nImprinting...." << std::endl;
      iGeom_imprintEnts(geom, ARRAY_IN(entities),&err); 
      CHECK("Imprint failed.");
      std::cout << "\n--------------------------------------------------"<<std::endl;

      // now  merge
      std::cout << "\n\nMerging...." << std::endl;
      iGeom_mergeEnts(geom, ARRAY_IN(entities), dTol, &err);
      CHECK("Merge failed.");
      std::cout <<"merging finished."<< std::endl;
      std::cout << "\n--------------------------------------------------"<<std::endl;

      // branch for creating planar geometry
      if(m_nPlanar ==1){ // this is set in main program nrgen_test.cpp
	std::cout << "\nCREATING TOP SURFACE..." << std::endl;
	iGeom_getEntities( geom, root_set, iBase_REGION,ARRAY_INOUT(all_geom),&err );
	CHECK( "ERROR : getRootSet failed!" );

	// get the surface with max z
	SimpleArray<iBase_EntityHandle> surfs;
 	int *offset = NULL, offset_alloc = 0, offset_size;
       
	iGeom_getArrAdj( geom, ARRAY_IN(all_geom) , iBase_FACE, ARRAY_INOUT(surfs),
			 &offset, &offset_alloc, &offset_size, &err ); 
	CHECK( "ERROR : getArrAdj failed!" );
 
 	SimpleArray<double> max_corn, min_corn;
	iGeom_getArrBoundBox( geom, ARRAY_IN(surfs), iBase_INTERLEAVED, 
			      ARRAY_INOUT( min_corn ),
			      ARRAY_INOUT( max_corn ),
			      &err );
	CHECK( "Problems getting max surf for rotation." );
  
	int t=0;
	double dtol = m_dVZAssm(2);
	for (int i = 0; i < surfs.size(); ++i){ 
	  if (max_corn[3*i+2] < dtol)  t++;
	}

	SimpleArray<iBase_EntityHandle> max_surfs(t);
	SimpleArray<iBase_EntityHandle> new_surfs(t);

	t=0;

	for (int i = 0; i < surfs.size(); ++i){
	  // first find the max z-coordinate
	  if (max_corn[3*i+2] <  dtol){
	    max_surfs[t] = surfs[i];
	    t++;
	  }
	}

	for(int i = 0; i < max_surfs.size(); ++i){
	  iGeom_copyEnt(geom, max_surfs[i], &new_surfs[i], &err);
	  CHECK( "Problems creating surface." );
	}

	for(int i=0; i<all_geom.size(); i++){
	  iGeom_deleteEnt(geom, all_geom[i], &err);
	  CHECK( "Problems deleting cyl." );
	}
      }

      // save .sat file
      iGeom_save(geom, m_szFileNameOut.c_str(), NULL, &err, m_szFileNameOut.length() , 0);
      CHECK("Save to file failed.");
      
      std::cout << "Normal Termination.\n"<< m_szFileNameOut << " file saved." << std::endl;
    }// if assembly card ends

    if (szInputString.substr(0,3) == "end"){
      break;
    }
  }
  // check data for validity
  if (m_nPincells < 1) 
    IOErrorHandler (PINCELLS);
  return 1;
}


int CNrgen::CreatePinCell(int i, double dX, double dY, double dZ)
//---------------------------------------------------------------------------
//Function: Create pincell i in location dX dY and dZ
//Input:    none
//Output:   none
//---------------------------------------------------------------------------
{
  int nRadii=0, nCyl=0, err=0, nCells = 0;
  double dCylMoveX = 0.0, dCylMoveY = 0.0, dHeightTotal = 0.0;
  CVector<double> dVCylZPos(2), dVCylXYPos(2);
  CVector<std::string> szVMatName; CVector<std::string> szVMatAlias;
  CVector<std::string> szVCellMat;
  CVector<double> dVStartZ; CVector<double> dVEndZ;
 
  double dHeight =0.0,dZMove = 0.0, PX = 0.0,PY = 0.0,PZ = 0.0, dP=0.0;
  iBase_EntityHandle cell;
  iBase_EntityHandle cyl= NULL, tmp_vol= NULL,tmp_vol1= NULL, tmp_new= NULL, max_surf = NULL;

  // name tag handle
  iBase_TagHandle this_tag= NULL;
  char* tag_name = (char*)"NAME";

  std::string sMatName = "";

  // get tag handle for 'NAME' tag, already created as iGeom instance is created
  iGeom_getTagHandle(geom, tag_name, &this_tag, &err, 4);
  CHECK("getTagHandle failed");
      
  // get cell material
  m_Pincell(i).GetCellMatSize(nCells);
  SimpleArray<iBase_EntityHandle> cells(nCells);

  // branch when cells are present
  if(nCells > 0){  
    dVStartZ.SetSize(nCells);
    dVEndZ.SetSize(nCells);
    szVCellMat.SetSize(nCells);
    m_Pincell(i).GetCellMat(dVStartZ, dVEndZ, szVCellMat);
  
    // get cylinder data
    m_Pincell(i).GetNumCyl(nCyl);

    for(int n=1;n<=nCells; n++){

      dHeight = dVEndZ(n) - dVStartZ(n);  

      if(m_szGeomType =="hexagonal"){
	
	m_Pincell(i).GetPitch(dP, dHeightTotal); // this dHeight is not used in creation
	double dSide = dP/(sqrt(3));

	if(nCells >0){
	  // create prism
	  iGeom_createPrism(geom, dHeight, 6, 
			    dSide, dSide,
			    &cell, &err); 
	  CHECK("Prism creation failed.");
	}
      }  
      // if cartesian geometry
      if(m_szGeomType =="cartesian"){  

	m_Pincell(i).GetPitch(PX, PY, PZ);
	
	if(nCells >0){
	  // create brick
	  iGeom_createBrick( geom,PX,PY,dHeight,&cell,&err );
	  CHECK("Couldn't create pincell."); 
	}
      }
      
      dZMove = (dVEndZ(n)+dVEndZ(n-1))/2.0;

      if(nCells > 0){
	// position the brick in assembly
	iGeom_moveEnt(geom, cell, dX, dY, dZMove, &err); 
	CHECK("Couldn't move cell.");
	cells[n-1]=cell;

	//search for the full name of the abbreviated Cell Mat and set name
	for(int p=1;p<= m_szAssmMatAlias.GetSize();p++){
	  if(strcmp (szVCellMat(n).c_str(), m_szAssmMatAlias(p).c_str()) == 0){
	    sMatName = m_szAssmMat(p);
	  }
	}

	std::cout << "created: " << sMatName << std::endl;	
	iGeom_setData(geom, cell, this_tag,
		      sMatName.c_str(), sMatName.size(), &err);
	CHECK("setData failed");

	int err = Get_Max_Surf(cell, &max_surf);
	sMatName+="_surface";
	if(max_surf !=0){
	  iGeom_setData(geom, max_surf, this_tag,
			sMatName.c_str(), sMatName.size(), &err);
	  CHECK("setData failed");
	  std::cout << "created: " << sMatName << std::endl;
	}

      }
      // loop and create cylinders
      if(nCyl > 0){
	m_Pincell(i).GetCylSizes(n, nRadii);
	SimpleArray<iBase_EntityHandle> cyls(nRadii);

	//declare variables
	CVector<double> dVCylRadii(nRadii);
	CVector<std::string> szVMat(nRadii);
	CVector<std::string> szVCylMat(nRadii);
 
	//get values
	m_Pincell(i).GetCylRadii(n, dVCylRadii);
	m_Pincell(i).GetCylPos(n, dVCylXYPos);
	m_Pincell(i).GetCylMat(n, szVCylMat);
	m_Pincell(i).GetCylZPos(n, dVCylZPos);
	dHeight = dVCylZPos(2)-dVCylZPos(1);

	for (int m=1; m<=nRadii; m++){ 
 
	  iGeom_createCylinder(geom, dHeight, dVCylRadii(m), dVCylRadii(m),
			       &cyl, &err);
	  CHECK("Couldn't create fuel rod.");

	  // move their centers and also move to the assembly location  ! Modify if cyl is outside brick
	  dCylMoveX = dVCylXYPos(1)+dX;
	  dCylMoveY = dVCylXYPos(2)+dY;
	  dZMove = (dVCylZPos(1)+dVCylZPos(2))/2.0;

	  iGeom_moveEnt(geom, cyl, dCylMoveX,dCylMoveY,dZMove, &err);
	  CHECK("Couldn't move cyl.");
	  cyls[m-1] = cyl;
	}
 
	if(nCells > 0){
	  // copy cyl before subtract 
	  iGeom_copyEnt(geom, cyls[nRadii-1], &tmp_vol, &err);
	  CHECK("Couldn't copy inner duct wall prism.");
	
	  // subtract outer most cyl from brick
	  iGeom_subtractEnts(geom, cells[n-1], cyls[nRadii-1], &tmp_new, &err);
	  CHECK("Subtract of inner from outer failed.");
	
	  // copy the new into the cyl array
	  cells[n-1] = tmp_new; cell = tmp_new;
	  cyls[nRadii-1]=tmp_vol;

	}
	std::cout << "hi  "<< szVCylMat(1) << std::endl;
	//set tag on inner most cylinder, search for the full name of the abbreviated Cell Mat
	for(int p=1;p<=m_szAssmMatAlias.GetSize();p++){
	  if(strcmp (szVCylMat(1).c_str(), m_szAssmMatAlias(p).c_str()) == 0){
	    sMatName = m_szAssmMat(p);
	  }
	}
	tmp_vol1=cyls[0]; //inner most cyl

	iGeom_setData(geom, tmp_vol1, this_tag,
		      sMatName.c_str(), 10, &err);
	CHECK("setData failed");
	int err = Get_Max_Surf(tmp_vol1, &max_surf);
	if(max_surf !=0){
	  sMatName+="_surface";
	  iGeom_setData(geom, max_surf, this_tag,
			sMatName.c_str(), sMatName.size(), &err);
	  CHECK("setData failed");
	  std::cout << "created: " << sMatName << std::endl;
	}   

	// other cyl annulus after substraction
	for (int b=nRadii; b>1; b--){  

	  iGeom_copyEnt(geom, cyls[b-2], &tmp_vol, &err);
	  CHECK("Couldn't copy inner duct wall prism.");

	  //subtract tmp vol from the outer most
	  iGeom_subtractEnts(geom, cyls[b-1], tmp_vol, &tmp_new, &err);
	  CHECK("Subtract of inner from outer failed.");
  
	  // now search for the full name of the abbreviated Cell Mat
	  int tag_no;
	  for(int p=1;p<=m_szAssmMatAlias.GetSize();p++){
	    if(strcmp (szVCylMat(b).c_str(), m_szAssmMatAlias(p).c_str()) == 0){
	      tag_no = p;
	      sMatName =  m_szAssmMat(p);
	    }
	  }
	  std::cout << "created: " << sMatName << std::endl;
	  // set the name of the annulus
	  iGeom_setData(geom, tmp_new, this_tag,
			sMatName.c_str(),sMatName.size(), &err);
	  CHECK("setData failed");
	  if(max_surf !=0){
	    int err = Get_Max_Surf(tmp_new, &max_surf);
	    sMatName+="_surface";
	    iGeom_setData(geom, max_surf, this_tag,
			  sMatName.c_str(), sMatName.size(), &err);
	    CHECK("setData failed");
	    std::cout << "created: " << sMatName << std::endl;
	  }

	  // copy the new into the cyl array
	  cyls[b-1] = tmp_new;
	}
      }
    }
  }
  // this branch of the routine is responsible for creating cylinders with '0' cells
  if(nCells == 0){ 
 
    // get cylinder data
    m_Pincell(i).GetNumCyl(nCyl);
    nCells = nCyl;


    for(int n=1;n<=nCells; n++){

 
      if(m_szGeomType =="hexagonal"){
	
	m_Pincell(i).GetPitch(dP, dHeightTotal); // this dHeight is not used in creation
      }
      // if cartesian geometry
      if(m_szGeomType =="cartesian"){  
	
	m_Pincell(i).GetPitch(PX, PY, PZ);
      }

      // loop and create cylinders
      if(nCyl > 0){
	m_Pincell(i).GetCylSizes(n, nRadii);
	SimpleArray<iBase_EntityHandle> cyls(nRadii);

	//declare variables
	CVector<double> dVCylRadii(nRadii);
	CVector<std::string> szVMat(nRadii);
	CVector<std::string> szVCylMat(nRadii);
 
	//get values
	m_Pincell(i).GetCylRadii(n, dVCylRadii);
	m_Pincell(i).GetCylPos(n, dVCylXYPos);
	m_Pincell(i).GetCylMat(n, szVCylMat);
	m_Pincell(i).GetCylZPos(n, dVCylZPos);

	dHeight = dVCylZPos(2)-dVCylZPos(1);

	for (int m=1; m<=nRadii; m++){ 
 
	  iGeom_createCylinder(geom, dHeight, dVCylRadii(m), dVCylRadii(m),
			       &cyl, &err);
	  CHECK("Couldn't create fuel rod.");

	  // move their centers and also move to the assembly location  ! Modify if cyl is outside brick
	  dCylMoveX = dVCylXYPos(1)+dX;
	  dCylMoveY = dVCylXYPos(2)+dY;
	  dZMove = (dVCylZPos(1)+dVCylZPos(2))/2.0;

	  iGeom_moveEnt(geom, cyl, dCylMoveX,dCylMoveY,dZMove, &err);
	  CHECK("Couldn't move cyl.");
	  cyls[m-1] = cyl;
	}

	//set tag on inner most cylinder, search for the full name of the abbreviated Cell Mat
	for(int p=1;p<=m_szAssmMatAlias.GetSize();p++){
	  if(strcmp (szVCylMat(1).c_str(), m_szAssmMatAlias(p).c_str()) == 0){
	    sMatName = m_szAssmMat(p);
	  }
	}
	std::cout << "created: " << sMatName << std::endl;
	tmp_vol1=cyls[0]; //inner most cyl

	iGeom_setData(geom, tmp_vol1, this_tag,
		      sMatName.c_str(), 10, &err);
	CHECK("setData failed");

	int err = Get_Max_Surf(tmp_vol1, &max_surf);
	if(max_surf !=NULL){
	  sMatName+="_surface";
	  iGeom_setData(geom, max_surf, this_tag,
			sMatName.c_str(), sMatName.size(), &err);
	  CHECK("setData failed");
	  std::cout << "created: " << sMatName << std::endl;     
	}

	// other cyl annulus after substraction
	for (int b=nRadii; b>1; b--){  

	  iGeom_copyEnt(geom, cyls[b-2], &tmp_vol, &err);
	  CHECK("Couldn't copy inner duct wall prism.");

	  //subtract tmp vol from the outer most
	  iGeom_subtractEnts(geom, cyls[b-1], tmp_vol, &tmp_new, &err);
	  CHECK("Subtract of inner from outer failed.");
  
	  // now search for the full name of the abbreviated Cell Mat
	  for(int p=1;p<=m_szAssmMatAlias.GetSize();p++){
	    if(strcmp (szVCylMat(b).c_str(), m_szAssmMatAlias(p).c_str()) == 0){
	      sMatName =  m_szAssmMat(p);
	    }
	  }
	  std::cout << "created: " << sMatName << std::endl;
	  // set the name of the annulus
	  iGeom_setData(geom, tmp_new, this_tag,
			sMatName.c_str(),sMatName.size(), &err);
	  CHECK("setData failed");
	  int err = Get_Max_Surf(tmp_new, &max_surf);
	  if(max_surf !=0){

	    sMatName+="_surface";
	    if (max_surf !=0){
	      iGeom_setData(geom, max_surf, this_tag,
			    sMatName.c_str(), sMatName.size(), &err);
	      CHECK("setData failed");
	      std::cout << "created: " << sMatName << std::endl;
	    }
	  }

	  // copy the new into the cyl array
	  cyls[b-1] = tmp_new;
	}
      }
    }
  }
  return 1;
}

void CNrgen:: ComputePinCentroid(int nTempPin, CMatrix<std::string> MAssembly, 
				 int m, int n, double &dX, double &dY, double &dZ)
// ---------------------------------------------------------------------------
// Function: displays error messages related to input data
// Input:    error code
// Output:   none
// ---------------------------------------------------------------------------
{
  int nTempPin1 = 0, nTempPin2 = 0, nInputLines;
  std::string szVolId, szVolAlias;
  if(m_szGeomType == "hexagonal"){
    double dP, dZ;
    m_Pincell(nTempPin).GetPitch(dP, dZ);   

    if (m < m_nPin){
      dX = (m_nPin - n + 1)*dP/2.0 + n*dP/2.0 + (n-1)*dP - (m-1)*dP/2.0;
      dY = (m-1)*(0.5*dP/sin(pi/3.0) + 0.5*dP*sin(pi/6.0)/sin(pi/3.0));
    }
    else{
      dX = (m_nPin - n + 1)*dP/2.0 + n*dP/2.0 + (n-1)*dP - (2*m_nPin - m -1)*dP/2.0;
      dY = (m-1)*(0.5*dP/sin(pi/3.0) + 0.5*dP*sin(pi/6.0)/sin(pi/3.0)); 
    }     
  }
  if(m_szGeomType == "cartesian"){
    double dPX, dPY, dPZ, dPX1, dPY1, dPZ1, dPX2, dPY2, dPZ2;
    m_Pincell(nTempPin).GetPitch(dPX, dPY, dPZ);
    if (n==1){
      dX = 0;
      if(m==1)
	dY = 0;
    }
    else{
      dX+= dPX/2.0;
      // find the previous pincell type
      for(int b=1; b<=m_nPincells; b++){
	m_Pincell(b).GetLineOne(szVolId, szVolAlias, nInputLines);
	if(m_Assembly(m,n-1) == szVolAlias)
	  nTempPin1 = b;
      }      
      m_Pincell(nTempPin1).GetPitch(dPX1, dPY1, dPZ1);
      // now add half of X pitch to the previous cells pitch
      dX+= dPX1/2.0;
    }
    if (m > 1 && n==1){
      dY+= dPY/2.0;
      for(int c=1; c<=m_nPincells; c++){
	m_Pincell(c).GetLineOne(szVolId, szVolAlias, nInputLines);
	if(m_Assembly(m-1,n) == szVolAlias)
	  nTempPin2 = c;
      }
      m_Pincell(nTempPin2).GetPitch(dPX2, dPY2, dPZ2);	
      dY+= dPY2/2.0;
    }   
    dZ = 0.0; // moving in XY plane only
  }//if cartesian ends
}

void CNrgen::IOErrorHandler (ErrorStates ECode) const
// ---------------------------------------------------------------------------
// Function: displays error messages related to input data
// Input:    error code
// Output:   none
// ---------------------------------------------------------------------------
{
  std::cerr << '\n';

  if (ECode == PINCELLS) // invalid number of pincells
    std::cerr << "Number of pincells must be >= 1.";
  else if (ECode == INVALIDINPUT) // invalid input
    std::cerr << "Invalid input.";
  else
    std::cerr << "Unknown error ...?";

  std::cerr << '\n' << "Error in input file line : " << m_nLineNumber;
  std::cerr << std::endl;
  exit (1);
}

int CNrgen::TerminateProgram ()
// ---------------------------------------------------------------------------
// Function: terminates the program steps by closing the input/output files
// Input:    none
// Output:   none
// ---------------------------------------------------------------------------
{
  int err;
  iGeom_dtor(geom, &err);
  CHECK( "Interface destruction didn't work properly." );
  // close the input and output files
  m_FileInput.close ();
  m_FileOutput.close ();
  return 1;
}


// print error function definition
static bool print_error( const char* desc, 
                         int err,
                         iGeom_Instance geom,
                         const char* file,
                         int line )
{
  char buffer[1024];
  int err2 = err;
  iGeom_getDescription( geom, buffer, &err2, sizeof(buffer) );
  buffer[sizeof(buffer)-1] = '\0';
  
  std::cerr << "ERROR: " << desc << std::endl
            << "  Error code: " << err << std::endl
            << "  Error desc: " << buffer << std::endl
            << "  At        : " << file << ':' << line << std::endl
    ;
  
  return false; // must always return false or CHECK macro will break
}

int CNrgen:: Get_Max_Surf(iBase_EntityHandle cyl, iBase_EntityHandle* max_surf)
// ---------------------------------------------------------------------------
// Function: terminates the program steps by closing the input/output files
// Input:    none
// Output:   none
// ---------------------------------------------------------------------------
{
  // get the surface with max z
  //  iBase_EntityHandle tmp_surf = 0;
  double dZCoor=-1.0e6;
  int err;
  SimpleArray<iBase_EntityHandle> surfs;
  iGeom_getEntAdj( geom, cyl, iBase_FACE, ARRAY_INOUT(surfs), &err );
  CHECK( "Problems getting max surf for rotation." );
  
  SimpleArray<double> max_corn, min_corn;
  iGeom_getArrBoundBox( geom, ARRAY_IN(surfs), iBase_INTERLEAVED, 
                        ARRAY_INOUT( min_corn ),
                        ARRAY_INOUT( max_corn ),
                        &err );
  CHECK( "Problems getting max surf for rotation." );
  for (int i = 0; i < surfs.size(); ++i){
    // first find the max z-coordinate
    if(min_corn[3*i+2]==max_corn[3*i+2]){
      if(min_corn[3*i+2] > dZCoor){
	dZCoor = min_corn[3*i+2];
	*max_surf = surfs[i];
      }
    }
  }
  if (0 == max_surf) {
    std::cerr << "Couldn't find max surf for rotation." << std::endl;
    return false;
  }
  return 0;
} 