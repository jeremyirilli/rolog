#include <SWI-Prolog.h>
#include "Rcpp.h"
using namespace Rcpp ;

// Translate prolog expression to R
//
// [] -> NULL
// real -> NumericVector
// #(r1, r2, r3) -> NumericVector (# is a default, see option realvec)
// ##(#(row1), #(row2), ...) -> Matrix
// integer -> IntegerVector
// %(i1, i2, i3) -> IntegerVector (see option intvec for the name)
// %%(%(row1), %(row2), ...) -> Matrix
// string -> CharacterVector
// $$(s1, s2, s3) CharacterVector
// $$$($$(row1), $$(row2), ...) -> Matrix
// na (atom) -> NA
// true, false (atoms) -> LogicalVector
// !(l1, l2, l3) -> LogicalVector (see option boolvec)
// !!(!(row1), !(row2), ...) -> Matrix
// the empty atom -> ""
// other atoms -> symbol/name
// variable -> expression(variable name)
// compound -> call (aka. "language")
// list -> list
//
RObject pl2r(term_t pl, CharacterVector& names, term_t& vars, List options) ;

// Translate R expression to prolog
//
// NULL -> []
// numeric vector of length 1 -> real (unless rolog.scalar == FALSE)
// numeric vector of length > 1 -> e.g., #(1.0, 2.0, 3.0) (see rolog.realvec)
// integer vector of length 1 -> integer
// integer vector of length > 1 -> %(1, 2, 3)
// character vector of length 1 -> string
// character vector of length > 1 -> $("a", "b", "c")
// logical vector of length 1 -> the atoms true, false or na
// logical vector of length > 1 -> $(true, false, na)
// other symbols/name -> atom
// expression -> variable
// call/language -> compound
// list -> list
//
term_t r2pl(SEXP r, CharacterVector& names, term_t& vars, List options) ;

// Consult one or more files. If something fails, the procedure stops, and
// will not try to consult the remaining files.
//
// [[Rcpp::export(.consult)]]
LogicalVector consult_(CharacterVector files)
{
  for(R_xlen_t i=0; i<files.size(); i++)
  {
    term_t fname = PL_new_term_ref() ;
    PL_put_atom_chars(fname, files(i)) ;

    predicate_t p = PL_predicate("consult", 1, NULL) ;
    if(PL_call_predicate(NULL, PL_Q_NORMAL, p, fname) == FALSE)
      stop("failed to consult %s", (char*) files(i)) ;
  }

  return true ;
}

// Prolog -> R
RObject pl2r_null()
{
  return R_NilValue ;
}

// This helper function checks for na and then translates an individual PlTerm 
// to a double.
double pl2r_double(term_t pl)
{
  if(PL_is_atom(pl) && !strcmp("na", PL_atom_nchars(pl, NULL)))
    return NA_REAL ;

  double f ;
  if(PL_get_float(pl, &f))
    return f ;

  char* s ;
  if(PL_get_chars(pl, &s, CVT_ALL|BUF_STACK|REP_UTF8))
    warning("cannot convert %s to float", s) ;
  else
    warning("cannot convert to float") ;

  return NA_REAL ;
}

// Convert scalar real to DoubleVector of length 1
DoubleVector pl2r_real(term_t pl)
{
  return DoubleVector::create(pl2r_double(pl)) ;
}

// Convert vector of reals (e.g., #(1.0, 2.0, na)) to DoubleVector
DoubleVector pl2r_realvec(term_t pl)
{
  size_t arity = PL_functor_arity(pl) ;
  DoubleVector r(arity) ;
  for(size_t i=0; i<arity; i++)
  {
    term_t arg ;
    PL_get_arg(pl, i+1, arg) ;
    r(i) = pl2r_double(arg) ;
  }

  return r ;
}

// Convert matrix of reals (e.g., ##(#(1.0, 2.0), #(na, ...), ...))
NumericMatrix pl2r_realmat(term_t pl)
{
  size_t nrow = PL_functor_arity(pl) ;
  size_t ncol = 0 ;
  if(nrow > 0)
  {
    for(size_t i=0; i<nrow; i++)
      if(i == 0)
      {
        term_t row1 ;
        PL_get_arg(pl, 1, row1) ;
        ncol = PL_functor_arity(row1) ;
      }
      else
      {
        term_t row ;
        PL_get_arg(pl, i+1, row) ;
        if(PL_functor_arity(row) != ncol)
          stop("cannot convert PlTerm to Matrix, inconsistent rows") ;
      }
  }

  NumericMatrix r(nrow, ncol) ;
  for(size_t i=0; i<nrow; i++)
  {
    term_t row ;
    PL_get_arg(pl, i+1, row) ;
    r.row(i) = pl2r_realvec(row) ;
  }

  return r ;
}

// See above for pl2r_double
long pl2r_int(PlTerm pl)
{
  if(PL_is_atom(pl) && pl == "na")
    return NA_INTEGER ;

  try 
  {
    return (long) pl ;
  }
  
  catch(PlException& ex)
  { 
    warning("Cannot convert %s to integer: %s", (char*) pl, (char*) ex) ;
    PL_clear_exception() ;
    return NA_INTEGER ;
  }
}

