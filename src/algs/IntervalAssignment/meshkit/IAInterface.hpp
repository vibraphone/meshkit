// IAInterface.hpp

#ifndef MESHKIT_IA_INTERFACE_HP
#define MESHKIT_IA_INTERFACE_HP

// #include "ModelEnt.hpp"
#include "meshkit/IAVariable.hpp"
#include "meshkit/Types.hpp"
#include "meshkit/Error.hpp"
#include "meshkit/MeshScheme.hpp"
#include "meshkit/ModelEnt.hpp"
#include "moab/Interface.hpp"

#include <set>
#include <vector>

namespace MeshKit {

class ModelEnt;
class IASolver;

/** \class IAInterface IAInterface.hpp "meshkit/IAInterface.hpp"
 * \brief The class used in MeshKit for Interval Assignment.
 *
 * Instances of this class are tools. The problem is to set up and solving the number of mesh edges 
 * to place on model entities and entity features. When solved, each curve can be meshed 
 * independently, and treated as fixed when meshing each surface or volume containing it. 
 * Different mesh schemes have different requirements (constraints), such as a mapped surface
 * needs opposite sides to have equal numbers of mesh edges (intervals).
 * The number of mesh edges on one curve is a variable; there may be additional variables.
 * The goal (objective function) is to have mesh edges close to the user-desired sizes.
 *
 * Construction of IAInterface does not cause the construction of variables.
 * Destruction of IAInterface does destroy the underlying variables.
 * IAInterface owns an IAVariable, for creation and deletion.
 * But the ModelEnt can request a variable for itself, and lets the interface know when 
 * it is no longer wanted.
 * ModelEnt keeps handles (pointers) to the variables it cares about.
 * \nosubgrouping
 */
 
class IAInterface : public MeshScheme // register it with SchemeFactory
{
public:
   /** \name Constructor/destructor
     */
    /**@{*/

    /** \brief Constructor; model entity can be missing, in which case it's retrieved or created
     *
     * \param MKCore instance
     * \param MEntVector 
     */
  IAInterface(MKCore *mkcore, const MEntVector &me_vec = MEntVector()) : MeshScheme(mkcore, me_vec){}

      /** \brief Destructor, destroys IAVariables
     */
  virtual ~IAInterface();
      /**@}*/
      
  /** \name Set Up
     */
    /**@{*/

   /** \name Variables
     */
    /**@{*/
   
     /** \brief Create a variable.
     * Variables are created before constraints.
     * It is OK if a variable is not in any constraint, but constraints are defined by their variables.
     * \param ModelEnt* model entity: this variable corresponds to the number of intervals
     *  on that model entity. If NULL, the variable corresponds to anything else that has
     *  meaning to the caller, and the caller will have to keep track of the variable 
     *  in order to access its solution value later.
     */
  IAVariable *create_variable( ModelEnt* model_entity = NULL );
     /** \brief Create a variable and assign it a firmness and goal
     * \param IAVariable::Firmness The required fidelity of the solution to the goal. 
     * If HARD, then it is required that the solution equals the goal; goal should be integer.
     * If SOFT, usual case, try to get close to the goal.
     * If LIMP, we don't care how far the solution is from the goal.
     * \param double goal The desired number of intervals for this variable.
     * The goal may be non-integer, but we assume the solution must be a natural number, 
     * i.e. an integer >= 1.
     */
  // there is a reason we don't provide default parameters here, don't combine with above version.
  IAVariable *create_variable( ModelEnt* model_entity, IAVariable::Firmness set_firmness, double goal_value);
  
    /** \brief Get const_iterators over the variables. May be slow to iterate.
	*/
  typedef std::set<IAVariable*> VariableSet;
  VariableSet::const_iterator variables_begin() const {return variables.begin();}
  VariableSet::const_iterator variables_end() const {return variables.end();}
  
     /** \brief Destroy a variable. If a variable is not explicitly destroyed, it will be
     * destroyed on IAInterface tool destruction.
     */
  void destroy_variable( IAVariable* ia_variable );
      /**@}*/

     /** \name Constraints
     */
    /**@{*/

     /** \brief Containers for variables for specifying constraints.
     */  
  typedef std::vector<ModelEnt*> MEVec;
  typedef std::vector<IAVariable*> IAVariableVec;
     /** \brief Convert container of ModelEnts to a container of IAVariables.
     * MEVec is an indirect way of specifying the model entities's variables.
     */   
    // convert vector of ModelEntities into vector of IAVariables
  IAVariableVec make_constraint_group( const MEVec &model_entity_vec );
  
      /** \brief Constrain that the sum of the number of intervals 
      * on one side is equal to the number on the other side. E.g. when mapping a surface,
      * the opposite sides require equal intervals.
      */
  void constrain_sum_equal( const IAVariableVec &side_one, const IAVariableVec &side_two );
      /** \brief Constrain that the sum of the number of intervals is an even number, 
      * i.e. 2k for some integer k. E.g. for the curves bounding an unstructured quad mesh. 
      */
  void constrain_sum_even( const IAVariableVec &sum_even_vars );
      /** \brief More constraint types may be implemented here.
      */
  //... additional constraint types...
     /**@}*/
    /**@}*/

