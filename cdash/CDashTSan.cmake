#
# CTest script for thread sanitizer (TSan) memcheck build and submission to dashboard 
# Warning: the correct flags are to be set through a separate (and prior) call to
# the setup script!
#
# Written by Roberto Di Remigio November 2014 
#

find_program(HOSTNAME_COMMAND NAMES hostname)
execute_process(COMMAND ${HOSTNAME_COMMAND} 
    OUTPUT_VARIABLE CTEST_SITE
    ERROR_QUIET OUTPUT_STRIP_TRAILING_WHITESPACE)

include(ProcessorCount)
ProcessorCount(NCORES)
if(NOT NCORES EQUAL 0)
    set(CTEST_BUILD_FLAGS -j${NCORES})
else()
    set(NCORES 1)
endif()

set(CTEST_MEMORYCHECK_TYPE "ThreadSanitizer")
ctest_start(Experimental)
ctest_configure()
ctest_build()
ctest_test(PARALLEL_LEVEL ${NCORES})
ctest_memcheck(PARALLEL_LEVEL ${NCORES})
ctest_submit()

# vim:et:sw=4:ts=4:
