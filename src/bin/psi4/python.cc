// Include Python's header file.
// Use "python-config --includes" to determine this location.

#define BOOST_FILESYSTEM_VERSION 3

//  As an example program, we don't want to use any deprecated features
#ifndef BOOST_FILESYSTEM_NO_DEPRECATED
#  define BOOST_FILESYSTEM_NO_DEPRECATED
#endif
#ifndef BOOST_SYSTEM_NO_DEPRECATED
#  define BOOST_SYSTEM_NO_DEPRECATED
#endif

#include <cstdio>
#include <boost/algorithm/string.hpp>
#include <boost/python.hpp>
#include <boost/filesystem.hpp>
#include <boost/algorithm/string.hpp>
#include <psiconfig.h>
#include <sstream>
#include "script.h"
#include "libchkpt/chkpt.hpp"
#include "libpsio/psio.hpp"
#include "liboptions/liboptions.h"
#include <libmints/molecule.h>
#include <libmints/pointgrp.h>
#include <libmints/benchmark.h>
#include <libmints/psimath.h>
#include <libmints/vector.h>
#include <libmints/matrix.h>
#include <lib3index/3index.h>
#include <libfunctional/superfunctional.h>
#include <libplugin/plugin.h>
#include <libparallel/parallel.h>
#include <map>

#include <libpsio/psio.hpp>

using namespace psi;
using namespace psi::functional;
using namespace boost;
using namespace boost::python;
using namespace std;

std::map<std::string, plugin_info> plugins;

namespace opt      { psi::PsiReturnType optking(psi::Options &); }

namespace psi {
    namespace mints    { PsiReturnType mints(Options &); }
    namespace deriv    { PsiReturnType deriv(Options &); }
    namespace mcscf    { PsiReturnType mcscf(Options &); }
    namespace scf      { PsiReturnType scf(Options &);   }
    namespace dfmp2    { PsiReturnType dfmp2(Options &); }
    namespace sapt     { PsiReturnType sapt(Options &);  }
    namespace dcft     { PsiReturnType dcft(Options &);  }

    namespace transqt  { PsiReturnType transqt(Options &);  }
    namespace transqt2 { PsiReturnType transqt2(Options &); }
    namespace ccsort   { PsiReturnType ccsort(Options&);    }
    namespace ccenergy { PsiReturnType ccenergy(Options&);  }
    namespace cctriples { PsiReturnType cctriples(Options&);  }
    namespace cchbar   { PsiReturnType cchbar(Options&);    }
    namespace cclambda { PsiReturnType cclambda(Options&);  }
    namespace ccdensity{ PsiReturnType ccdensity(Options&); }
    namespace oeprop   { PsiReturnType oeprop(Options&);    }

    extern int read_options(const std::string &name, Options & options, bool call_ipv1 = true,
      bool suppress_printing = false);
    extern FILE *outfile;
}

/**************************************************************************
 * Plug-In functions                                                      *
 **************************************************************************/

/**
    Python interface for loading plugins.

        Python:
            plugin_load("integrals.so")

    @param fullpathname Full path and filename of the plugin to load.
    @returns 0 if not loaded, 1 if loaded, 2 if already loaded.
*/

int py_psi_plugin_load(std::string fullpathname)
{
    int ret = 0;

    boost::filesystem::path pluginPath(fullpathname);
    std::string uc = boost::algorithm::to_upper_copy(pluginPath.stem().string());

    // Make sure the plugin isn't already loaded.
    if (plugins.count(uc) == 0) {
        plugins[uc] = plugin_load(fullpathname);
        fprintf(outfile, "%s loaded.\n", fullpathname.c_str());
        ret = 1;
    }
    else
        ret = 2;

    return ret;
}

/**
    Python interface for calling plugins.

        Python:
            plugin("integrals.so")

    @param fullpathname Used to identity loaded plugin.
    @returns The result from the plugin.
*/
int py_psi_plugin(std::string fullpathname)
{
    boost::filesystem::path pluginPath(fullpathname);
    std::string uc = boost::algorithm::to_upper_copy(pluginPath.stem().string());
    if (plugins.count(uc) == 0) {
        plugins[uc] = plugin_load(fullpathname);
        plugin_info& tmpinfo = plugins[uc];
        fprintf(outfile, "Reading options from the %s block\n", tmpinfo.name.c_str());
        fflush(outfile);
        tmpinfo.read_options(tmpinfo.name, Process::environment.options);
    }

    plugin_info& info = plugins[uc];

    // Call the plugin
    // Should be wrapped in a try/catch block.
    fprintf(outfile, "Calling plugin %s.\n", fullpathname.c_str());
    fflush(outfile);

    // Have the plugin copy the environment to get current options.
    info.init_plugin(Communicator::world, Process::environment, _default_chkpt_lib_, _default_psio_lib_);

    // Call the plugin
    int ret = info.plugin(Process::environment.options);

    return ret;
}

/**
    Python interface for closing plugin.

        Python:
            plugin_close("integrals.so")

    @param fullpathname Used to identity loaded plugin.
*/
void py_psi_plugin_close(std::string fullpathname)
{
    boost::filesystem::path pluginPath(fullpathname);
    std::string uc = boost::algorithm::to_upper_copy(pluginPath.stem().string());
    if (plugins.count(uc) > 0) {
        plugin_info& info = plugins[uc];
        plugin_close(info);
        plugins.erase(uc);
    }
}

/**
    Python interface for closing all plugins.

        Python:
            plugin_close_all()
*/
void py_psi_plugin_close_all()
{
    std::map<std::string, plugin_info>::const_iterator iter = plugins.begin();

    for (; iter != plugins.end(); ++iter)
        plugin_close(plugins[iter->first]);

    plugins.clear();
}

