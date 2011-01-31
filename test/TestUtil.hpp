#ifndef TEST_UTIL_HPP
#define TEST_UTIL_HPP

/** \file TestUtil.hpp
 *  How to use this test suite utility:
 * 1) Write tests that use the CHECK and CHECK_* macros defined below to assert test conditions.
 * 2) Write a main routine that invokes each test through the RUN_TEST macro
 * 3) RUN_TEST evaluates to 1 if test failed, zero otherwize.  Count failures and print summary.
 */

/* Define these here because they are used by many tests
 * to find the add directory for input files */
#define STRINGIFY_(X) #X
#define STRINGIFY(X) STRINGIFY_(X)
#ifdef DATADIR
std::string TestDir( STRINGIFY(DATADIR) );
#else
std::string TestDir( "." );
#endif

/** Check that A is MK_SUCCESS */
#define CHECK_ERR( A )                    check_equal( MK_SUCCESS, (A), "MK_SUCCESS", #A, __LINE__, __FILE__ )
/**  Ensure that A is true */
#define CHECK( A )                        check_true( (A), #A, __LINE__, __FILE__ )
/** Check that two values are equal */
#define CHECK_EQUAL( EXP, ACT )           check_equal( (EXP), (ACT), #EXP, #ACT, __LINE__, __FILE__ )
/** Check that two real (float or double) values are equal within EPS */
#define CHECK_REAL_EQUAL( EXP, ACT, EPS ) check_equal( (EXP), (ACT), (EPS), #EXP, #ACT, __LINE__, __FILE__ )
/** Check that two arrays contain the same values in the same order */
#define CHECK_ARRAYS_EQUAL( EXP, EXP_LEN, ACT, ACT_LEN ) check_array_equal( (EXP), (EXP_LEN), (ACT), (ACT_LEN), #EXP, #ACT, __LINE__, __FILE__ )
/** Run a test
 *  Argument should be a function with the signature:  void func(void)
 *  Evaluates to zero if test is successful, one otherwise.
 */
#define RUN_TEST( FUNC )           run_test( &FUNC, #FUNC )


// Use C++ exceptions to return error state to test runner
// Portable, but whole test suite stops if any test segfaults, asserts, etc.
#define EXCEPTION_MODE 1   

// Test runner forks separate process for each test.
// Difficult to debug tests (with debugger).  Not portable to Windows.  
// Very robust (no test can distrub test running code)
#define FORK_MODE 2

// Use signal handler and long jumps to return error state to test runner.
// Might be portable to Windows (not sure).  Possibly undefined behavior (e.g. continuing 
// with next test after catching segfault is technically undefined behavior.)
// Also, tests can corrupt heap memory management, interferring with later tests.
// Leaks memory on test failure (no stack unwind).  This is actually a feature, as
// we don't care too much about tests leaking memory and trying to reconver memory
// might make things worse, depending on why the test failed.
#define LONGJMP_MODE 3      

// If test application hasn't set MODE, set to default
#ifndef MODE
#if defined(_MSC_VER) || defined(__MINGW32__)
#    define MODE EXCEPTION_MODE
#  else
#    define MODE LONGJMP_MODE
#  endif
#endif



/***************************************************************************************
 * NOTE: The remainder of this file contains the implementation of the above macros.
 *       The above macros constitute the entire intended API.
 ***************************************************************************************/

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <string>
#ifdef __cplusplus
#include <iostream>
#include <vector>
#endif

/***************************************************************************************
 *                     Define What to do when a test fails.
 ***************************************************************************************/

// For EXCEPTION_MODE, throw an exception when a test fails.
// This will unwind stack, recover memory, etc. 
#if MODE == EXCEPTION_MODE
   struct ErrorExcept{};