IntegerVector pl2r_integer(PlTerm pl)
{
  return IntegerVector::create(pl2r_int(pl)) ;
}

IntegerVector pl2r_intvec(PlTerm pl)
{
  IntegerVector r(pl.arity()) ;
  for(size_t i=0; i<pl.arity(); i++)
    r(i) = pl2r_int(pl[i+1]) ;

  return r ;
}

IntegerMatrix pl2r_intmat(PlTerm pl)
{
  size_t nrow = pl.arity() ;
  size_t ncol = 0 ;
  if(nrow > 0)
  {
    for(size_t i=0; i<pl.arity(); i++)
      if(i == 0)
        ncol = pl[1].arity() ;
      else
      {
        if(pl[i+1].arity() != ncol)
          stop("cannot convert PlTerm to Matrix, inconsistent rows") ;
      }
  }

  IntegerMatrix r(nrow, ncol) ;
  for(size_t i=0; i<nrow; i++)
    r.row(i) = pl2r_intvec(pl[i+1]) ;

  return r ;
}

// See above for pl2r_double
String pl2r_string(PlTerm pl)
{
  if(PL_is_atom(pl) && pl == "na")
    return NA_STRING ;
  
  return (char*) pl ;
}

CharacterVector pl2r_char(PlTerm pl)
{
  return CharacterVector::create(pl2r_string(pl)) ;
}

CharacterVector pl2r_charvec(PlTerm pl)
{
  CharacterVector r(pl.arity()) ;
  for(size_t i=0; i<pl.arity(); i++)
    r(i) = pl2r_string(pl[i+1]) ;

  return r ;
}

CharacterMatrix pl2r_charmat(PlTerm pl)
{
  size_t nrow = pl.arity() ;
  size_t ncol = 0 ;
  if(nrow > 0)
  {
    for(size_t i=0; i<pl.arity(); i++)
      if(i == 0)
        ncol = pl[1].arity() ;
      else
      {
        if(pl[i+1].arity() != ncol)
          stop("cannot convert PlTerm to Matrix, inconsistent rows") ;
      }
  }

  CharacterMatrix r(nrow, ncol) ;
  for(size_t i=0; i<nrow; i++)
    r.row(i) = pl2r_charvec(pl[i+1]) ;

  return r ;
}

// Convert prolog atom to R symbol (handle na, true, false)
RObject pl2r_symbol(PlTerm pl)
{
  if(pl == "na")
    return wrap(NA_LOGICAL) ;
  
  if(pl == "true")
    return wrap(true) ;
  
  if(pl == "false")
    return wrap(false) ;

  // Empty symbols
  if(pl == "")
    return Function("substitute")() ;

  return wrap(Symbol((char*) pl)) ;
}

// Forward declaration, needed below
RObject pl2r_compound(PlTerm pl, CharacterVector& names, PlTerm& vars, List options) ;

// Convert prolog neck to R function
RObject pl2r_function(PlTerm pl, CharacterVector& names, PlTerm& vars, List options)
{
  PlTerm plhead = pl[1] ;
  PlTerm plbody = pl[2] ;

  Language head("alist") ;
  for(unsigned int i=1 ; i<=plhead.arity() ; i++)
  {
    PlTerm arg = plhead[i] ;

    // Compounds like mean=100 are translated to named function arguments
    if(PL_is_compound(arg) && !strcmp(arg.name(), "=") && arg.arity() == 2)
    {
      PlTerm a1 = arg[1] ;
      PlTerm a2 = arg[2] ;
      if(PL_is_atom(a1))
      {
        head.push_back(Named((char*) a1) = pl2r(a2, names, vars, options)) ;
        continue ;
      }
    }

    // just the name, no argument like in alist(a=, b=)
    head.push_back(Named((char*) arg) = Function("substitute")()) ;
  }

  RObject body = pl2r_compound(plbody, names, vars, options) ;
  head.push_back(body) ;

  Function as_function("as.function") ;
  return wrap(as_function(head)) ;
}

LogicalVector pl2r_boolvec(PlTerm pl)
{
  LogicalVector r(pl.arity()) ;
  for(size_t i=0; i<pl.arity(); i++)
  {
    PlTerm t = pl[i+1] ;
    if(PL_is_atom(t))
    {
      if(t == "na")
      {
        r(i) = NA_LOGICAL ;
        continue ;
      }
      
      if(t == "true")
      {
        r(i) = 1 ;
        continue ;
      }
      
      if(t == "false")
      {
        r(i) = 0 ;
        continue ;
      }
    }
    
    warning("r2pl_logical: invalid item %s, returning NA", (char*) t) ;
    r(i) = NA_LOGICAL ;
  }

  return r ;
}