/**************************************************************************
 * End of Plug-In functions                                               *
 **************************************************************************/

int py_psi_optking()
{
    return opt::optking(Process::environment.options);
}

int py_psi_mints()
{
    return mints::mints(Process::environment.options);
}

int py_psi_deriv()
{
    return deriv::deriv(Process::environment.options);
}
double py_psi_mcscf()
{
  if (mcscf::mcscf(Process::environment.options) == Success) {
    return Process::environment.globals["CURRENT ENERGY"];
  }
  else
    return 0.0;

}
double py_psi_scf()
{
    if (scf::scf(Process::environment.options) == Success) {
        return Process::environment.globals["CURRENT ENERGY"];
    }
    else
        return 0.0;
}

double py_psi_dcft()
{
    if (dcft::dcft(Process::environment.options) == Success) {
        return Process::environment.globals["CURRENT ENERGY"];
    }
    else
        return 0.0;
}

double py_psi_dfmp2()
{
    if (dfmp2::dfmp2(Process::environment.options) == Success) {
        return Process::environment.globals["CURRENT ENERGY"];
    }
    else
        return 0.0;
}

double py_psi_sapt()
{
    if (sapt::sapt(Process::environment.options) == Success) {
        return Process::environment.globals["SAPT ENERGY"];
    }
    else
        return 0.0;
}

double py_psi_transqt()
{
    transqt::transqt(Process::environment.options);
    return 0.0;
}

double py_psi_transqt2()
{
    transqt2::transqt2(Process::environment.options);
    return 0.0;
}

double py_psi_ccsort()
{
    ccsort::ccsort(Process::environment.options);
    return 0.0;
}

double py_psi_ccenergy()
{
    if (ccenergy::ccenergy(Process::environment.options) == Success) {
        return Process::environment.globals["CURRENT ENERGY"];
    }
    else
        return 0.0;
}

double py_psi_cctriples()
{
    if (cctriples::cctriples(Process::environment.options) == Success) {
        return Process::environment.globals["CURRENT ENERGY"];
    }
    else
        return 0.0;
}

double py_psi_cchbar()
{
    cchbar::cchbar(Process::environment.options);
    return 0.0;
}

double py_psi_cclambda()
{
    cclambda::cclambda(Process::environment.options);
    return 0.0;
}

double py_psi_ccdensity()
{
    ccdensity::ccdensity(Process::environment.options);
    return 0.0;
}

double py_psi_oeprop()
{
    oeprop::oeprop(Process::environment.options);
    return 0.0;
}

char const* py_psi_version()
{
    return PSI_VERSION;
}

void py_psi_clean()
{
    PSIOManager::shared_object()->psiclean();
}

void py_psi_set_default_options_for_module(std::string const & name)
{
    read_options(name, Process::environment.options, false);

    if (plugins.count(name)) {
        // Easy reference
        plugin_info& info = plugins[name];

        // Tell the plugin to load in its options into the current environment.
        info.read_options(info.name, Process::environment.options);
    }
}

void py_psi_print_options()
{
    Process::environment.options.print();
}

void py_psi_print_global_options()
{
    Process::environment.options.print_globals();
}

void py_psi_print_out(std::string s)
{
    fprintf(outfile,"%s",s.c_str());
}

bool py_psi_set_option_string(std::string const & name, std::string const & value)
{
    Process::environment.options.set_str(name, value);

    string nonconst_key = name;
    Data& data = Process::environment.options.use(nonconst_key);

    if (data.type() == "string") {
        Process::environment.options.set_str(name, value);
    } else if (data.type() == "boolean") {
        if (boost::to_upper_copy(value) == "TRUE" || boost::to_upper_copy(value) == "YES" || \
          boost::to_upper_copy(value) == "ON")
            Process::environment.options.set_int(name, true);
        else if (boost::to_upper_copy(value) == "FALSE" || boost::to_upper_copy(value) == "NO" || \
          boost::to_upper_copy(value) == "OFF")
            Process::environment.options.set_int(name, false);
        else
            throw std::domain_error("Required option type is boolean, no boolean specified");
    }
    return true;
}

bool py_psi_set_option_int(std::string const & name, int value)
{
    Process::environment.options.set_int(name, value);
    return true;
}

// Right now this can only handle arrays of integers.
// Unable to handle strings.
bool py_psi_set_option_array(std::string const & name, python::list values)
{
    size_t n = len(values);

    // Reset the array to a known state (empty).
    Process::environment.options[name].reset();

    for (size_t i=0; i < n; ++i) {
        Process::environment.options[name].add(extract<double>(values[i]));
    }

    return true;
}

bool py_psi_set_global_option_string(std::string const & name, std::string const & value)
{
    //Process::environment.options.set_global_str(name, value);

    string nonconst_key = name;
    Data& data = Process::environment.options.use(nonconst_key);

    if (data.type() == "string") {
        Process::environment.options.set_global_str(name, value);
    } else if (data.type() == "boolean") {
        if (boost::to_upper_copy(value) == "TRUE" || boost::to_upper_copy(value) == "YES" || \
          boost::to_upper_copy(value) == "ON")
            Process::environment.options.set_global_int(name, true);
        else if (boost::to_upper_copy(value) == "FALSE" || boost::to_upper_copy(value) == "NO" || \
          boost::to_upper_copy(value) == "OFF")
            Process::environment.options.set_global_int(name, false);
        else
            throw std::domain_error("Required option type is boolean, no boolean specified");
    }
    return true;
}