#  define FLAG_ERROR throw ErrorExcept()
// For FORK_MODE, the test is running in its own processs.  Just
// terminate the process with a non-zero exit code when the test
// fails.
#elif MODE == FORK_MODE
#  include <sys/types.h>
#  include <sys/wait.h>
#  include <unistd.h>
#  include <errno.h>
#  define FLAG_ERROR exit(1)
// For LONGJMP_MODE, we do a long jump to just before the test is
// run, with a return value of -1 to indicate failures (positive
// return codes are used if the test caused a segfault or other
// signal.)
#elif MODE == LONGJMP_MODE
#  include <signal.h>
#  include <setjmp.h>
#  define FLAG_ERROR siglongjmp( jmpenv, -1 )
#else
#  error "MODE not set"
#endif

/***************************************************************************************
 *                              Setup for LONGJMP_MODE
 ***************************************************************************************/

#if MODE == LONGJMP_MODE

// Variable to hold stack state for longjmp
sigjmp_buf jmpenv;

// Define signal handler used to catch errors such as segfaults.
// Signal handler does longjmp with the signal number as the 
// return value.
extern "C" {
  void sighandler( int sig ) {
    signal( sig, sighandler );
    siglongjmp(jmpenv, sig);
    // should never return from longjmp
    exit(1);
  }
  typedef void (*sigfunc_t)(int);
} // extern "C"

// Helper function to register signal handlers.  
int sethandler( int sig ) {
  sigfunc_t h = signal( sig, &sighandler );
  if (h == SIG_ERR)
    return  1;
   // If user-defined signal handler (or signal is ignored),
   // than unregister our handler.
  else if (h != SIG_DFL)
    signal( sig, h );
  return 0;
}

// Register signal handlers for all defined signals that typicall result
// in process termination.
int init_signal_handlers()
{
  int result = 0;
#ifdef SIGHUP
  result += sethandler( SIGHUP );
#endif
#ifdef SIGINT
  result += sethandler( SIGINT );
#endif
#ifdef SIGQUIT
  result += sethandler( SIGQUIT );
#endif
#ifdef SIGILL
  result += sethandler( SIGILL );
#endif
#ifdef SIGTRAP
  result += sethandler( SIGTRAP );
#endif
#ifdef SIGABRT
  result += sethandler( SIGABRT );
#endif
#ifdef SIGBUS
  result += sethandler( SIGBUS );
#endif
#ifdef SIGFPE
  result += sethandler( SIGFPE );
#endif
#ifdef SIGUSR1
  result += sethandler( SIGUSR1 );
#endif
#ifdef SIGSEGV
  result += sethandler( SIGSEGV );
#endif
#ifdef SIGUSR2
  result += sethandler( SIGUSR2 );
#endif
#ifdef SIGPIPE
  result += sethandler( SIGPIPE );
#endif
#ifdef SIGTERM
  result += sethandler( SIGTERM );
#endif
#ifdef SIGCHLD
  result += sethandler( SIGCHLD );
#endif
#ifdef SIGIO
  result += sethandler( SIGIO );
#endif
#ifdef SIGSYS
  result += sethandler( SIGSYS );
#endif
  return result;
}

// Declare a garbage global variable.  Use variable initialization to
// force call to init_signal_handlers().  
int junk_init_var = init_signal_handlers();

#endif // LONGJMP_MODE


/***************************************************************************************
 *                            Function to handle failed tests
 ***************************************************************************************/

// use a function rather than substituting FLAG_ERROR directly
// so we have a convenient place to set a break point
inline void flag_error() 
  { FLAG_ERROR; }


/***************************************************************************************
 *                            The Code to Run Tests
 ***************************************************************************************/


