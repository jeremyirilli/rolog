// Generated by using Rcpp::compileAttributes() -> do not edit by hand
// Generator token: 10BE3573-1514-4C36-9D1C-5A225CD40393

#include <Rcpp.h>

using namespace Rcpp;

// init_
LogicalVector init_(String argv0);
RcppExport SEXP _rolog_init_(SEXP argv0SEXP) {
BEGIN_RCPP
    Rcpp::RObject rcpp_result_gen;
    Rcpp::RNGScope rcpp_rngScope_gen;
    Rcpp::traits::input_parameter< String >::type argv0(argv0SEXP);
    rcpp_result_gen = Rcpp::wrap(init_(argv0));
    return rcpp_result_gen;
END_RCPP
}
// done_
LogicalVector done_();
RcppExport SEXP _rolog_done_() {
BEGIN_RCPP
    Rcpp::RObject rcpp_result_gen;
    Rcpp::RNGScope rcpp_rngScope_gen;
    rcpp_result_gen = Rcpp::wrap(done_());
    return rcpp_result_gen;
END_RCPP
}
// consult_
LogicalVector consult_(CharacterVector files);
RcppExport SEXP _rolog_consult_(SEXP filesSEXP) {
BEGIN_RCPP
    Rcpp::RObject rcpp_result_gen;
    Rcpp::RNGScope rcpp_rngScope_gen;
    Rcpp::traits::input_parameter< CharacterVector >::type files(filesSEXP);
    rcpp_result_gen = Rcpp::wrap(consult_(files));
    return rcpp_result_gen;
END_RCPP
}
// call_
LogicalVector call_(RObject lang);
RcppExport SEXP _rolog_call_(SEXP langSEXP) {
BEGIN_RCPP
    Rcpp::RObject rcpp_result_gen;
    Rcpp::RNGScope rcpp_rngScope_gen;
    Rcpp::traits::input_parameter< RObject >::type lang(langSEXP);
    rcpp_result_gen = Rcpp::wrap(call_(lang));
    return rcpp_result_gen;
END_RCPP
}
// findall_
List findall_(RObject lang);
RcppExport SEXP _rolog_findall_(SEXP langSEXP) {
BEGIN_RCPP
    Rcpp::RObject rcpp_result_gen;
    Rcpp::RNGScope rcpp_rngScope_gen;
    Rcpp::traits::input_parameter< RObject >::type lang(langSEXP);
    rcpp_result_gen = Rcpp::wrap(findall_(lang));
    return rcpp_result_gen;
END_RCPP
}

static const R_CallMethodDef CallEntries[] = {
    {"_rolog_init_", (DL_FUNC) &_rolog_init_, 1},
    {"_rolog_done_", (DL_FUNC) &_rolog_done_, 0},
    {"_rolog_consult_", (DL_FUNC) &_rolog_consult_, 1},
    {"_rolog_call_", (DL_FUNC) &_rolog_call_, 1},
    {"_rolog_findall_", (DL_FUNC) &_rolog_findall_, 1},
    {NULL, NULL, 0}
};

RcppExport void R_init_rolog(DllInfo *dll) {
    R_registerRoutines(dll, NULL, CallEntries, NULL, NULL);
    R_useDynamicSymbols(dll, FALSE);
}