bool py_psi_set_global_option_int(std::string const & name, int value)
{
    Process::environment.options.set_global_int(name, value);
    return true;
}
// Right now this can only handle arrays of integers.
// Unable to handle strings.
bool py_psi_set_global_option_array(std::string const & name, python::list values)
{
    size_t n = len(values);

    // Reset the array to a known state (empty).
    // Process::environment.options[name].reset();
    Process::environment.options.get_global(name).reset();

    for (size_t i=0; i < n; ++i) {
        Process::environment.options.get_global(name).add(extract<double>(values[i]));
    }

    return true;
}

object py_psi_get_option(const string& key)
{
    string nonconst_key = key;
    Data& data = Process::environment.options.use(nonconst_key);

    if (data.type() == "string")
        return str(data.to_string());
    else if (data.type() == "boolean" || data.type() == "integer")
        return object(data.to_integer());
    else if (data.type() == "double")
        return object(data.to_double());

    return object();
}

object py_psi_get_global_option(const string& key)
{
    string nonconst_key = key;
    Data& data = Process::environment.options.use(nonconst_key);

    if (data.type() == "string")
        return str(data.to_string());
    else if (data.type() == "boolean" || data.type() == "integer")
        return object(data.to_integer());
    else if (data.type() == "double")
        return object(data.to_double());

    return object();
}

void py_psi_set_active_molecule(shared_ptr<Molecule> molecule)
{
    Process::environment.set_molecule(molecule);
}

boost::shared_ptr<Molecule> py_psi_get_active_molecule()
{
    return Process::environment.molecule();
}

double py_psi_get_variable(const std::string & key)
{
    string uppercase_key = key;
    transform(uppercase_key.begin(), uppercase_key.end(), uppercase_key.begin(), ::toupper);
    return Process::environment.globals[uppercase_key];
}

void py_psi_set_memory(unsigned long int mem)
{
    Process::environment.set_memory(mem);
    fprintf(outfile,"\n  Memory set to %7.3f %s by Python script.\n",(mem > 1000000000 ? mem/1.0E9 : mem/1.0E6), \
        (mem > 1000000000 ? "GiB" : "MiB" ));
}

unsigned long int py_psi_get_memory()
{
    return Process::environment.get_memory();
}

void py_psi_set_n_threads(int nthread)
{
    Process::environment.set_n_threads(nthread);
}

int py_psi_get_n_threads()
{
    return Process::environment.get_n_threads();
}