LogicalMatrix pl2r_boolmat(PlTerm pl)
{
  size_t nrow = pl.arity() ;
  size_t ncol = 0 ;
  if(nrow > 0)
  {
    for(size_t i=0; i<pl.arity(); i++)
      if(i == 0)
        ncol = pl[1].arity() ;
      else
      {
        if(pl[i+1].arity() != ncol)
          stop("cannot convert PlTerm to Matrix, inconsistent rows") ;
      }
  }

  LogicalMatrix r(nrow, ncol) ;
  for(size_t i=0; i<nrow; i++)
    r.row(i) = pl2r_boolvec(pl[i+1]) ;

  return r ;
}

// Translate prolog variables to R expressions.
RObject pl2r_variable(PlTerm pl, CharacterVector& names, PlTerm& vars)
{
  // names and vars is a list of all the variables from the R query,
  // a typical member of names is something like X, a member of vars 
  // is something like _1545.
  //
  // Search for the variable (e.g., _1545) in names and return its R name as an
  // expression (say, X).
  PlTail tail(vars) ;
  PlTerm v ;
  for(int i=0 ; i<names.length() ; i++)
  {
    tail.next(v) ;
    if(v == pl)
      return ExpressionVector::create(Symbol(names(i))) ;
  }
  
  // If the variable is not found, it's a new one created by Prolog, e.g., in
  // queries like member(1, Y), Y is unified with [1 | _NewVar ]. This variable
  // cannot be translated to a human-readable name, so it is returned as _1545.
  return ExpressionVector::create(Symbol((char*) pl)) ;
}

// Translate prolog compound to R call
//
// This function takes care of special compound names (#, %, $, !) for vector
// objects in R, as well as "named" function arguments like "mean=100", in
// rnorm(10, mean=100, sd=15).
RObject pl2r_compound(PlTerm pl, CharacterVector& names, PlTerm& vars, List options)
{
  // This function does not (yet) work for cyclic terms
  if(!PL_is_acyclic(pl))
    stop("pl2r: Cannot convert cyclic term %s", (char*) pl) ;

  // Convert ##(#(...), ...) to NumericMatrix
  if(!strcmp(pl.name(), options("realmat")))
    return pl2r_realmat(pl) ;

  // Convert #(1.0, 2.0, 3.0) to DoubleVector (# given by options("realvec"))
  if(!strcmp(pl.name(), options("realvec")))
    return pl2r_realvec(pl) ;

  // Convert %%(%(...), ...) to IntegerMatrix
  if(!strcmp(pl.name(), options("intmat")))
    return pl2r_intmat(pl) ;

  // Convert %(1.0, 2.0, 3.0) to IntegerVector
  if(!strcmp(pl.name(), options("intvec")))
    return pl2r_intvec(pl) ;

  // Convert $$$($$(...), ...) to StringMatrix
  if(!strcmp(pl.name(), options("charmat")))
    return pl2r_charmat(pl) ;

  // Convert $$(1.0, 2.0, 3.0) to CharacterVector
  if(!strcmp(pl.name(), options("charvec")))
    return pl2r_charvec(pl) ;

  // Convert !!(!(...), ...) to LogicalMatrix
  if(!strcmp(pl.name(), options("boolmat")))
    return pl2r_boolmat(pl) ;

  // Convert !(1.0, 2.0, 3.0) to LogicalVector
  if(!strcmp(pl.name(), options("boolvec")))
    return pl2r_boolvec(pl) ;

  // Convert :- to function
  if(!strcmp(pl.name(), ":-"))
    return pl2r_function(pl, names, vars, options) ;

  // Other compounds
  Language r(pl.name()) ;
  for(unsigned int i=1 ; i<=pl.arity() ; i++)
  {
    PlTerm arg = pl[i] ;

    // Compounds like mean=100 are translated to named function arguments
    if(PL_is_compound(arg) && !strcmp(arg.name(), "=") && arg.arity() == 2)
    {
      PlTerm a1 = arg[1] ;
      PlTerm a2 = arg[2] ;
      if(PL_is_atom(a1))
      {
        r.push_back(Named(a1.name()) = pl2r(a2, names, vars, options)) ;
        continue ;
      }
    }

    // argument has no name
    r.push_back(pl2r(arg, names, vars, options)) ;
  }

  return as<RObject>(r) ;
}

