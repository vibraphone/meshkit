// IASolver.cpp
// Interval Assignment for Meshkit
//
#include "IASolver.hpp"
#include "IPData.hpp"
// #include "IAMilp.hpp"
#include "IASolverRelaxed.hpp"
#include "IASolverInt.hpp"

#include <cstdlib>
#include <stdio.h>
#include <math.h>
#include <limits.h>

namespace MeshKit 
{
    
IASolver::IASolver(IAData *ia_data_ptr, IASolution *ia_solution_ptr)
  : IASolverToolInt( ia_data_ptr, ia_solution_ptr ),
  debugging(true) 
  {}

/** default destructor */
IASolver::~IASolver() {}


bool IASolver::solve_relaxed()
{
  IASolverRelaxed relaxed(ia_data(), ia_solution(), !debugging); 
  bool succeeded=relaxed.solve();
  return succeeded;
}  


/* milp solution is too slow
bool IASolver::solve_int()
{
  IAMilp milp(this,this);
  bool succeeded=milp.solve();
  return succeeded;
}  
*/

bool IASolver::solve_int()
{
  // set relaxed solution to this's solution (from solve_relaxed)
  IASolverInt solver_int(ia_data(), ia_solution(), !debugging);
  bool succeeded=solver_int.solve();
  return succeeded;
}  

bool IASolver::solve_even()
{
  bool succeeded=true;
  return succeeded;
}  


bool IASolver::solve()
{

  if (debugging)
  {
    printf("Solving subproblem %p\n", this);
    print_problem();
  }
  
  // todo: subdivide problem into independent sub-problems for speed
  
  bool relaxed_succeeded = solve_relaxed();
  bool int_succeeded = relaxed_succeeded && solve_int();
  bool even_succeeded = int_succeeded && solve_even();
  
  if (debugging)
  {
    printf("==========IA summary:\n"); 
    if (relaxed_succeeded)
    {
      printf(" relaxed succeeded\n");
      if (int_succeeded)
      {
        printf(" integer succeeded\n");
        if (even_succeeded)
        {
          printf(" even succeeded\n");
        }
        else
        {
          printf(" even failed\n");        
        }
      }
      else
      {
        printf(" integer failed\n");        
      }
    }
    else
    {
      printf(" relaxed failed\n");
    }  
    print_solution();
  }
	  
  return even_succeeded;
}


} // namespace MeshKit