BOOST_PYTHON_MODULE(PsiMod)
{
    def("version", py_psi_version);
    def("clean", py_psi_clean);

    // Benchmarks    
    def("benchmark_blas1", &psi::benchmark_blas1);
    def("benchmark_blas2", &psi::benchmark_blas2);
    def("benchmark_blas3", &psi::benchmark_blas3);
    def("benchmark_disk", &psi::benchmark_disk);
    def("benchmark_math", &psi::benchmark_math);
    def("benchmark_integrals", &psi::benchmark_integrals);
   
    // BLAS Static Wrappers 
    def("DGBMV", &psi::PSI_DGBMV);
    def("DGEMM", &psi::PSI_DGEMM);
    def("DGEMV", &psi::PSI_DGEMV);
    def("DGER", &psi::PSI_DGER);
    def("DSBMV", &psi::PSI_DSBMV);
    def("DSYMM", &psi::PSI_DSYMM);
    def("DSYMV", &psi::PSI_DSYMV);
    def("DSYR", &psi::PSI_DSYR);
    def("DSYR2", &psi::PSI_DSYR2);
    def("DSYR2K", &psi::PSI_DSYR2K);
    def("DSYRK", &psi::PSI_DSYRK);
    def("DTBMV", &psi::PSI_DTBMV);
    def("DTBSV", &psi::PSI_DTBSV);
    def("DTRMM", &psi::PSI_DTRMM);
    def("DTRMV", &psi::PSI_DTRMV);
    def("DTRSM", &psi::PSI_DTRSM);
    def("DTRSV", &psi::PSI_DTRSV);
    def("DROT", &psi::PSI_DROT);
    def("DSWAP", &psi::PSI_DSWAP);
    def("DSCAL", &psi::PSI_DSCAL);
    def("DAXPY", &psi::PSI_DAXPY);
    def("DCOPY", &psi::PSI_DCOPY);
    def("DDOT", &psi::PSI_DDOT);
    def("DNRM2", &psi::PSI_DNRM2);
    def("DASUM", &psi::PSI_DASUM);
    def("IDAMAX", &psi::PSI_IDAMAX);

    // LAPACK static wrappers
    
    def("DGEEV", &psi::PSI_DGEEV);
    def("DSYEV", &psi::PSI_DSYEV);
    def("DSYSV", &psi::PSI_DSYSV);
    def("DGETRF", &psi::PSI_DGETRF);
    def("DGETRS", &psi::PSI_DGETRS);
    def("DGETRI", &psi::PSI_DGETRI);
    def("DPOTRF", &psi::PSI_DPOTRF);
    def("DPOTRS", &psi::PSI_DPOTRS);
    def("DPOTRI", &psi::PSI_DPOTRI);
    /**
    def("DBDSDC", &psi::PSI_DBDSDC);
    def("DBDSQR", &psi::PSI_DBDSQR);
    def("DDISNA", &psi::PSI_DDISNA);
    def("DGBBRD", &psi::PSI_DGBBRD);
    def("DGBCON", &psi::PSI_DGBCON);
    def("DGBEQU", &psi::PSI_DGBEQU);
    def("DGBRFS", &psi::PSI_DGBRFS);
    def("DGBSV", &psi::PSI_DGBSV);
    def("DGBSVX", &psi::PSI_DGBSVX);
    def("DGBTRF", &psi::PSI_DGBTRF);
    def("DGBTRS", &psi::PSI_DGBTRS);
    def("DGEBAK", &psi::PSI_DGEBAK);
    def("DGEBAL", &psi::PSI_DGEBAL);
    def("DGEBRD", &psi::PSI_DGEBRD);
    def("DGECON", &psi::PSI_DGECON);
    def("DGEEQU", &psi::PSI_DGEEQU);
    def("DGEES", &psi::PSI_DGEES);
    def("DGEESX", &psi::PSI_DGEESX);
    def("DGEEVX", &psi::PSI_DGEEVX);
    def("DGEGS", &psi::PSI_DGEGS);
    def("DGEGV", &psi::PSI_DGEGV);
    def("DGEHRD", &psi::PSI_DGEHRD);
    def("DGELQF", &psi::PSI_DGELQF);
    def("DGELS", &psi::PSI_DGELS);
    def("DGELSD", &psi::PSI_DGELSD);
    def("DGELSS", &psi::PSI_DGELSS);
    def("DGELSX", &psi::PSI_DGELSX);
    def("DGELSY", &psi::PSI_DGELSY);
    def("DGEQLF", &psi::PSI_DGEQLF);
    def("DGEQP3", &psi::PSI_DGEQP3);
    def("DGEQPF", &psi::PSI_DGEQPF);
    def("DGERFS", &psi::PSI_DGERFS);
    def("DGERQF", &psi::PSI_DGERQF);
    def("DGESDD", &psi::PSI_DGESDD);
    def("DGESV", &psi::PSI_DGESV);
    def("DGESVX", &psi::PSI_DGESVX);
    def("DGETRF", &psi::PSI_DGETRF);
    def("DGETRI", &psi::PSI_DGETRI);
    def("DGETRS", &psi::PSI_DGETRS);
    def("DGGBAK", &psi::PSI_DGGBAK);
    def("DGGBAL", &psi::PSI_DGGBAL);
    def("DGGES", &psi::PSI_DGGES);
    def("DGGESX", &psi::PSI_DGGESX);
    def("DGGEV", &psi::PSI_DGGEV);
    def("DGGEVX", &psi::PSI_DGGEVX);
    def("DGGGLM", &psi::PSI_DGGGLM);
    def("DGGHRD", &psi::PSI_DGGHRD);
    def("DGGLSE", &psi::PSI_DGGLSE);
    def("DGGQRF", &psi::PSI_DGGQRF);
    def("DGGRQF", &psi::PSI_DGGRQF);
    def("DGGSVD", &psi::PSI_DGGSVD);
    def("DGGSVP", &psi::PSI_DGGSVP);
    def("DGTCON", &psi::PSI_DGTCON);
    def("DGTRFS", &psi::PSI_DGTRFS);
    def("DGTSV", &psi::PSI_DGTSV);
    def("DGTSVX", &psi::PSI_DGTSVX);
    def("DGTTRF", &psi::PSI_DGTTRF);
    def("DGTTRS", &psi::PSI_DGTTRS);
    def("DHGEQZ", &psi::PSI_DHGEQZ);
    def("DHSEIN", &psi::PSI_DHSEIN);
    def("DHSEQR", &psi::PSI_DHSEQR);
    def("DORGBR", &psi::PSI_DORGBR);
    def("DORGHR", &psi::PSI_DORGHR);
    def("DORGLQ", &psi::PSI_DORGLQ);
    def("DORGQL", &psi::PSI_DORGQL);
    def("DORGQR", &psi::PSI_DORGQR);
    def("DORGRQ", &psi::PSI_DORGRQ);
    def("DORGTR", &psi::PSI_DORGTR);
    def("DORMBR", &psi::PSI_DORMBR);
    def("DORMHR", &psi::PSI_DORMHR);
    def("DORMLQ", &psi::PSI_DORMLQ);
    def("DORMQL", &psi::PSI_DORMQL);
    def("DORMQR", &psi::PSI_DORMQR);
    def("DORMR3", &psi::PSI_DORMR3);
    def("DORMRQ", &psi::PSI_DORMRQ);
    def("DORMRZ", &psi::PSI_DORMRZ);
    def("DORMTR", &psi::PSI_DORMTR);
    def("DPBCON", &psi::PSI_DPBCON);
    def("DPBEQU", &psi::PSI_DPBEQU);
    def("DPBRFS", &psi::PSI_DPBRFS);
    def("DPBSTF", &psi::PSI_DPBSTF);
    def("DPBSV", &psi::PSI_DPBSV);
    def("DPBSVX", &psi::PSI_DPBSVX);
    def("DPBTRF", &psi::PSI_DPBTRF);
    def("DPBTRS", &psi::PSI_DPBTRS);
    def("DPOCON", &psi::PSI_DPOCON);
    def("DPOEQU", &psi::PSI_DPOEQU);
    def("DPORFS", &psi::PSI_DPORFS);
    def("DPOSV", &psi::PSI_DPOSV);
    def("DPOSVX", &psi::PSI_DPOSVX);
    def("DPOTRF", &psi::PSI_DPOTRF);
    def("DPOTRI", &psi::PSI_DPOTRI);
    def("DPOTRS", &psi::PSI_DPOTRS);
    def("DPTCON", &psi::PSI_DPTCON);
    def("DPTEQR", &psi::PSI_DPTEQR);
    def("DPTRFS", &psi::PSI_DPTRFS);
    def("DPTSV", &psi::PSI_DPTSV);
    def("DPTSVX", &psi::PSI_DPTSVX);
    def("DPTTRF", &psi::PSI_DPTTRF);
    def("DPTTRS", &psi::PSI_DPTTRS);
    def("DSBEV", &psi::PSI_DSBEV);
    def("DSBEVD", &psi::PSI_DSBEVD);
    def("DSBEVX", &psi::PSI_DSBEVX);
    def("DSBGST", &psi::PSI_DSBGST);
    def("DSBGV", &psi::PSI_DSBGV);
    def("DSBGVD", &psi::PSI_DSBGVD);
    def("DSBGVX", &psi::PSI_DSBGVX);
    def("DSBTRD", &psi::PSI_DSBTRD);
    def("DSGESV", &psi::PSI_DSGESV);
    def("DSTEBZ", &psi::PSI_DSTEBZ);
    def("DSTEDC", &psi::PSI_DSTEDC);
    def("DSTEGR", &psi::PSI_DSTEGR);
    def("DSTEIN", &psi::PSI_DSTEIN);
    def("DSTEQR", &psi::PSI_DSTEQR);
    def("DSTERF", &psi::PSI_DSTERF);
    def("DSTEV", &psi::PSI_DSTEV);
    def("DSTEVD", &psi::PSI_DSTEVD);
    def("DSTEVR", &psi::PSI_DSTEVR);
    def("DSTEVX", &psi::PSI_DSTEVX);
    def("DSYCON", &psi::PSI_DSYCON);
    def("DSYGST", &psi::PSI_DSYGST);
    def("DSYGV", &psi::PSI_DSYGV);
    def("DSYGVD", &psi::PSI_DSYGVD);
    def("DSYGVX", &psi::PSI_DSYGVX);
    def("DSYRFS", &psi::PSI_DSYRFS);
    def("DSYTRD", &psi::PSI_DSYTRD);
    def("DSYTRF", &psi::PSI_DSYTRF);
    def("DSYTRI", &psi::PSI_DSYTRI);
    def("DSYTRS", &psi::PSI_DSYTRS);
    def("DTBCON", &psi::PSI_DTBCON);
    def("DTBRFS", &psi::PSI_DTBRFS);
    def("DTBTRS", &psi::PSI_DTBTRS);
    def("DTGEVC", &psi::PSI_DTGEVC);
    def("DTGEXC", &psi::PSI_DTGEXC);
    def("DTGSEN", &psi::PSI_DTGSEN);
    def("DTGSJA", &psi::PSI_DTGSJA);
    def("DTGSNA", &psi::PSI_DTGSNA);
    def("DTGSYL", &psi::PSI_DTGSYL);
    def("DTRCON", &psi::PSI_DTRCON);
    def("DTREVC", &psi::PSI_DTREVC);
    def("DTREXC", &psi::PSI_DTREXC);
    def("DTRRFS", &psi::PSI_DTRRFS);
    def("DTRSEN", &psi::PSI_DTRSEN);
    def("DTRSNA", &psi::PSI_DTRSNA);
    def("DTRSYL", &psi::PSI_DTRSYL);
    def("DTRTRI", &psi::PSI_DTRTRI);
    def("DTRTRS", &psi::PSI_DTRTRS);
    def("DTZRQF", &psi::PSI_DTZRQF);
    def("DTZRZF", &psi::PSI_DTZRZF);
    **/

    // Options
    def("set_default_options_for_module", py_psi_set_default_options_for_module);
    def("set_active_molecule", py_psi_set_active_molecule);
    def("get_active_molecule", &py_psi_get_active_molecule);
    def("set_memory", py_psi_set_memory);
    def("get_memory", py_psi_get_memory);
    def("set_n_threads", &py_psi_set_n_threads);
    def("get_n_threads", &py_psi_get_n_threads);

    def("print_options", py_psi_print_options);
    def("print_global_options", py_psi_print_global_options);
    def("print_out", py_psi_print_out);

    def("set_option", py_psi_set_option_string);
    def("set_option", py_psi_set_option_int);
    def("set_option", py_psi_set_option_array);

    def("set_global_option", py_psi_set_global_option_string);
    def("set_global_option", py_psi_set_global_option_int);
    def("set_global_option", py_psi_set_global_option_array);

    def("get_option", py_psi_get_option);
    def("get_global_option", py_psi_get_global_option);

    def("get_variable", py_psi_get_variable);

    // plugins
    def("plugin_load",      py_psi_plugin_load);
    def("plugin",           py_psi_plugin);
    def("plugin_close",     py_psi_plugin_close);
    def("plugin_close_all", py_psi_plugin_close_all);

    // modules
//    def("input", py_psi_input);
//    def("cints", py_psi_cints);
    def("mints", py_psi_mints);
    def("deriv", py_psi_deriv);
//    def("cscf",  py_psi_cscf);
    def("mcscf", py_psi_mcscf);
    def("scf",   py_psi_scf);
    def("dcft", py_psi_dcft);
    def("dfmp2", py_psi_dfmp2);
    def("sapt", py_psi_sapt);
    def("optking", py_psi_optking);
    def("transqt", py_psi_transqt);
    def("transqt2", py_psi_transqt2);
    def("ccsort", py_psi_ccsort);
    def("ccenergy", py_psi_ccenergy);
    def("cctriples", py_psi_cctriples);
    def("cchbar", py_psi_cchbar);
    def("cclambda", py_psi_cclambda);
    def("ccdensity", py_psi_ccdensity);
    def("oeprop", py_psi_oeprop);

    // Define library classes
    class_<PSIO, shared_ptr<PSIO> >( "IO" ).
        def( "state", &PSIO::state ).
        def( "open", &PSIO::open ).
        def( "close", &PSIO::close ).
        def( "rehash", &PSIO::rehash ).
        def( "open_check", &PSIO::open_check ).
        def( "tocclean", &PSIO::tocclean ).
        def( "tocprint", &PSIO::tocprint ).
        def( "tocwrite", &PSIO::tocwrite ).
        def( "shared_object", &PSIO::shared_object).
        staticmethod("shared_object").
        def( "get_default_namespace", &PSIO::get_default_namespace).
        staticmethod("get_default_namespace").
        def( "set_default_namespace", &PSIO::set_default_namespace).
        staticmethod("set_default_namespace").
        def( "change_file_namespace", &PSIO::change_file_namespace).
        staticmethod("change_file_namespace");

    class_<PSIOManager, shared_ptr<PSIOManager> >( "IOManager" ).
        def( "shared_object", &PSIOManager::shared_object ).
        staticmethod("shared_object").
        def( "print_out", &PSIOManager::print_out ).
        def( "psiclean", &PSIOManager::psiclean ).
        def( "crashclean", &PSIOManager::crashclean ).
        def( "mark_file_for_retention", &PSIOManager::mark_file_for_retention ).
        def( "set_default_path", &PSIOManager::set_default_path ).
        def( "set_specific_path", &PSIOManager::set_specific_path ).
        def( "set_specific_retention", &PSIOManager::set_specific_retention );

    class_<Chkpt, shared_ptr<Chkpt> >( "Checkpoint", init<PSIO*, int>() ).
        add_property( "enuc", &Chkpt::rd_enuc, &Chkpt::wt_enuc).
        add_property( "label", &Chkpt::rd_label, &Chkpt::wt_label).
        add_property( "escf", &Chkpt::rd_escf, &Chkpt::wt_escf).
        add_property( "eref", &Chkpt::rd_eref, &Chkpt::wt_eref).
        add_property( "ecorr", &Chkpt::rd_ecorr, &Chkpt::wt_ecorr).
        add_property( "efzc", &Chkpt::rd_efzc, &Chkpt::wt_efzc).
        add_property( "etot", &Chkpt::rd_etot, &Chkpt::wt_etot).
        add_property( "disp", &Chkpt::rd_disp, &Chkpt::wt_disp).
        add_property( "eccsd", &Chkpt::rd_eccsd, &Chkpt::wt_eccsd).
        add_property( "e_t", &Chkpt::rd_e_t, &Chkpt::wt_e_t).
        add_property( "emp2", &Chkpt::rd_emp2, &Chkpt::wt_emp2).
        def( "shared_object", &Chkpt::shared_object).
        staticmethod("shared_object");

    class_<Matrix, shared_ptr<Matrix> >( "Matrix").
        def(init<int, int>()).
        def("get", &Matrix::get).
        def("set", &Matrix::set_python).
        def("set_name", &Matrix::set_name).
        def("print_out", &Matrix::print_out);
    
    class_<Vector, shared_ptr<Vector> >( "Vector").
        def(init<int>()).
        def("get", &Vector::get).
        def("set", &Vector::set_python).
        def("print_out", &Vector::print_out);

    class_<IntVector, shared_ptr<IntVector> >( "IntVector").
        def(init<int>()).
        def("get", &IntVector::get).
        def("set", &IntVector::set_python).
        def("print_out", &IntVector::print_out);

    /**
    class_<DFTensor, shared_ptr<DFTensor> >( "DFTensor", no_init).
        def("bootstrap", &DFTensor::bootstrap_DFTensor).
        staticmethod("booststrap").
        def("form_fitting_metric", &DFTensor::form_fitting_metric).
        def("form_cholesky_metric", &DFTensor::form_cholesky_metric).
        def("form_qr_metric", &DFTensor::form_qr_metric).
        def("finalize", &DFTensor::finalize).
        def("print_out", &DFTensor::print_python);
    **/

    class_<Vector3>("Vector3").
        def(init<double>()).
        def(init<double, double, double>()).
        def(init<const Vector3&>()).
//      def(self = other<double>()).
        def(self += self).
        def(self -= self).
        def(self *= other<double>()).
        def(self + self).
        def(self - self).
        def(-self).
        def("dot", &Vector3::dot).
        def("distance", &Vector3::distance).
        def("normalize", &Vector3::normalize).
        def("norm", &Vector3::norm).
        def("cross", &Vector3::cross).
        def("__str__", &Vector3::to_string).
        def("__getitem__", &Vector3::get);

    typedef string (Process::Environment::*environmentStringFunction)(const string&);

    class_<Process::Environment>("Environment").
        def("__getitem__", environmentStringFunction(&Process::Environment::operator ()));
//        def("set", &Process::Environment::set);

    typedef string (Process::Arguments::*argumentsStringFunction)(int);

    class_<Process::Arguments>("Arguments").
        def("__getitem__", argumentsStringFunction(&Process::Arguments::operator ()));

    class_<Process>("Process").
        add_static_property("environment", &Process::environment);

    typedef void (SymmetryOperation::*intFunction)(int);
    typedef void (SymmetryOperation::*doubleFunction)(double);

    class_<SymmetryOperation>("SymmetryOperation").
        def(init<const SymmetryOperation& >()).
        def("trace", &SymmetryOperation::trace).
        def("zero", &SymmetryOperation::zero).
        def("operate", &SymmetryOperation::operate).
        def("transform", &SymmetryOperation::transform).
        def("unit", &SymmetryOperation::unit).
        def("E", &SymmetryOperation::E).
        def("i", &SymmetryOperation::i).
        def("sigma_h", &SymmetryOperation::sigma_h).
        def("sigma_xz", &SymmetryOperation::sigma_xz).
//        def("sigma_yz", &SymmetryOperation::sigma_yz).
        def("rotate_n", intFunction(&SymmetryOperation::rotation)).
        def("rotate_theta", doubleFunction(&SymmetryOperation::rotation)).
        def("c2_x", &SymmetryOperation::c2_x).
        def("c2_y", &SymmetryOperation::c2_y).
        def("transpose", &SymmetryOperation::transpose);

    class_<PointGroup, shared_ptr<PointGroup> >("PointGroup").
        def(init<const char*>()).
        def("symbol", &PointGroup::symbol).
        //def("origin", &PointGroup::origin).
        def("set_symbol", &PointGroup::set_symbol);

    class_<Molecule, shared_ptr<Molecule> >("Molecule").
        def("set_name", &Molecule::set_name).
        def("get_name", &Molecule::get_name).
        def("fix_orientation", &Molecule::set_orientation_fixed).
        def("init_with_checkpoint", &Molecule::init_with_chkpt).
        def("save_to_checkpoint", &Molecule::save_to_chkpt).
        def("init_with_io", &Molecule::init_with_psio).
        def("add_atom", &Molecule::add_atom).
        def("natom", &Molecule::natom).
        def("nfragments", &Molecule::num_fragments).
        def("print_out", &Molecule::print).
        def("update_geometry", &Molecule::update_geometry).
        def("Z", &Molecule::Z).
        def("x", &Molecule::x).
        def("y", &Molecule::y).
        def("z", &Molecule::z).
        //def("xyz", &Molecule::xyz).
        def("center_of_mass", &Molecule::center_of_mass).
        def("translate", &Molecule::translate).
        def("move_to_com", &Molecule::move_to_com).
        def("mass", &Molecule::mass).
        def("label", &Molecule::label).
        def("charge", &Molecule::charge).
        def("molecular_charge", &Molecule::molecular_charge).
        def("extract_subsets", &Molecule::py_extract_subsets_1).
        def("extract_subsets", &Molecule::py_extract_subsets_2).
        def("extract_subsets", &Molecule::py_extract_subsets_3).
        def("extract_subsets", &Molecule::py_extract_subsets_4).
        def("extract_subsets", &Molecule::py_extract_subsets_5).
        def("extract_subsets", &Molecule::py_extract_subsets_6).
        def("activate_all_fragments", &Molecule::activate_all_fragments).
        def("deactivate_all_fragments", &Molecule::deactivate_all_fragments).
        def("set_active_fragments", &Molecule::set_active_fragments).
        def("set_active_fragment", &Molecule::set_active_fragment).
        def("set_ghost_fragments", &Molecule::set_ghost_fragments).
        def("set_ghost_fragment", &Molecule::set_ghost_fragment).
        def("atom_at_position", &Molecule::atom_at_position1).
        def("print_to_output", &Molecule::print).
        def("nuclear_repulsion_energy", &Molecule::nuclear_repulsion_energy).
        def("reorient", &Molecule::reorient).
        def("find_point_group", &Molecule::find_point_group).
        def("set_point_group", &Molecule::set_point_group).
        def("schoenflies_symbol", &Molecule::schoenflies_symbol).
        def("form_symmetry_information", &Molecule::form_symmetry_information).
        def("create_molecule_from_string", &Molecule::create_molecule_from_string).
        staticmethod("create_molecule_from_string").
        def("is_variable", &Molecule::is_variable).
        def("set_variable", &Molecule::set_variable).
        def("get_variable", &Molecule::get_variable).
        def("update_geometry", &Molecule::update_geometry);

    class_<SuperFunctional, shared_ptr<SuperFunctional> >("SuperFunctional").
        def("create_superfunctional", &SuperFunctional::createSuperFunctional).
        staticmethod("create_superfunctional").
        def("build_superfunctional", &SuperFunctional::buildSuperFunctional).
        staticmethod("build_superfunctional").
        def("available_superfunctionals", &SuperFunctional::availableSuperFunctionals).
        staticmethod("available_superfunctionals").
        def("available_names", &SuperFunctional::availableNames).
        staticmethod("available_names").
        def("test_superfunctionals", &SuperFunctional::testSuperFunctionals).
        staticmethod("test_superfunctionals").
        def("test_superfunctional", &SuperFunctional::testSuperFunctional).

        def("get_name", &SuperFunctional::getName).
        def("get_description", &SuperFunctional::getDescription).
        def("get_citation", &SuperFunctional::getCitation).
        def("get_composition", &SuperFunctional::getComposition).
        def("get_exact_exchange", &SuperFunctional::getExactExchange).
        def("get_pt2", &SuperFunctional::getPT2).
        def("get_omega", &SuperFunctional::getOmega).
        def("get_dash_d_weight", &SuperFunctional::getDashDWeight).
        def("get_dash_d", &SuperFunctional::getDashD).
        def("get_npoints", &SuperFunctional::getNPoints).
        def("get_deriv", &SuperFunctional::getDeriv).
        def("get_functional", &SuperFunctional::getFunctional).
        def("get_weight", &SuperFunctional::getWeight).
        def("get_size", &SuperFunctional::size).

        def("set_name", &SuperFunctional::setName).
        def("set_description", &SuperFunctional::setDescription).
        def("set_citation", &SuperFunctional::setCitation).
        def("set_parameter", &SuperFunctional::setParameter).
        def("set_exact_exchange", &SuperFunctional::setExactExchange).
        def("set_pt2", &SuperFunctional::setPT2).
        def("set_omega", &SuperFunctional::setOmega).
        def("set_dash_d", &SuperFunctional::setDashD).
        def("set_npoints", &SuperFunctional::setNPoints).
        def("set_deriv", &SuperFunctional::setDeriv).
        def("set_size", &SuperFunctional::size).

        def("is_gga", &SuperFunctional::isGGA).
        def("is_meta", &SuperFunctional::isMeta).
        def("is_hybrid", &SuperFunctional::isHybrid).
        def("is_double_hybrid", &SuperFunctional::isDoubleHybrid).
        def("is_range_corrected", &SuperFunctional::isRangeCorrected).
        def("is_dash_d", &SuperFunctional::isDashD).

        //def("add_functional", &SuperFunctional::addFunctional).

        def("computeRKSFunctional", &SuperFunctional::computeRKSFunctional).
        def("computeUKSFunctional", &SuperFunctional::computeUKSFunctional);

    class_<Functional, shared_ptr<Functional> >("Functional").
        def("create_functional", &Functional::createFunctional).
        staticmethod("create_functional").
        def("available_functionals", &Functional::availableFunctionals).
        staticmethod("available_functionals").
        def("available_names", &Functional::availableNames).
        staticmethod("available_names").
        def("test_functionals", &Functional::testFunctionals).
        staticmethod("test_functionals").
        def("test_functional", &Functional::testFunctional).

        def("get_name", &Functional::getName).
        def("get_description", &Functional::getDescription).
        def("get_citation", &Functional::getCitation).
        def("get_parameters_string", &Functional::getParametersString).
        def("get_parameters", &Functional::getParameters).
        def("get_npoints", &Functional::getNPoints).
        def("get_deriv", &Functional::getDeriv).
        def("get_density_cutoff", &Functional::getDensityCutoff).

        def("set_name", &Functional::setName).
        def("set_description", &Functional::setDescription).
        def("set_citation", &Functional::setCitation).
        def("set_parameter", &Functional::setParameter).
        def("set_parameters", &Functional::setParameters).
        def("set_npoints", &Functional::setNPoints).
        def("set_deriv", &Functional::setDeriv).
        def("set_density_cutoff", &Functional::setDensityCutoff).

        def("is_gga", &Functional::isGGA).
        def("is_meta", &Functional::isMeta).

        def("computeRKSFunctional", &Functional::computeRKSFunctional).
        def("computeUKSFunctional", &Functional::computeUKSFunctional);
}