// Translate prolog list to R list
//
// This code allows for lists like [1, 2 | Tail] with variable tail. These 
// cannot be processed by PlTerm_tail, therefore, the code is a bit more 
// complicated, also because it can handle named arguments.
//
// Examples:
// [1, 2, 3] -> list(1, 2, 3)
// [1, 2 | X] -> `[|]`(1, `[|]`(2, expression(X)))
// [a-1, b-2, c-3] -> list(a=1, b=2, c=3)
//
RObject pl2r_list(PlTerm pl, CharacterVector& names, PlTerm& vars, List options)
{
  PlTerm head = pl[1] ;
  
  // if the tail is a list or empty, return a normal list
  RObject tail = pl2r(pl[2], names, vars, options) ;
  if(TYPEOF(tail) == VECSXP || TYPEOF(tail) == NILSXP)
  {
    List r = as<List>(tail) ;
    
    // convert prolog pair a-X to named list element
    if(PL_is_compound(head) && !strcmp(head.name(), "-") && head.arity() == 2)
    {
      PlTerm a1 = head[1] ;
      PlTerm a2 = head[2] ;
      if(PL_is_atom(a1))
      {
        r.push_front(pl2r(a2, names, vars, options), a1.name()) ;
        return r ;
      }
    }
    
    // element has no name
    r.push_front(pl2r(head, names, vars, options)) ; 
    return r ;
  }
    
  // if the tail is something else, return [|](head, tail)
  Language r(pl.name()) ;
  
  // convert prolog pair a-X to named list element
  if(PL_is_compound(head) && !strcmp(head.name(), "-") && head.arity() == 2)
  {
    PlTerm a1 = head[1] ;
    PlTerm a2 = head[2] ;
    if(PL_is_atom(a1))
    {
      r.push_back(Named(a1.name()) = pl2r(a2, names, vars, options)) ;
      r.push_back(tail) ;
      return as<RObject>(r) ;
    }
  }

  // element has no name
  r.push_back(pl2r(head, names, vars, options)) ; 
  r.push_back(tail) ;
  return as<RObject>(r) ;
}

RObject pl2r(PlTerm pl, CharacterVector& names, PlTerm& vars, List options)
{
  if(pl.type() == PL_NIL)
    return pl2r_null() ;

  if(PL_is_integer(pl))
    return pl2r_integer(pl) ;

  if(PL_is_float(pl))
    return pl2r_real(pl) ;

  if(PL_is_string(pl))
    return pl2r_char(pl) ;

  if(PL_is_atom(pl))
    return pl2r_symbol(pl) ;

  if(PL_is_list(pl))
    return pl2r_list(pl, names, vars, options) ;

  if(PL_is_compound(pl))
    return pl2r_compound(pl, names, vars, options) ;

  if(PL_is_variable(pl))
    return pl2r_variable(pl, names, vars) ;

  stop("pl2r: Cannot convert %s", (char*) pl) ;
  return pl2r_null() ;
}

// Translate R expression to prolog
//
// Forward declarations
PlTerm r2pl_real(NumericVector r, List options) ;
PlTerm r2pl_logical(LogicalVector r, List options) ;
PlTerm r2pl_integer(IntegerVector r, List options) ;
PlTerm r2pl_string(CharacterVector r, List options) ;

// This returns an empty list
PlTerm r2pl_null()
{
  PlTerm pl ;
  PlTail(pl).close() ;
  return pl ;
}

// Prolog representation of R's NA.
PlTerm r2pl_na()
{
  return PlAtom("na") ;
}

// Translate to matrix ###(##(1.0, 2.0, 3.0), ##(4.0, 5.0, 6.0))
PlTerm r2pl_matrix(Matrix<REALSXP> r, List aoptions)
{
  List options(aoptions) ;
  options("scalar") = false ;

  PlTermv rows(r.nrow()) ;
  for(int i=0 ; i<r.nrow() ; i++)
    rows[i] = r2pl_real(r.row(i), options) ;

  PlTermv matrix(5) ;
  matrix[0] = PlTerm((long int) r.nrow()) ;
  matrix[1] = PlTerm((long int) r.ncol()) ;
  matrix[2] = r2pl_string(rownames(r), options) ;
  matrix[3] = r2pl_string(colnames(r), options) ;
  matrix[4] = PlCompound("data", rows) ;
  return PlCompound((const char*) aoptions("realmat"), matrix) ;
}

// Translate to (scalar) real or compounds like ##(1.0, 2.0, 3.0)
PlTerm r2pl_real(NumericVector r, List options)
{
  if(Rf_isMatrix(r))
    return r2pl_matrix(as<Matrix<REALSXP>>(r), options) ;

  if(r.length() == 0)
    return r2pl_null() ;

  LogicalVector nan = is_nan(r) ;
  LogicalVector na = is_na(r) ;
  
  // Translate to scalar
  if(as<LogicalVector>(options("scalar"))(0) && r.length() == 1)
  {
    if(na[0] && !nan[0])
      return r2pl_na() ;
    
    return PlTerm((double) r[0]);
  }

  // Translate to vector ##(1.0, 2.0, 3.0)
  size_t len = (size_t) r.length() ;
  PlTermv args(len) ;
  for(size_t i=0 ; i<len ; i++)
  {
    if(na[i] && !nan[i])
      args[i] = r2pl_na() ;
    else
      args[i] = PlTerm((double) r[i]) ;
  }

  return PlCompound((const char*) options("realvec"), args) ;
}

// Translate to matrix !!(!(true, false), !(false, true))
PlTerm r2pl_matrix(Matrix<LGLSXP> r, List aoptions)
{
  List options(aoptions) ;
  options("scalar") = false ;

  PlTermv rows(r.nrow()) ;
  for(int i=0 ; i<r.nrow() ; i++)
    rows[i] = r2pl_logical(r.row(i), options) ;

  return PlCompound((const char*) options("boolmat"), rows) ;
}