      /** \brief Main function that graph calls. Inherited from MeshScheme. 
      */
  virtual void setup_this();

  /** \name Solve the problem
     */
    /**@{*/

      /** \brief Main function that graph calls. Inherited from MeshScheme. 
      * find solution satisfying all the constraints
      * assign the solution to the variables
      * May be unsuccessful if the problem is over-specified. 
      * (how can callers detect failure? exception throw? no valid solution value in variables?) 
      */
  virtual void execute_this();
    /**@}*/

  /**\brief Get class name */
  static const char* name() 
    { return "IntervalAssignment"; }

  /**\brief Function returning whether this scheme can mesh entities of 
   *        the specified dimension.
   *\param dim entity dimension
   */
  static bool can_mesh(iBase_EntityType dim)
    { return iBase_VERTEX <= dim && iBase_REGION >= dim; }

  /** \brief Function returning whether this scheme can mesh the specified entity
   * 
   * Used by MeshOpFactory to find scheme for an entity.
   * \param model_ent ModelEnt being queried
   * \return If true, this scheme can mesh the specified ModelEnt
   */
  static bool can_mesh(ModelEnt *model_ent)
      { return can_mesh((iBase_EntityType)model_ent->dimension()); }
    
  /**\brief Get list of mesh entity types that can be generated.
   *\return array terminated with \c moab::MBMAXTYPE
   */
  static const moab::EntityType* output_types();

  /** \brief Return the mesh entity types operated on by this scheme
   * \return array terminated with \c moab::MBMAXTYPE
   */
  virtual const moab::EntityType* mesh_types_arr() const
    { return output_types(); }

private:
  /** \brief Internal representation of the data specifying the Interval Assignment problem.
   */
  // data
  VariableSet variables;
  typedef std::vector< IAVariable* > VariableVec;
  typedef std::vector< VariableVec > VariableVecVec;
  VariableVecVec sumEqualConstraints1, sumEqualConstraints2; // one for each side
  VariableVecVec sumEvenConstraints;
  // ... additional types of constraints ...
  
      /** \brief Subdivide the problem into independent subproblems, in the sense that the
    * solution to one subproblem is not affected in any way by the solution to another.
    * I.e. independent if have no constraints or variables in common. 
    * A given subproblem is complete, in that it contains all the variables for each of 
    * its constraints, and also all the constraints for each of its variables. 
    */
  void subdivide_problem(std::vector<IASolver*> &subproblems);
  
  /** \brief solution sub-methods and sub-data
  * Return the position of var in the variables set, from 0 .. n-1
  */
  int variable_to_index(const IAVariable* var) const;
  /** \brief Find the ind'th variable.
  */
  IAVariable *index_to_variable(int ind) const;
  typedef std::set<int> IndexSet;
  typedef std::vector< IndexSet > IndexSetVec;
  /** \brief Populate index_set with 0..n-1 
  */
  void make_0_to_nm1( IndexSet &index_set, const int k); 
  
  /** \brief Build a representation of the dependency of the problem using indicator sets.
  * \param IndexSetVec Output. The ith constraint_variables are the indices of the variables in the ith constraints.
  * \param IndexSetVec Output. The ith variable_constraints are the indices of the constraints containing the ith variable.
  * \param int i_start: Input. The following vector of constraints is indexed starting at i_start.
  * \param VariableVecVec: Input. Constraints, each entry is a vector specifying one constraint.
  */
  void get_constraint_variable_indices( IndexSetVec &constraint_variables, 
                                       IndexSetVec &variable_constraints,
                                       const int i_start, 
                                       const VariableVecVec &variable_vec_vec );
                                       
  /** \brief Add the variable (index) to the sub-problem, mark it as removed from the larger
  * problem, and recursively build the sub-problem by chasing its dependent constraints.
  */
  void find_variable_dependent_set( const int variable_j, 
                                    const IndexSetVec &constraint_variables,
                                    const IndexSetVec &variable_constraints,
                                    IndexSet &constraint_set, IndexSet &variable_set, 
                                    IndexSet &sub_constraint_set, IndexSet &sub_variable_set);
  
  /** \brief Add the constraint (index) to the sub-problem, mark it as removed from the larger
  * problem, and recursively build the sub-problem by chasing its variables.
  */
  void find_constraint_dependent_set( const int constraint_i, 
                                      const IndexSetVec &constraint_variables,
                                      const IndexSetVec &variable_constraints,
                                      IndexSet &constraint_set, IndexSet &variable_set, 
                                      IndexSet &sub_constraint_set, IndexSet &sub_variable_set);
  

  /** \brief Find a solution for the number of intervals for each variable.
  */
  bool solve_subproblem( IASolver *subproblem );
  
  /** \brief Assign the found solution to the IAVariables.
  */
  void assign_solution( IASolver *subproblem );
  
};

} // namespace MeshKit

#endif