Python::Python() : Script()
{

}

Python::~Python()
{

}

void Python::initialize()
{
}

void Python::finalize()
{
}

void Python::run(FILE *input)
{
    using namespace boost::python;
    char *s;
    if (input == NULL)
        return;

    // Setup globals options
    Process::environment.options.set_read_globals(true);
    read_options("", Process::environment.options, false, true);
    Process::environment.options.set_read_globals(false);

    if (!Py_IsInitialized()) {
        s = strdup("psi");
        // Py_InitializeEx(0) causes sig handlers to not be installed.
        Py_InitializeEx(0);
        #if PY_VERSION_HEX >= 0x03000000
        Py_SetProgramName(L"psi");
        #else
        Py_SetProgramName(s);
        #endif

        // Track down the location of PSI4's python script directory.
        std::string psiDataDirName = Process::environment("PSIDATADIR") + "/python";
        boost::filesystem::path bf_path;
        bf_path = boost::filesystem::system_complete(psiDataDirName);
        if(!boost::filesystem::is_directory(bf_path))
            throw PSIEXCEPTION("Unable to read the Python folder - check the PSIDATADIR environmental variable");

        // Add PSI library python path
        PyObject *path, *sysmod, *str;
        sysmod = PyImport_ImportModule("sys");
        path = PyObject_GetAttrString(sysmod, "path");
        str = PyString_FromString(psiDataDirName.c_str());
        PyList_Append(path, str);
        Py_DECREF(str);
        Py_DECREF(path);
        Py_DECREF(sysmod);
    }
    if (Py_IsInitialized()) {
        // Stupid way to read in entire file.
        char line[256];
        std::stringstream file;
        while(fgets(line, sizeof(line), input)) {
            file << line;
        }

        // Process the input file
        PyObject *input = PyImport_ImportModule("input");
        PyObject *function = PyObject_GetAttrString(input, "process_input");
        PyObject *pargs = Py_BuildValue("(s)", file.str().c_str());
        PyObject *ret = PyEval_CallObject(function, pargs);

        char *val;
        PyArg_Parse(ret, "s", &val);
        string inputfile = val;

        Py_DECREF(ret);
        Py_DECREF(pargs);
        Py_DECREF(function);
        Py_DECREF(input);

        // str is a Boost Python C++ wrapper for Python strings.
//        str strStartScript(file.str().c_str());
        str strStartScript(inputfile);

        if (verbose) {
            fprintf(outfile, "Input file to run:\n%s", inputfile.c_str());
            fflush(outfile);
        }

        try {
            PyImport_AppendInittab(strdup("PsiMod"), initPsiMod);
            object objectMain(handle<>(borrowed(PyImport_AddModule("__main__"))));
            object objectDict = objectMain.attr("__dict__");
            s = strdup("import PsiMod");
            PyRun_SimpleString(s);

            object objectScriptInit = exec( strStartScript, objectDict, objectDict );
        }
        catch (error_already_set const& e)
        {
            PyErr_Print();
        }
    }
    else {
        fprintf(stderr, "Unable to run Python input file.\n");
        return;
    }

    py_psi_plugin_close_all();
}