// Translate to (scalar) boolean or compounds like !(true, false, na)
PlTerm r2pl_logical(LogicalVector r, List options)
{
  if(Rf_isMatrix(r))
    return r2pl_matrix(as<Matrix<LGLSXP>>(r), options) ;

  if(r.length() == 0)
    return r2pl_null() ;
  
  LogicalVector na = is_na(r) ;
  
  // scalar boolean
  if(as<LogicalVector>(options("scalar"))(0) && r.length() == 1)
  {
    if(na[0])
      return r2pl_na() ;
    
    return PlAtom(r[0] ? "true" : "false") ;
  }

  // LogicalVector !(true, false, na)
  size_t len = (size_t) r.length() ;
  PlTermv args(len) ;
  for(size_t i=0 ; i<len ; i++)
  {
    if(na[i])
      args[i] = r2pl_na() ;
    else
      args[i] = PlAtom(r[i] ? "true" : "false") ;
  }

  return PlCompound((const char*) options("boolvec"), args) ;
}

// Translate to matrix %%(%(1, 2), %(3, 4))
PlTerm r2pl_matrix(Matrix<INTSXP> r, List aoptions)
{
  List options(aoptions) ;
  options("scalar") = false ;

  PlTermv rows(r.nrow()) ;
  for(int i=0 ; i<r.nrow() ; i++)
    rows[i] = r2pl_integer(r.row(i), options) ;

  return PlCompound((const char*) options("intmat"), rows) ;
}

// Translate to (scalar) integer or compounds like %(1, 2, 3)
PlTerm r2pl_integer(IntegerVector r, List options)
{
  if(Rf_isMatrix(r))
    return r2pl_matrix(as<Matrix<INTSXP>>(r), options) ;

  if(r.length() == 0)
    return r2pl_null() ;
  
  LogicalVector na = is_na(r) ;
  
  // scalar integer
  if(as<LogicalVector>(options("scalar"))(0) && r.length() == 1)
  {
    if(na[0])
      return r2pl_na() ;
    
    return PlTerm((long) r(0)) ;
  }
  
  // IntegerVector %(1, 2, 3)
  size_t len = (size_t) r.length() ;
  PlTermv args(len) ;
  for(size_t i=0 ; i<len ; i++)
  {
    if(na[i])
      args[i] = r2pl_na() ;
    else
      args[i] = PlTerm((long) r[i]) ;
  }
  
  return PlCompound((const char*) options("intvec"), args) ;
}

// Translate R expression to prolog variable
//
// This function keeps a record of the names of the variables in 
// use (e.g., _1545) as well as the corresponding R names (e.g., X). If a new
// variable is encountered, its name is looked up in the list of known 
// variables, and it is unified with it if the name is found. Otherwise, a new
// variable is created.
//
// If options("atomize") is true, no variable is created, but an atom is created 
// with the variable name from R. This is only used for pretty printing.
PlTerm r2pl_var(ExpressionVector r, CharacterVector& names, PlTerm& vars, List options)
{
  // Variable name in R
  Symbol n = as<Symbol>(r[0]) ;
  
  // If the variable should be "atomized" for pretty printing
  if(as<LogicalVector>(options("atomize"))(0))
    return PlAtom(n.c_str()) ;

  // Do not map the anonymous variable to a known variable name
  if(n == "_")
    return PlTerm() ;

  // Unify with existing variable of the same name
  PlTail tail(vars) ;
  PlTerm v ;
  for(R_xlen_t i=0 ; i<names.length() ; i++)
  {
    tail.next(v) ;
    if(n == names(i))
      return v ;
  }

  // If no such variable exists, create a new one and remember the name
  names.push_back(n.c_str()) ;
  PlTerm pl ;
  tail.append(pl) ;
  return pl ;
}

// Translate R symbol to prolog atom
PlTerm r2pl_atom(Symbol r)
{
  return PlAtom(r.c_str()) ;
}

// Translate to matrix $$$($$(1, 2), $$(3, 4))
PlTerm r2pl_matrix(Matrix<STRSXP> r, List aoptions)
{
  List options(aoptions) ;
  options("scalar") = false ;

  PlTermv rows(r.nrow()) ;
  for(int i=0 ; i<r.nrow() ; i++)
    rows[i] = r2pl_string(r.row(i), options) ;

  return PlCompound((const char*) options("charmat"), rows) ;
}

// Translate CharacterVector to (scalar) string or things like $("a", "b", "c")
PlTerm r2pl_string(CharacterVector r, List options)
{
  if(Rf_isMatrix(r))
    return r2pl_matrix(as<Matrix<STRSXP>>(r), options) ;

  if(r.length() == 0)
    return r2pl_null() ;
  
  LogicalVector na = is_na(r) ;
  
  // scalar string
  if(as<LogicalVector>(options["scalar"])(0) && r.length() == 1)
  {
    if(na[0])
      return r2pl_na() ;
    
    return PlString(r(0)) ;
  }

  // compound like $("a", "b", "c")
  size_t len = (size_t) r.length() ;
  PlTermv args(len) ;
  for(size_t i=0 ; i<len ; i++)
  {
    if(na[i])
      args[i] = r2pl_na() ;
    else
      args[i] = PlString(r(i)) ;
  }

  return PlCompound((const char*) options("charvec"), args) ;
}

