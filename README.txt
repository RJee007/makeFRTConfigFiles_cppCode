Code to create a FRT config file for each SMK (Suomen metsäkeskus) plot-aquisition-instance.

This C++ code was developed in CodeBlocks IDE (http://www.codeblocks.org/); it is best to compile
and run in this environment. The project file is hence "run_multiple_times.cpb".

The directory "sections" contains the templates of various sections of the FRT config files.

TO RUN:
1. Make sure that an empty "cfg_files_output" directory is present (at this level) and empty. This is where the
   FRT config files will be created.
2. Make sure that the two csv files (one for sampleplots and one for strata) are present.
3. Edit the header portion of common.cpp, to make sure that it is pointing to these csv files.
4. Delete or move the old "log.txt", if needed

NOTE: The program produces no output on the run-console!! Hence, the only way to know it is running is seeing the
number of FRT config files in the "cfg_files_output" directory; this should increase steadily.