typedef void (*test_func)(void);
int run_test( test_func test, const char* func_name )
{
  printf("Running %s ...\n", func_name );
  
#if MODE == EXCEPTION_MODE
  /* On Windows, run all tests in same process.
     Flag errors by throwing an exception.
   */
  try {
    (*test)();
    return 0;
  }
  catch (ErrorExcept) {
    printf( "  %s: FAILED\n", func_name );
    return 1;
  }
  catch (...) {
    printf( "  %s: UNCAUGHT EXCEPTION\n", func_name );
    return 1;
  }
    
#elif MODE == FORK_MODE
    /* For non-Windows OSs, fork() and run test in child process. */
  pid_t pid = fork();
  int status;
  
    /* Fork failed? */
  if (pid == -1) {  
    perror( "fork()" );
    abort(); /* abort all tests (can't fork child processes) */
  }
  
    /* If child process*/
  if (pid == 0) {
    (*test)();  /* call test function */
    exit(0);    /* if function returned, then it succeeded */
  }
  
    /* If here, then parent process */
    
    /* Wait until child process exits */
  waitpid( pid, &status, 0 );
  
    /* Check child exit status */
  if (WIFSIGNALED(status)) {
    if (WTERMSIG(status))
      printf("  %s: TERMINATED (signal %d)\n", func_name, (int)WTERMSIG(status) );
    if (WCOREDUMP(status))
      printf("  %s: CORE DUMP\n", func_name);
    return 1;
  }
  else if(WEXITSTATUS(status)) {
    printf( "  %s: FAILED\n", func_name );
    return 1;
  }
  else {
    return 0;
  }
  
#elif MODE == LONGJMP_MODE
    // Save stack state at this location.
  int rval = sigsetjmp( jmpenv, 1 );
    // If rval is zero, then we haven't run the test yet. 
    // If rval is non-zero then
    // a) we ran the test
    // b) the test failed
    // c) we did a longjmp back to the location where we called setsigjmp.
    
    // run test
  if (!rval) {
    (*test)();
    return 0;
  }
    // some check failed
  else if (rval == -1) {
    printf( "  %s: FAILED\n", func_name );
    return 1;
  }
    // a signal was raised (e.g. segfault)
  else {
    printf( "  %s: TERMINATED (signal %d)\n", func_name, rval );
    return 1;
  }
#else
  #error "MODE not set"
#endif // MODE
}



/***************************************************************************************
 *                            CHECK_EQUAL implementations
 ***************************************************************************************/