// Translate R call to prolog compound, taking into account the names of the
// arguments, e.g., rexp(50, rate=1) -> rexp(50, =(rate, 1))
PlTerm r2pl_compound(Language r, CharacterVector& names, PlTerm& vars, List options)
{
  // For convenience, collect arguments in a list
  List l = as<List>(CDR(r)) ;

  // R functions with no arguments are translated to compounds (not atoms)
  size_t len = (size_t) l.size() ;
  if(len == 0)
  {
    PlTermv pl(3) ;
    pl[1] = as<Symbol>(CAR(r)).c_str() ;
    pl[2] = (long) 0 ;
    PlCall("compound_name_arity", pl) ;
    return pl[0] ;
  }

  // Extract names of arguments
  CharacterVector n ;
  // if there are no names, l.names() returns NULL and n has length 0
  if(TYPEOF(l.names()) == STRSXP)
    n = l.names() ;
  
  PlTermv pl(len) ;
  for(size_t i=0 ; i<len ; i++)
  {
    PlTerm arg = r2pl(l(i), names, vars, options) ;
    
    // Convert named arguments to prolog compounds a=X
    if(n.length() && n(i) != "")
      pl[i] = PlCompound("=", PlTermv(PlAtom(n(i)), arg)) ;
    else
      pl[i] = arg ; // no name
  }

  return PlCompound(as<Symbol>(CAR(r)).c_str(), pl) ;
}

// Translate R list to prolog list, taking into account the names of the
// elements, e.g., list(a=1, b=2) -> [a-1, b-2]. This may change, since the
// minus sign is a bit specific to prolog, and the conversion in the reverse
// direction may be ambiguous.
//
PlTerm r2pl_list(List r, CharacterVector& names, PlTerm& vars, List options)
{
  // Names of list elements (empty vector if r.names() == NULL)  
  CharacterVector n ;
  if(TYPEOF(r.names()) == STRSXP)
    n = as<CharacterVector>(r.names()) ;
  
  PlTerm pl ;
  PlTail tail(pl) ;
  for(R_xlen_t i=0; i<r.size() ; i++)
  {
    PlTerm arg = r2pl(r(i), names, vars, options) ;
    
    // Convert named argument to prolog pair a-X.
    if(n.length() && n(i) != "")
      tail.append(PlCompound("-", PlTermv(PlAtom(n(i)), arg))) ;
    else
      tail.append(arg) ; // no name
  }
  
  tail.close() ;
  return pl ;
}

// Translate R function to :- ("neck")
PlTerm r2pl_function(Function r, CharacterVector& names, PlTerm& vars, List options)
{
  PlTermv fun(2) ;
  fun[1] = r2pl_compound(BODY(r), names, vars, options) ;
  
  List formals = as<List>(FORMALS(r)) ;
  size_t len = (size_t) formals.size() ;
  if(len == 0)
  {
    PlTermv pl(3) ;
    pl[1] = PlAtom("function") ;
    pl[2] = (long) 0 ;
    PlCall("compound_name_arity", pl) ;

    fun[0] = pl[0] ;
    return PlCompound(":-", fun) ;
  }
  
  CharacterVector n = formals.names() ;
  PlTermv pl(len) ;
  for(size_t i=0 ; i<len ; i++)
    pl[i] = PlAtom(n(i)) ;

  fun[0] = PlCompound("function", pl) ;
  return PlCompound(":-", fun) ;
}

// Translate R primitive to :- ("neck")
PlTerm r2pl_builtin(Function r, CharacterVector& names, PlTerm& vars, List options)
{
  PlTermv fun(2) ;
  fun[1] = r2pl_null() ;
  
  List formals = as<List>(FORMALS(r)) ;
  size_t len = (size_t) formals.size() ;
  if(len == 0)
  {
    PlTermv pl(3) ;
    pl[1] = PlAtom("function") ;
    pl[2] = (long) 0 ;
    PlCall("compound_name_arity", pl) ;

    fun[0] = pl[0] ;
    return PlCompound(":-", fun) ;
  }
  
  CharacterVector n = formals.names() ;
  PlTermv pl(len) ;
  for(size_t i=0 ; i<len ; i++)
    pl[i] = PlAtom(n(i)) ;

  fun[0] = PlCompound("function", pl) ;
  return PlCompound(":-", fun) ;
}

