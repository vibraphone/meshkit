// IAInterface.cpp
// Interval Assignment Data for Meshkit
// Interface to the rest of Meshkit

#include "meshkit/IAInterface.hpp"
#include "IASolver.hpp"
#include "meshkit/ModelEnt.hpp" // from MeshKit
#include <vector>
#include <cstdio>
#include <assert.h>
#include <math.h>

namespace MeshKit 
{

moab::EntityType IAInterface_types[] = {moab::MBMAXTYPE};
const moab::EntityType* IAInterface::output_types() 
  { return IAInterface_types; }
    
void IAInterface::setup_this()
{
  ; //nothing for now
}
    
IAVariable *IAInterface::create_variable( ModelEnt* model_entity )
{
  // already exists?
  if (model_entity && model_entity->ia_var())
  {
    // return without setting values
    return model_entity->ia_var();
  }
 
  return create_variable( model_entity, IAVariable::SOFT, 0. );  
}


IAVariable *IAInterface::create_variable( ModelEnt* model_entity, IAVariable::Firmness set_firm, double goal_value)
{
  IAVariable *ia_var = NULL;

  // already exists?
  if (model_entity && model_entity->ia_var())
  {
    // set new values and return
    ia_var = model_entity->ia_var();
    ia_var->set_firmness(set_firm);
    ia_var->set_goal(goal_value);
    return ia_var;
  }
      
  ia_var = new IAVariable(model_entity, set_firm, goal_value);
  // add to set, giving the end of the set as a hint of where it goes
  variables.insert( ia_var);

  return ia_var;
}

IAInterface::IAVariableVec IAInterface::make_constraint_group( const MEVec &model_entity_vec )
{
  IAVariableVec result( model_entity_vec.size() );
  for (unsigned int i = 0; i < model_entity_vec.size(); ++i)
  {
    assert(model_entity_vec[i]);
    result[i] = model_entity_vec[i]->ia_var();
  }
  return result;
}

void IAInterface::destroy_variable( IAVariable* ia_variable )
{
  if (!ia_variable)
    return;
  // model_entity shouldn't point to ia_variable anymore
  ModelEnt *me = ia_variable->get_model_ent();
  if (me && me->ia_var() == ia_variable)
    me->ia_var(NULL);
  
  variables.erase( ia_variable );
  delete ia_variable;    
}

IAInterface::~IAInterface()
{
  // destroy remaining variables
  // in reverse order for efficiency
  while (!variables.empty())
  {
    destroy_variable( * variables.rbegin() );
  }

}

void IAInterface::constrain_sum_even( const IAVariableVec &sum_even_vars )
{
  sumEvenConstraints.push_back( sum_even_vars ); // vector copy
}

void IAInterface::constrain_sum_equal( const IAVariableVec &side_one, const IAVariableVec &side_two )
{
  sumEqualConstraints1.push_back(side_one); //vector copy of side_one
  sumEqualConstraints2.push_back(side_two); //vector copy of side_two
}

void IAInterface::make_0_to_nm1( IndexSet &index_set, const int k )
{
  for (int i = 0; i < k; ++i)
    index_set.insert( i);
}

int IAInterface::variable_to_index(const IAVariable* var) const
{
  VariableSet::const_iterator var_pos = variables.find( const_cast<IAVariable*>(var) );
  int v = std::distance(variables.begin(), var_pos);
  assert( v >= 0 );
  assert( v < (int)variables.size() ); // == size means it wasn't found
  return v;
}

IAVariable *IAInterface::index_to_variable(int ind) const
{
  assert(ind < (int)variables.size());
  VariableSet::const_iterator var_pos = variables.begin();
  for (int i = 0; i < ind; i++) var_pos++;
  return *var_pos;
}

void IAInterface::get_constraint_variable_indices( IndexSetVec &constraint_variables, 
                                                  IndexSetVec &variable_constraints,
                                                  const int i_start, 
                                                  const VariableVecVec &variable_vec_vec )
{
  unsigned int j;
  int i;
  for (j = 0, i = i_start; j < variable_vec_vec.size(); ++j, ++i)
  {
    for (unsigned int k = 0; k < variable_vec_vec[j].size(); ++k )
    {
      const IAVariable *var = variable_vec_vec[j][k];
      const int v = variable_to_index( var ); 
      constraint_variables[i].insert( v );        
      assert( v < (int)variable_constraints.size() );
      variable_constraints[v].insert( i );
    }
  }
}

void IAInterface::find_variable_dependent_set( const int variable_j, 
                                              const IndexSetVec &constraint_variables,
                                              const IndexSetVec &variable_constraints,
                                              IndexSet &constraint_set, IndexSet &variable_set, 
                                              IndexSet &sub_constraint_set, IndexSet &sub_variable_set)
{
  // return if we've already added this variable
  IndexSet::iterator j = variable_set.find(variable_j);
  if ( j == variable_set.end() )
    return;
  
  // add the variable to the sub-problem
  sub_variable_set.insert(*j);
  
  // remove the variable from the big problem
  variable_set.erase(j);
  
  // recursively find the dependent constraints
  for (IndexSet::iterator i = variable_constraints[variable_j].begin(); i != variable_constraints[variable_j].end(); ++i)
    find_constraint_dependent_set( *i, constraint_variables, variable_constraints, 
                                  constraint_set, variable_set, sub_constraint_set, sub_variable_set);

}


void IAInterface::find_constraint_dependent_set( const int constraint_i, 
                                                const IndexSetVec &constraint_variables,
                                                const IndexSetVec &variable_constraints,
                                                IndexSet &constraint_set, IndexSet &variable_set, 
                                                IndexSet &sub_constraint_set, IndexSet &sub_variable_set)
{
  // return if we've already added this constraint
  IndexSet::iterator i = constraint_set.find(constraint_i);
  if ( i == constraint_set.end() )
    return;
  
  // add the constraint to the sub-problem
  sub_constraint_set.insert(*i);

  // remove the constraint from the big problem
  constraint_set.erase(i);

  // recursively find the dependent variables
  for (IndexSet::iterator j = constraint_variables[constraint_i].begin(); j != constraint_variables[constraint_i].end(); ++j)
    find_variable_dependent_set( *j, constraint_variables, variable_constraints, 
                                constraint_set, variable_set, sub_constraint_set, sub_variable_set);
}


void IAInterface::subdivide_problem(std::vector<IASolver*> &subproblems)
{

    /* todo: get subdivision working from src code in progress and copy it over */

   // placeholder - just make one subproblem
  IASolver *sub_problem = new IASolver();
  subproblems.push_back(sub_problem);

  // this grabs the whole problem and converts it
  // variables numbered 0..n
  for(VariableSet::const_iterator i = variables.begin(); i != variables.end(); ++i)
  {
    IAVariable *v = *i;
    sub_problem->I.push_back( v->goal );
  }
  // equal constraints
  for (unsigned int i = 0; i < sumEqualConstraints1.size(); ++i)
  {
    std::vector<int> side_1, side_2;
    for (unsigned int j = 0; j < sumEqualConstraints1[i].size(); ++j)
    {
      IAVariable *v = sumEqualConstraints1[i][j];
      int index = variable_to_index(v);
      side_1.push_back(index);
    }
    for (unsigned int j = 0; j < sumEqualConstraints2[i].size(); ++j)
    {
      IAVariable *v = sumEqualConstraints2[i][j];
      int index = variable_to_index(v);
      side_2.push_back(index);
    }
    sub_problem->constrain_opposite_side_equal(side_1, side_2, 0.);
  }
  // even constraints
  for (unsigned int i = 0; i < sumEvenConstraints.size(); ++i)
  {
    std::vector<int> side;
    for (unsigned int j = 0; j < sumEvenConstraints[i].size(); ++j)
    {
      IAVariable *v = sumEvenConstraints[i][j];
      int index = variable_to_index(v);
      side.push_back(index);
    }
    sub_problem->constrain_sum_even(side, 0.);
  }

}

bool IAInterface::solve_subproblem( IASolver *subproblem )
{
  return subproblem->solve();
}

void IAInterface::assign_solution( IASolver *subproblem )
{
  // assign solution value from subproblem to model entities
  for (unsigned int i = 0; i < subproblem->x_solution.size(); ++i)
  {
    const double x = subproblem->x_solution[i];
    // map index i to IAVariable, which will be different when we have non-full subproblems
    IAVariable *v = index_to_variable( i );
    assert(v);
    assert( x > 0. );
    v->solution = floor( x + 0.5 );
  }
}

void IAInterface::execute_this()
{
  std::vector<IASolver*> subproblems;
  subdivide_problem(subproblems);
  for (unsigned int i = 0; i < subproblems.size(); ++i)
  {
    IASolver* p = subproblems[i];
    if ( solve_subproblem(p) )
      assign_solution(p);
    else
    {
      ; // some error statement
    }
    delete p;
  }
  subproblems.clear();
  return;
}

} // namespace MeshKit
