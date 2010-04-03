Please read this file before making any modifications to the test suite.

**********************************************************************
**********************************************************************
* Overview
**********************************************************************
**********************************************************************

The test suite is based on a shell script, 'run_tests.sh', which
calls pcb to export various test case layouts to different output
formats.  The tests to be run are defined in the file 'tests.list'.
The 'tests.list' file defines the export command line options passed
to pcb, the name of the input .pcb file, and the names and file types
for the expected output files.

After a particular test is run, the output files are compared against
a set of "golden" files which are in the golden/ subdirectory.
ALL CHANGES TO THE GOLDEN FILES MUST BE HAND VERIFIED.  This point
can not be emphasized too much.  

While this testsuite is clearly not comprehensive (the GUI is totally
left out of the testing for example), it is still better than nothing
and can help detect newly introduced bugs.

**********************************************************************
**********************************************************************
* Running an existing test
**********************************************************************
**********************************************************************

To run all of the tests defined in tests.list run:

  ./run_tests.sh

To only run a specific test or tests, then simply list them by name
on the command line like:

  ./run_tests.sh test_one test_two ... 

**********************************************************************
**********************************************************************
* Updating existing "golden" files
**********************************************************************
**********************************************************************

./run_tests.sh --regen <testname>

will regenerate the golden file for <testname>.  If you are generating
ASCII output such as BOMs or RS-274X files, then use the diff(3) program
to examine all differences.  If you are generating a graphics file
such as a PNG, then I suggest saving off a copy and using ImageMagick
to look for the differences visually.  The run_tests.sh script has
examples of comparing .png files.  Make sure the changes are only
the expected ones and then check the new files back into git.  Do
not blindly update these files as that defeats the purpose of the tests.

**********************************************************************
**********************************************************************
* Adding New Tests
**********************************************************************
**********************************************************************

----------------------------------------------------------------------
Create input files
----------------------------------------------------------------------

Create a *small* layout input file and put it in the inputs/
directory.  The goal is to have a file which tests one particular aspect
of the capabilities of pcb.

----------------------------------------------------------------------
Add to tests.list
----------------------------------------------------------------------

Add an entry to the tests.list file for your new tests.  Use existing
entries as an example.

----------------------------------------------------------------------
Generate the reference files
----------------------------------------------------------------------

Generate the reference files for your new tests using the following

./run_tests.sh --regen <new_test_name> 

where <new_test_name> is the name of your new test from tests.list.  If you
are adding multiple tests, then you can list them all on the same
command line.

*IMPORTANT*
Verify that the generated .png files for your new tests are correct.  These
files will have been placed in the golden/ subdirectory.

----------------------------------------------------------------------
Update Makefile.am's
----------------------------------------------------------------------

Update inputs/Makefile.am and golden/Makefile.am to include your new
files.  If you added new Makefile.am's then be sure to also update
configure.ac at the top level of the source tree.

----------------------------------------------------------------------
Add the new files to git
----------------------------------------------------------------------