// Common implementatation for most types
#define EQUAL_TEST_IMPL( TEST, TYPE ) if( !(TEST) ) { \
  printf( "Equality Test Failed: %s == %s\n", sA, sB ); \
  printf( "  at line %d of '%s'\n", line, file ); \
  printf( "  Expected value: %" #TYPE "\n", A ); \
  printf( "  Actual value:   %" #TYPE "\n", B ); \
  printf( "\n" ); \
  flag_error(); \
}

void check_equal( int A, int B, const char* sA, const char* sB, int line, const char* file )
  {  EQUAL_TEST_IMPL( A == B, d ) }

void check_equal( unsigned A, unsigned B, const char* sA, const char* sB, int line, const char* file )
  {  EQUAL_TEST_IMPL( A == B, u ) }

void check_equal( long A, long B, const char* sA, const char* sB, int line, const char* file )
  {  EQUAL_TEST_IMPL( A == B, ld ) }

void check_equal( unsigned long A, unsigned long B, const char* sA, const char* sB, int line, const char* file )
  {  EQUAL_TEST_IMPL( A == B, lu ) }

void check_equal( void* A, void* B, const char* sA, const char* sB, int line, const char* file )
  {  EQUAL_TEST_IMPL( A == B, p ) }

void check_equal( const char* A, const char* B, const char* sA, const char* sB, int line, const char* file )
  {  EQUAL_TEST_IMPL( !strcmp((A),(B)), s ) }

void check_equal( const std::string& A, const std::string& B, const char* sA, const char* sB, int line, const char* file )
  {  check_equal( A.c_str(), B.c_str(), sA, sB, line, file); }

void check_equal( float A, float B, float eps, const char* sA, const char* sB, int line, const char* file )
  {  EQUAL_TEST_IMPL( fabsf(A - B) <= eps, f ) }

void check_equal( double A, double B, float eps, const char* sA, const char* sB, int line, const char* file )
  {  EQUAL_TEST_IMPL( fabs(A - B) <= eps, f ) }



// Special case for MBErrorCode, use mb_error_str() to print the 
// string name of the error code.
void check_equal( MeshKit::ErrorCode A, MeshKit::ErrorCode B, const char* sA, const char* sB, int line, const char* file )
{
  if (A == B)
    return;
  
  printf( "MeshKit::ErrorCode Test Failed: %s == %s\n", sA, sB ); 
  printf( "  at line %d of '%s'\n", line, file ); 
  printf( "  Expected value: %s (%d)\n", Error::error_str(A), (int)A ); 
  printf( "  Actual value:   %s (%d)\n", Error::error_str(B), (int)B ); 
  printf( "\n" ); 
  flag_error(); 
}

void check_true( bool cond, const char* str, int line, const char* file )
{
  if( !cond ) { 
    printf( "Test Failed: %s\n", str ); 
    printf( "  at line %d of '%s'\n", line, file ); 
    printf( "\n" ); 
    flag_error(); 
  }
}

#ifdef __cplusplus

template <typename T>
void check_array_equal( const T* A, size_t A_size,
                        const T* B, size_t B_size, 
                        const char* sA, const char* sB, 
                        int line, const char* file )
{
  size_t i = 0;
  for (;;) {
    if (i == A_size && i == B_size)
      return; // equal
    else if (i == A_size || i == B_size)
      break; // differene lengths
    else if (A[i] != B[i])
      break;
    ++i;
  }
  
  std::cout << "Equality Test Failed: " << sA << " == " << sB << std::endl;
  std::cout << "  at line " << line << " of '" << file << "'" << std::endl;
  std::cout << "  Vectors differ at position " << i << std::endl;
  
    // print at most 10 values, roughly centered on the unequal one
  size_t count = 10, num_front_values = std::min(count/2,i);
  size_t max_len = std::max(A_size,B_size);
  if (i + count - num_front_values > max_len) {
    if (count > max_len) {
      num_front_values = i;
      count = max_len;
    }
    else {
      num_front_values = count - (max_len - i);
    }
  }
  
  std::cout << "  Expected: ";
  if (!A_size) {
    std::cout << "(empty)" << std::endl;
  }
  else {
    size_t j = i - num_front_values;
    size_t end = std::min(j + count, A_size);
    if (j) 
      std::cout << "... ";
    for (; j < end; ++j) {
      if (j == i)
        std::cout << '>' << A[j] << "< ";
      else
        std::cout << A[j] << " ";
    }
    if (end != A_size)
      std::cout << "...";
    std::cout << std::endl;
  }
  
  std::cout << "  Actual:   ";
  if (!B_size) {
    std::cout << "(empty)" << std::endl;
  }
  else {
    size_t j = i - num_front_values;
    size_t end = std::min(j + count, B_size);
    if (j) 
      std::cout << "... ";
    for (; j < end; ++j) {
      if (j == i)
        std::cout << '>' << B[j] << "< ";
      else
        std::cout << B[j] << " ";
    }
    if (end != B_size)
      std::cout << ", ...";
    std::cout << std::endl;
  }
  
  flag_error(); 
}
  
 
template <typename T>
void check_equal( const std::vector<T>& A, const std::vector<T>& B, 
                  const char* sA, const char* sB, 
                  int line, const char* file )
{
  check_array_equal( &A[0], A.size(), &B[0], B.size(), sA, sB, line, file );
}

#ifdef MOAB_RANGE_HPP

void check_equal( const moab::Range& A, const moab::Range& B, const char* sA, const char* sB, int line, const char* file )
{
  if (A == B)
    return;
    
  std::cout << "MeshKit::ErrorCode Test Failed: " << sA << " == " << sB << std::endl;
  std::cout << "  at line " << line << " of '" << file << "'" << std::endl;
  std::cout << "   Expected: " << A << std::endl;
  std::cout << "   Actual  : " << B << std::endl;
  std::cout << std::endl;
  flag_error();
}

#endif  /* ifdef MOAB_RANGE_HPP */
    
#endif /* ifdef __cplusplus */

#endif