PlTerm r2pl(SEXP r, CharacterVector& names, PlTerm& vars, List options)
{
  if(TYPEOF(r) == LANGSXP)
    return r2pl_compound(r, names, vars, options) ;

  if(TYPEOF(r) == REALSXP)
    return r2pl_real(r, options) ;
  
  if(TYPEOF(r) == LGLSXP)
    return r2pl_logical(r, options) ;
  
  if(TYPEOF(r) == INTSXP)
    return r2pl_integer(r, options) ;
  
  if(TYPEOF(r) == EXPRSXP)
    return r2pl_var(r, names, vars, options) ;

  if(TYPEOF(r) == SYMSXP)
    return r2pl_atom(r) ;

  if(TYPEOF(r) == STRSXP)
    return r2pl_string(r, options) ;

  if(TYPEOF(r) == VECSXP)
    return r2pl_list(r, names, vars, options) ;
  
  if(TYPEOF(r) == NILSXP)
    return r2pl_null() ;
  
  if(TYPEOF(r) == CLOSXP)
    return r2pl_function(r, names, vars, options) ;

  if(TYPEOF(r) == BUILTINSXP)
    return r2pl_builtin(r, names, vars, options) ;

  warning("r2pl: cannot translate R object of class %s, returning NA\n", 
    as<CharacterVector>(Function("class")(r))) ;
  return r2pl_na() ;
}

class RlQuery  
{
  CharacterVector names ;
  PlTerm vars ;
  List options ;
  Environment env ;
  PlQuery* qid ;
  
public:
  RlQuery(RObject aquery, List aoptions, Environment aenv) ;
  ~RlQuery() ;
  
  int next_solution() ;

  List bindings() ;

  const List& get_options() const 
  {
    return options ;
  }

  Environment& get_env()
  {
    return env ;
  }
} ;

RlQuery::RlQuery(RObject aquery, List aoptions, Environment aenv)
  : names(),
    vars(),
    options(aoptions),
    env(aenv),
    qid(NULL)
{
  options("atomize") = false ;
  PlTerm pl = r2pl(aquery, names, vars, options) ;
  qid = new PlQuery("call", PlTermv(PlTerm(pl))) ;
}

RlQuery::~RlQuery()
{
  if(qid)
    delete qid ;
}

int RlQuery::next_solution()
{
  if(qid == NULL)
    stop("next_solution: no open query.") ;
    
  int q ;
  try
  {
    q = qid->next_solution() ;
  }

  catch(PlException& ex)
  {
    char* err = (char*) ex ;
    PL_clear_exception() ;
    warning("Prolog exception: %s", err) ;
    stop(err) ;
  }

  return q ;
}

List RlQuery::bindings()
{
  List l ;

  PlTail tail(vars) ;
  PlTerm v ;
  for(int i=0 ; i<names.length() ; i++)
  {
    tail.next(v) ;
    RObject r = pl2r(v, names, vars, options) ;
    if(TYPEOF(r) == EXPRSXP && names[i] == as<Symbol>(as<ExpressionVector>(r)[0]).c_str())
    continue ;

    l.push_back(r, (const char*) names[i]) ;
  }

  return l ;
}

RlQuery* query_id = NULL ;

// Open a query for later use.
// [[Rcpp::export(.query)]]
RObject query_(RObject query, List options, Environment env)
{
  if(PL_current_query() != 0)
  {
    warning("Cannot raise simultaneous queries. Please invoke clear()") ;
    return wrap(false) ;
  }

  query_id = new RlQuery(query, options, env) ;
  return wrap(true) ;
}

// Clear query (and invoke cleanup handlers, see PL_close_query)
// [[Rcpp::export(.clear)]]
RObject clear_()
{
  if(query_id)
    delete query_id ;
  query_id = NULL ;

  return wrap(true) ;
}

// Submit query
// [[Rcpp::export(.submit)]]
RObject submit_()
{
  if(query_id == NULL)
  {
    warning("submit: no open query.") ;
    return wrap(false) ;
  }

  if(!query_id->next_solution())
  {
    delete query_id ;
    query_id = NULL ;
    return wrap(false) ;
  }

  return query_id->bindings() ;
}

// Execute a query once and return conditions
//
// Examples:
//
// once(call("=", 1, 2)) -> FALSE
// once(call("=", 1, 1)) -> empty list
// once(call("member", 1, list(2, expression(X)))) -> list stating that X = 1
// once(call("=", list(expression(X), expression(Y)), list(1, expression(Z))))
//   -> list stating that X = 1 and Z = Y
// once(call("member", 1, expression(X))) -> list stating that X = [1 | _]; 
//   e.g., something like [|]`(1, expression(`_6330`)). This is cumbersome, any
//   better ideas are welcome.
//
// [[Rcpp::export(.once)]]
RObject once_(RObject query, List options, Environment env)
{
  PlFrame f ;
  if(!query_(query, options, env))
    stop("Could not create query.") ;
    
  RObject l = submit_() ;
  clear_() ;
  return l ;
}

// Same as once_ above, but return all solutions to a query.
// [[Rcpp::export(.findall)]]
List findall_(RObject query, List options, Environment env)
{
  PlFrame f ;
  if(!query_(query, options, env))
    stop("Could not create query.") ;
    
  List results ;
  while(true)
  {
    RObject l = submit_() ;
    if(TYPEOF(l) == LGLSXP)
      break ;
    
    results.push_back(l) ;
  }
  
  clear_() ;
  return results ;
}

