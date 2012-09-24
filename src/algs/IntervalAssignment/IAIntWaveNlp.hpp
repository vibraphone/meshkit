// IAIntWaveNlp.hpp
// Interval Assignment for Meshkit
//
// ipopt mixed-integer solution
// The idea is the optimal solution will be an integer one, if one exists
// Define some region around the relaxed solution and search for an integer solution
//

#ifndef MESHKIT_IA_IAINTWAVENLP_HP
#define MESHKIT_IA_IAINTWAVENLP_HP

class IAData;
class IPData;
class IASolution;
#include "IANlp.hpp"

#include "IpTNLP.hpp"
using namespace Ipopt;

class IAIntWaveNlp : public TNLP
{
  // first set of functions required by TNLP
public:
  /** default constructor */
  IAIntWaveNlp(const IAData *data_ptr, const IPData *ip_data_ptr, IASolution *solution_ptr); 

  /** default destructor */
  virtual ~IAIntWaveNlp();

  /**@name Overloaded from TNLP */
  //@{
  /** Method to return some info about the nlp */
  virtual bool get_nlp_info(Index& n, Index& m, Index& nnz_jac_g,
                            Index& nnz_h_lag, IndexStyleEnum& index_style);

  /** Method to return the bounds for my problem */
  virtual bool get_bounds_info(Index n, Number* x_l, Number* x_u,
                               Index m, Number* g_l, Number* g_u);

  /** Method to return the starting point for the algorithm */
  virtual bool get_starting_point(Index n, bool init_x, Number* x_init,
                                  bool init_z, Number* z_L, Number* z_U,
                                  Index m, bool init_lambda,
                                  Number* lambda);

  /** Method to return the objective value */
  virtual bool eval_f(Index n, const Number* x, bool new_x, Number& obj_value);

  /** Method to return the gradient of the objective */
  virtual bool eval_grad_f(Index n, const Number* x, bool new_x, Number* grad_f);

  /** Method to return the constraint residuals */
  virtual bool eval_g(Index n, const Number* x, bool new_x, Index m, Number* g);

  /** Method to return:
   *   1) The structure of the jacobian (if "values" is NULL)
   *   2) The values of the jacobian (if "values" is not NULL)
   */
  virtual bool eval_jac_g(Index n, const Number* x, bool new_x,
                          Index m, Index nele_jac, Index* iRow, Index *jCol,
                          Number* values);

  /** Method to return:
   *   1) The structure of the hessian of the lagrangian (if "values" is NULL)
   *   2) The values of the hessian of the lagrangian (if "values" is not NULL)
   */
  virtual bool eval_h(Index n, const Number* x, bool new_x,
                      Number obj_factor, Index m, const Number* lambda,
                      bool new_lambda, Index nele_hess, Index* iRow,
                      Index* jCol, Number* values);

  //@}

  /** @name Solution Methods */
  //@{
  /** This method is called when the algorithm is complete so the TNLP can store/write the solution */
  virtual void finalize_solution(SolverReturn status,
                                 Index n, const Number* x, const Number* z_L, const Number* z_U,
                                 Index m, const Number* g, const Number* lambda,
                                 Number obj_value,
                                 const IpoptData* ip_data,
                                 IpoptCalculatedQuantities* ip_cq);
  //@}

// extra stuff not required by TNLP
  // rescale and randomize so fabs of weights are in [lo, hi], and different from each other
  static void uniquify_weights(std::vector<double> & weights, const double lo, const double hi);
  //generate double in [-1,-0.5] U [.5,1]
  static double rand_excluded_middle(); 
  
private:  
  // hide untrusted default methods
  //@{
  //  IA_NLP();
  IAIntWaveNlp();
  IAIntWaveNlp(const IAIntWaveNlp&);
  IAIntWaveNlp& operator=(const IAIntWaveNlp&);
  //@}
  
  // input data
  const IAData *data;
  const IPData *ipData;

  // solution data
  IASolution *solution;
  
  
  // implemented using an overlay over an IANlp
  IANlp baseNlp;  
  
  const int base_n, base_m;
  const int problem_n, problem_m;
  const int sum_even_constraint_start;
  const int x_int_constraint_start;
  const double PI;
  
  double f_x_value( double I_i, double x_i );

  const bool debugging;
  const bool verbose; // verbose debugging
  
  struct SparseMatrixEntry
	{
	  // position in matrix
	  // column must be less than row, j <= i
	  int i; // row
	  int j; // col
	  
	  // order in sequence for hessian values array, etc.
	  int k;
	
	  static int n; // matrix is n x n
	  int key() const { return i * n + j; }
	  
	  SparseMatrixEntry(const int iset, const int jset, const int kset) 
	  : i(iset), j(jset), k(kset) { assert(j <= i); } 
	  SparseMatrixEntry() : i(-1), j(-1), kP(-1) {} // bad values if unspecified
	};
	
	typedef std::map<int, SparseMatrixEntry> SparseMatrixMap; 
	
	SparseMatrixMap hessian_map;  // sorted by key
	std::vector< SparseMatrixEntry > hessian_vector; // from 0..k
	
	void add_hessian_entry( int i, int j, int &k );
	void build_hessian();
	int get_hessian_k( int i, int j ) const;

};

#endif