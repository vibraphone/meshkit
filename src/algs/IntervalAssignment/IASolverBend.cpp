// IASolverBend.cpp
// Interval Assignment for Meshkit
//
#include "IASolverBend.hpp"
#include "IAData.hpp"
#include "IPData.hpp"
#include "IPBend.hpp"
#include "IASolution.hpp"
#include "IABendNlp.hpp"

#include <stdio.h>
#include <math.h>
#include <limits.h>

#include "IpIpoptApplication.hpp"

namespace MeshKit 
{
    
IASolverBend::IASolverBend(const IAData * ia_data_ptr, IASolution *relaxed_solution_ptr, 
  const bool set_silent) 
  : IASolverToolInt(ia_data_ptr, relaxed_solution_ptr, true), evenConstraintsActive(false),
  silent(set_silent), debugging(true)
{ 
  ip_data(new IPData);
  // initialize copies relaxed solution, then we can overwrite relaxed_solution_pointer with our integer solution 
  ip_data()->initialize(relaxed_solution_ptr->x_solution); 
}

/** default destructor */
IASolverBend::~IASolverBend() 
{
  delete ip_data();
}
    
bool IASolverBend::solve_nlp() // IABendNlp *mynlp
{
  if (debugging)
  {
    printf("IASolverBend::solve_nlp() == ");        
    printf("BEND problem formulation\n");
    printf("Attempting to find a naturally-integer solution by linearizing and bending the objective function at integer values.\n");
    printf("x = sum of positive and negative deltas around the floor of the relaxed solution. Deltas are within [0,1]. Deltas are dynamically added. Objective is linear function of weighted deltas, randomized to break ties.\n");
  }
  
  // solver setup  
  Ipopt::SmartPtr<Ipopt::IpoptApplication> app = IpoptApplicationFactory();
/* try leaving defaults
  // convergence parameters
  // see $IPOPTDIR/Ipopt/src/Interfaces/IpIpoptApplication.cpp
  // our real criteria are: all integer, constraints satisfied. How to test the "all_integer" part?
  app->Options()->SetNumericValue("tol", 1e-6); //"converged" if NLP error<this, default is 1e-7. Obj are scaled to be >1, so e-2 is plenty // was 1e-2
  app->Options()->SetNumericValue("max_cpu_time", sqrt( iaData->num_variables() ) ); // max time allowed in seconds
  app->Options()->SetIntegerValue("max_iter", 3 * (10 + iaData->num_variables() ) ); // max number of iterations
  // app->Options()->SetNumericValue("primal_inf_tol", 1e-2 ); 
  app->Options()->SetNumericValue("dual_inf_tol", 1e-2 ); // how close to infeasibility? // was 1e-2
  app->Options()->SetNumericValue("constr_viol_tol", 1e-2 ); // by how much can constraints be violated?
  app->Options()->SetNumericValue("compl_inf_tol", 1e-6 ); // max norm of complementary condition // was 1e-2
  
  // second criteria convergence parameters: quit if within this tol for many iterations
  // was  app->Options()->SetIntegerValue("acceptable_iter", 4 + sqrt( iaData->num_variables() ) ); //as "tol"
  app->Options()->SetNumericValue("acceptable_tol", 1e-6 ); //as "tol" was 1e-1
  
  app->Options()->SetStringValue("mu_strategy", "adaptive");
  // print level 0 to 12, Ipopt default is 5
  const int print_level = (silent) ? 0 : 1;  // simple info is 1, debug at other values
  app->Options()->SetIntegerValue("print_level", print_level);  
  // uncomment next line to write the solution to an output file
  // app->Options()->SetStringValue("output_file", "IA.out");  
  // The following overwrites the default name (ipopt.opt) of the options file
  // app->Options()->SetStringValue("option_file_name", "IA.opt");
  
  */
  const int print_level = 2;  // simple info is 1, debug at other values
  app->Options()->SetIntegerValue("print_level", print_level);  
  
  // Intialize the IpoptApplication and process the options
  Ipopt::ApplicationReturnStatus status;
  status = app->Initialize();
  if (status != Ipopt::Solve_Succeeded) {
    if (!silent)
      printf("\n\n*** Error during initialization!\n");
    return (int) status;
  }
  
  bool try_again = false; // only try again if we didn't converge and want to spend more time
  int iter = 0;
  
  // print();
  bool solution_ok = false;
  
  myianlp->even_constraints_active( evenConstraintsActive );
  
  do {
    if (debugging)
    {
      print();
      printf("%d Bend iteration\n", iter );
      // build the hessian, evaluate it and f at the current solution?
    }
      
    // Ask Ipopt to solve the problem
    status = app->OptimizeTNLP(myianlp); // the inherited IANlp
    
    // see /CoinIpopt/build/include/coin/IpReturnCodes_inc.h
    /*
    Solve_Succeeded=0,
    Solved_To_Acceptable_Level=1,
    Infeasible_Problem_Detected=2,
    Search_Direction_Becomes_Too_Small=3,
    Diverging_Iterates=4,
    User_Requested_Stop=5,
    Feasible_Point_Found=6,
    
    Maximum_Iterations_Exceeded=-1,
    Restoration_Failed=-2,
    Error_In_Step_Computation=-3,
    Maximum_CpuTime_Exceeded=-4,
    Not_Enough_Degrees_Of_Freedom=-10,
    Invalid_Problem_Definition=-11,
    Invalid_Option=-12,
    Invalid_Number_Detected=-13,
    
    Unrecoverable_Exception=-100,
    NonIpopt_Exception_Thrown=-101,
    Insufficient_Memory=-102,
    Internal_Error=-199
     */

    bool solved_full = false;
    bool solved_partial = false;
    bool solver_failed = false;
    bool bad_problem = false;

    switch (status) {
      case Ipopt::Solve_Succeeded:
      case Ipopt::Solved_To_Acceptable_Level:
      case Ipopt::Feasible_Point_Found:
        solved_full = true;
        break;
      case Ipopt::Maximum_Iterations_Exceeded:
      case Ipopt::User_Requested_Stop:
      case Ipopt::Maximum_CpuTime_Exceeded:
        solved_partial = true;
        break;
      case Ipopt::Infeasible_Problem_Detected:
      case Ipopt::Not_Enough_Degrees_Of_Freedom:
      case Ipopt::Invalid_Problem_Definition:
      case Ipopt::Invalid_Option:
      case Ipopt::Invalid_Number_Detected:
        bad_problem = true;
        break;
      case Ipopt::Search_Direction_Becomes_Too_Small:
      case Ipopt::Restoration_Failed:
      case Ipopt::Diverging_Iterates:
      case Ipopt::Error_In_Step_Computation:
      case Ipopt::Unrecoverable_Exception:
      case Ipopt::NonIpopt_Exception_Thrown:
      case Ipopt::Insufficient_Memory:
      case Ipopt::Internal_Error:        
        solver_failed = true;
        break;
        
      default:
        break;
    }
  
    if (!silent)
    {
      if (solved_full) {
        printf("\n\n*** BendNlp solved!\n");
      }
      else {
        printf("\n\n*** BendNlp FAILED!\n");
      }
    }
    
    if (debugging)
    {
      printf("\nChecking solution.\n");
      bool integer_sat = solution_is_integer(true);
      bool even_sat = even_constraints( false, true);
      bool equal_sat = equal_constraints( false, true );
      printf("Bend solution summary, %s, equal-constraints %s, even-constraints %s.\n", 
             integer_sat ? "integer" : "NON-INTEGER",
             equal_sat ? "satisfied" : "VIOLATED", 
             even_sat ? "satisfied" : "VIOLATED" );
    }
    try_again = false; 
  
    if ( solved_full || solved_partial )
    {
      return true;
    }
    else
    {
      // todo: tweak the problem and try again
      return false;
    }
    
  } while (try_again);
  
  return solution_ok;
  
}

bool IASolverBend::round_solution()
{
  IASolution nlp_solution;
  nlp_solution.x_solution = ia_solution()->x_solution; // vector copy
  IASolverToolInt sti( ia_data(), &nlp_solution );
  sti.round_solution();
  if (debugging)
    printf("Checking rounded bend solution.\n");
  if (sti.equal_constraints(false, debugging) && sti.even_constraints(false, debugging) )
  {
  if (debugging)
    printf("Rounding worked.\n");
    
    // rounding was a valid integer solution
    ia_solution()->x_solution.swap( nlp_solution.x_solution );
    // ia_solution()->obj_value is no longer accurate, as it was for the non-rounded solution
    return true;
  }
  return false;
}

void IASolverBend::cleanup()
{
  // the solution includes the delta values.
  // remove those
  iaSolution->x_solution.resize( (unsigned) iaData->num_variables());
}


// the objective function we should use for deriving the weights
double IASolverBend::f_x_value( double I_i, double x_i ) const
{
  return myianlp->f_x_value(I_i, x_i);
}

  
double IASolverBend::fpow(double f) const
{
  return f*f*f; // f^3
  // todo, experiment with squaring, or less
}
  
 
  /* Idea: a form of IARoundingNlp with larger variable bounds, but still with a natural integer solution.
   x in [1..inf]
   xr = x optimal relaxed solution with objective function fnlp, see IANlp.xpp 
   f is piecewise linear, with corners at integer values. f slopes are unique (we hope)
   Slope definitions
   for x between xl = floor xr and xh = ceil xr, we use the difference in fnlp between xl and xh
   
   case A. xr > ceil g, g is goal I[i]
   for x above xh, 
   let h+ be fnlp ( xh+1 ) - fnlp ( xh )
   let kp be the number of intervals x is above xh 
   then slope = floor kp * h+
   for x below xl, h- = sqrt(11) / 5 h+, and slope = floor km * h-
   all this is weighted by some unique weight
   
   case B. xr in [ floor g, ceil g] 
   h+ = fnlp ( xh+1 ) - fnlp ( xh )
   h- = fnlp ( xl-1 ) - fnlp ( xl ), not used if xl == 1
   
   case C. xr < floor g
   h- = fnlp ( xl-1 ) - fnlp ( xl )
   h+ = sqrt(10)/5 h-
   If g < 2, then h- is unused, and h+ = as in case B
   
   // representation:
   h0 is weights 0..n-1
   h+ is weights n..2n-1
   h- is weights 2n..
   
   */

  
/* try parabolic constraints instead
 
void IASolverBend::add_bend_sum_weights(unsigned int i, const double factor)
{
  // scaling issue, 
  // in order to overcome the slopes of the integers, the minimum slope should be larger
  // than the largest active slope, where active means the weight of the bend delta at the current solution
  
  IPBend &bend = bendData.bendVec[i];
  // designed so xl is the "ideal"
  // const double xl = bend.xl; 
  
  const double s = even_value(i);
  const double g = s/2.;

  // plusses
  for (int j = 0; j < bend.numDeltaPlus; ++j)
  {
    double w = bendData.maxActiveVarWeight;
    // if current sum is between xl and xl + 1, underweight the first delta
    if ( (j == 0) && (g > bend.xl) )
    {
      double f = ceil(g) - g;
      assert( f >= 0.499 ); // xl was chosen to be the closest integer to s/2
      w *= f;
    }
    w *= factor * 2.2 * sqrt( 1.1 * j + 1.1 );    
    weights.push_back(w);
  }
  
  // minuses
  for (int j = 0; j < bend.numDeltaMinus; ++j)
  {
    double w = bendData.maxActiveVarWeight;
    if ( (j == 0) && (g < bend.xl) )
    {
      double f = g - floor(g);
      assert( f >= 0.499 ); // xl was chosen to be the closest integer to s/2
      w *= f;
    }
    w *= factor * 2.1 * sqrt( 1.02 * j + 1.02 );    
    weights.push_back(w);
  }
}
  
 */
  
void IASolverBend::add_bend_weights(unsigned int i)
{
  // shorthands
  IPBend &bend = bendData.bendVec[i];
  const double xl = bend.xl;
  const double g = iaData->I[i]; // goal
  
  // current x solution for finding active weight
  const double x = iaSolution->x_solution[i]; 

  // plusses
  for (int j = 0; j < bend.numDeltaPlus; ++j)
  {
    double xlit = xl + j;
    double xbig = xlit + 1.;
    // special handling when crossing goal to avoid zero slope
    if ( xlit < g && xbig > g )
    {
      if ( g - xlit < xbig - g) 
        xlit = g;
      else
        xbig = g;
    }
    const double flit = f_x_value(g, xlit); 
    const double fbig = f_x_value(g, xbig);
    double w = fpow(fbig) - fpow(flit); 
    weights.push_back(w);

    // active? or nearly active
    if (x <= xbig + 1. && x >= xlit)
    {
      // biggest?
      if (fabs(w) > bendData.maxActiveVarWeight)
        bendData.maxActiveVarWeight = fabs(w);
    }
  }
  
  // minuses
  for (int j = 0; j < bend.numDeltaMinus; ++j)
  {
    double xbig = xl - j;   
    double xlit = xbig - 1.;
    assert(xlit >= 1.); // if this fails, then the numDeltaMinus is too large
    
    // special handling when crossing goal to avoid zero slope
    if ( xlit < g && xbig > g )
    {
      if ( g - xlit < xbig - g) 
        xlit = g;
      else
        xbig = g;
    }
    const double flit = f_x_value(g, xlit); 
    const double fbig = f_x_value(g, xbig);
    double w = - fpow(fbig) + fpow(flit);
    weights.push_back(w);

    // active? or nearly active
    if (x <= xbig && x >= xlit - 1.)
    {
      // biggest?
      if (fabs(w) > bendData.maxActiveVarWeight)
        bendData.maxActiveVarWeight = fabs(w);
    }
  }

}
  

void IASolverBend::initialize_ip_bends()
{
  // problem layout:
  // vars:
  //   x variables that are supposed to be integer
  //   sums that are supposed to be even
  //   for i = 0 .. iaData->num_variables()
  //     x[i] delta pluses
  //     x[i] delta minuses
  // 
  // constraints
  //    base-problem
  //      equal
  //      even
  //    x=deltas, x = xl(const) + deltasplus - deltasminus, <==> x - deltasplus + deltasminus = -xl
  //    s=deltas
  //
  bendData.numSumVars = (int) iaData->sumEvenConstraints.size();
  bendData.sumVarStart = iaData->num_variables();
  
  // loop over the vars, 
  // finding upper and lower rounding values
  // assign weights
  bendData.bendVec.resize( iaData->num_variables() ); // + bendData.numSumVars if we want to do those using bends rather than waves
  weights.reserve(bendData.bendVec.size()*4);
  
  bendData.maxActiveVarWeight = 0.;
  
  int d_start = iaData->num_variables();
  for (int i = 0; i < iaData->num_variables(); ++i)
  {
    IPBend &bend = bendData.bendVec[i]; // shorthand
    bend.deltaIStart = d_start;
    
    double x = ipData->relaxedSolution[i];
    // fix any out-of-bounds roundoff issue
    if ( x < 1. ) 
      x = 1.;
    double xl = bend.xl = floor(x);
    assert(xl >= 1.);
    // to do, experimenting with starting with +2, -1 
    if ( x - floor(x) > 0.5 )
    {
      bend.numDeltaPlus = 2;
      bend.numDeltaMinus = 1; 
    }
    else
    {
      bend.numDeltaPlus = 1;
      bend.numDeltaMinus = 1;      
    }

    /**/
    // debug, test negative deltas by starting with an xl that's too high
    x += 2.;
    xl = bend.xl = floor(x);
    bend.numDeltaPlus = 1;
    bend.numDeltaMinus = 1;      
    /**/

    // avoid minus deltas that allow the x solution to be less than 1
    if (bend.numDeltaMinus > xl - 1. )
      bend.numDeltaMinus = xl - 1;
    
    // always have at least one deltaMinus and one deltaPlus, so that the full range of x can be represented
    assert( ( xl == 1 ) || (bend.numDeltaMinus >= 1) );
    assert( bend.numDeltaPlus >= 1 );
    
    d_start += bend.num_deltas();

    add_bend_weights( i );
  }
  
  /* handle the sum-evens using parabolic constraints intead.

   // initialize all the sum-evens to have no bends - try to enforce those after iteration 1
  // as the initial int rounding could change the sum values a lot,
  // and unlike the x's there is no intrinsic goal for them.

  for (int i = 0; i < bendData.numSumVars; ++i)
  {
    IPBend &bend = bendData.bendVec[i + bendData.sumVarStart]; // shorthand
    // get current sum-even-value
    double s = even_value(i);
    bend.xl = floor( s / 2. + 0.5 ); // closest integer to s/2
    bend.numDeltaMinus = 1;
    bend.numDeltaPlus = 1;
    bend.deltaIStart = d_start;

    add_bend_sum_weights( i, 0. );

    d_start += bend.num_deltas();
  }
   */
  
  // gather the weights in a vector 
  
  // uniquify the weights
  weights.uniquify(1., 1.e6); // 1.e4 ??
  
  
  // also do that later in a lazy fasion after we see which vars are not integer
  
  
}
  
 
bool IASolverBend::update_ip_bends()
{

  int d_start = iaData->num_variables();
  weights.clear();
  bool new_bend = false;
  bool randomized = false;
  
  for (int i = 0; i < iaData->num_variables(); ++i)
  {
    IPBend &bend = bendData.bendVec[i]; // shorthand

    std::vector<int> dp_non_int, dm_non_int;
    
    // delta non-integer? 
    for (int j = 0; j < bend.numDeltaPlus; ++j)
    {
      const int di = bend.deltaIStart + j;
      const double xp = iaSolution->x_solution[ di ];
      if (!is_integer(xp))
      {
        dp_non_int.push_back(j);
      }        
    }
    for (int j = 0; j < bend.numDeltaMinus; ++j)
    {
      const int di = bend.deltaIStart + bend.numDeltaPlus + j;
      const double xm = iaSolution->x_solution[ di ];
      if (!is_integer(xm))
      {
        dm_non_int.push_back(j);
      }        
    }

    
    // delta > 1? 
    // add more deltas
    if (bend.numDeltaPlus > 0)
    {
      const int di = bend.deltaIStart + bend.numDeltaPlus - 1;
      const double xp = iaSolution->x_solution[ di ]; 
      if (xp > 1.01)
      {
        bend.numDeltaPlus += floor(xp + 0.1);
        new_bend = true;
      }
    }
    if (bend.numDeltaMinus > 0)
    {
      const int di = bend.deltaIStart + bend.numDeltaPlus + bend.numDeltaMinus - 1;
      const double xm = iaSolution->x_solution[ di ]; 
      if (xm > 1.01 && bend.numDeltaMinus < bend.xl - 1)
      {
        bend.numDeltaMinus += floor(xm + .1);
        if ( bend.numDeltaMinus > bend.xl - 1 )
          bend.numDeltaMinus = bend.xl - 1;          
        new_bend = true;
      }
    }

    // generate new deltas and weights
    add_bend_weights(i);

    bend.deltaIStart = d_start;
    assert( bend.deltaIStart + bend.num_deltas() == (int) weights.size() + iaData->num_variables());
    d_start += bend.num_deltas();
    
    // if delta non-integer? 
    // randomize weight w/ excluded middle
    for (unsigned int j = 0; j < dp_non_int.size(); ++j)
    {
      int k = bend.deltaIStart - iaData->num_variables() + dp_non_int[j];
      // this might make weights tend to zero. Revisit later
      weights[ k ] *= 1. + 0.2 * IAWeights::rand_excluded_middle();
    }
    for (unsigned int j = 0; j < dm_non_int.size(); ++j)
    {
      int k = bend.deltaIStart + bend.numDeltaPlus - iaData->num_variables() + dm_non_int[j];
      // this might make weights tend to zero. Revisit later
      weights[ k ] *= 1. + 0.2 * IAWeights::rand_excluded_middle();
    }
    

  }
  
   weights.uniquify(1., 1e6); // 1e4?

  // return if something changed
  return new_bend || randomized;

}
  
bool IASolverBend::solve()
{
  if (debugging)
  {
  }

  myianlp = new IABendNlp(iaData, ipData, &bendData, iaSolution, &weights, silent);
  Ipopt::SmartPtr<Ipopt::TNLP> mynlp = myianlp; // Ipopt requires the use of smartptrs!

  // set initial ip bends from relaxed solution
  initialize_ip_bends();

  int iter = 0;
  bool try_again = true;
  bool success = false;
  evenConstraintsActive = false;
  const int max_last_iter = 4 * ( 2 + iaData->num_variables() );
  const int max_first_iter = max_last_iter / 2;
  do 
  {
  
    // call the nlp solver
    bool solved = solve_nlp();
    
    try_again = false;
    if (solved)
    {
      
      // update bends based on solution
      bool changed = update_ip_bends();
      if (changed)
      {
        try_again = true;
      }
      // try again if sum-evens not already satisfied
      if (!even_constraints())
        try_again = true;
      // if nothing changed, or we've had enough initial iterations,
      // then activate the sum-even parabola constraints
      if (!changed || (iter > max_first_iter))
      {
        evenConstraintsActive = true;
      }
    }
    // avoid infinite loop if the method isn't working
    ++iter;
    if ( iter > max_last_iter )
      try_again = false;
    
  }
  while (try_again);
    
  cleanup();
  
  success = solution_is_integer() && all_constraints();
  if (success)
  {
    if (!silent)
      printf("IASolverBend produced integer and even solution\n");
  }
  else
  {
    return false; 
  }
  return success;
}

} // namespace MeshKit