// Pretty print query. Maybe simplify to something like this:
// with_output_to(string(S), write_term(member(X), [variable_names(['X'=X])])).
//
// [[Rcpp::export(.portray)]]
RObject portray_(RObject query, List options)
{
  if(PL_current_query() != 0)
  {
    warning("Closing the current query.") ;
    clear_() ;
  }

  CharacterVector names ;
  PlTerm vars ;
  options("atomize") = true ; // translate variables to their R names
  PlTermv pl(3) ;
  pl[0] = r2pl(query, names, vars, options) ;
  PlTail tail(pl[2]) ;
  tail.append(PlCompound("quoted", PlTermv(PlAtom("false")))) ;
  tail.append(PlCompound("spacing", PlTermv(PlAtom("next_argument")))) ;
  tail.close() ;

  PlFrame f ;
  PlQuery q("term_string", pl) ;
  try
  {
    if(!q.next_solution())
      return wrap(false) ;
  }
  
  catch(PlException& ex)
  {
    char* err = (char*) ex ;
    PL_clear_exception() ;
    stop("portray of %s failed: %s", (char*) pl[0], err) ;
  }
  
  return pl2r(pl[1], names, vars, options) ;
}

// Call R expression from Prolog
PREDICATE(r_eval, 1)
{
  CharacterVector names ;
  PlTerm vars ;
  List options ;
  if(query_id)
    options = query_id->get_options() ;
  else
    options = List::create(Named("realvec") = "#", Named("realmat") = "##",
      Named("boolvec") = "!", Named("boolmat") = "!!",
      Named("charvec") = "$$", Named("charmat") = "$$$",
      Named("intvec") = "%", Named("intmat") = "%%", 
      Named("atomize") = false, Named("scalar") = true) ;

  RObject Expr = pl2r(A1, names, vars, options) ;
  RObject Res = Expr ;
  try
  {
    Environment env = query_id->get_env() ;
    Res = Language("dontCheck", Expr).eval(env) ;
  }

  catch(std::exception& ex)
  {
    throw PlException(PlCompound("r_eval", PlTermv(A1, PlAtom(ex.what())))) ;
  }

  catch(...)
  {
    throw PlException(PlCompound("r_eval", PlTermv(A1, PlString("exception occurred")))) ;
  }

  return true ;
}

// Evaluate R expression from Prolog
PREDICATE(r_eval, 2)
{
  CharacterVector names ;
  PlTerm vars ;
  List options ;
  if(query_id)
    options = query_id->get_options() ;
  else
    options = List::create(Named("realvec") = "#", Named("realmat") = "##",
      Named("boolvec") = "!", Named("boolmat") = "!!",
      Named("charvec") = "$$", Named("charmat") = "$$$",
      Named("intvec") = "%", Named("intmat") = "%%", 
      Named("atomize") = false, Named("scalar") = true) ;

  RObject Expr = pl2r(A1, names, vars, options) ;
  RObject Res = Expr ;
  try
  {
    Environment env = query_id->get_env() ;
    Res = Language("dontCheck", Expr).eval(env) ;
  }
  
  catch(std::exception& ex)
  {
    throw PlException(PlCompound("r_eval", PlTermv(A1, PlAtom(ex.what())))) ;
  }

  catch(...)
  {
    throw PlException(PlCompound("r_eval", PlTermv(A1, PlAtom("exception occurred")))) ;
  }

  PlTerm pl = r2pl(Res, names, vars, options) ;
  return A2 = pl ;
}

// The SWI system should not be initialized twice; therefore, we keep track of
// its status.
bool pl_initialized = false ;

// Initialize SWI-prolog. This needs a list of the command-line arguments of 
// the calling program, the most important being the name of the main 
// executable, argv[0]. I added "-q" to suppress SWI prolog's welcome message
// which is shown in .onAttach anyway.
// [[Rcpp::export(.init)]]
LogicalVector init_(String argv0)
{
  if(pl_initialized)
    warning("Please do not initialize SWI-prolog twice in the same session.") ;
  
  // Prolog documentation requires that argv is accessible during the entire 
  // session. I assume that this pointer is valid during the whole R session,
  // and that I can safely cast it to const.
  const int argc = 2 ;
  const char* argv[argc] ;
  argv[0] = argv0.get_cstring() ;
  argv[1] = "-q" ;
  if(!PL_initialise(argc, (char**) argv))
    stop("rolog_init: initialization failed.") ;

  pl_initialized = true ;  
  return true ;
}

// [[Rcpp::export(.done)]]
LogicalVector done_()
{
  if(!pl_initialized)
  {
    warning("rolog_done: swipl has not been initialized") ;
    return true ;
  }

  // Just in case there are open queries
  clear_() ;

  PL_cleanup(0) ;
  pl_initialized = false ;
  return true ;
}
