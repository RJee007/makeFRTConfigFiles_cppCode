Code to create FRT config files for a set of FFC (Finnish forest center) POCs (plot observation cases).
The code can be adapted to read in plot data from other inventories, too.
Contact: Ranjith Gopalakrishnan (ranjith.gopalakrishnan@uef.fi)

This C++ code was developed in CodeBlocks IDE (http://www.codeblocks.org/). It was compiled using the 
GNU GCC 8.1.0 compiler, but should compile with any generic C++ compiler.

The directory "sections" should contain three text (.txt) files: frt_begin.txt, ellipsoid.txt and last5sections.txt.
These are templates of various sections of the FRT config files. Please keep them as they are.

The C++ program takes two input csvs that contain information of the POCs (plot observation cases, also
known as "observations") and the FFC plots. Please see the two sample csvs named "pocs.csv" and
"strata.csv" for a tempelate to construct your own versions of these csvs.

Each line in "pocs.csv" represents a POC, and one FRT config file will be generated for each line.
The fields in this csv are:
sampleplotid:   Plot ID of the FFC plot
POC_ID:         The ID associated with the POC. It contains the satellite identifier (L8), the (L8) scene
                ID, the date of the image aquisition and the FFC plot ID.
date_L8:        Date of Landsat aquisition
time_L8:        Time of Landsat aquisition
fertilityclass: Fertility class of the FFC plot
lon,lat:        Lon/Lat co-ordinates of the plot (decimal degrees; WGS84)
num_strata:	Number of forest strata assocaited with FFC plot (and they are listed in "strata.csv")

Each line in the "strata.csv" is about one forest stratum of a particular FFC plot. The fields are:
sampleplotid: Plot ID of the FFC plot
treespecies:  Tree species ID of the stratum (as per FFC codes)
stemcount:    Stem count in the stratum
meandiameter: Mean diameter of trees in the stratum
meanheight:   Mean height of trees in the stratum

VERY IMPORTANT!! Please make sure that both these input csvs are sorted by sampleplotid (ascending order)
before running the program!

BEFORE COMPILING/RUNNING:
1. Make sure that an EMPTY "cfg_files_output" directory is present (at this level). This is where the
   FRT config files will be created.
2. Make sure that the directory "sections" is present, and contains three .txt files.
3. Make sure that the two csv files (one named something like "pocs.csv" and the other like "strata.csv") are present,
   and contain the relevant POC/plot information.
3. Edit the header portion of common.cpp (two lines, see below), to make sure that it is pointing to these two csv files.
   These two relevant lines in common.cpp are:
   string poc_csv_filename = "pocs.csv";
   string strata_csv_filename = "strata.csv";
4. Delete or move the old "log.txt", if needed.

Now, compile & run. Examine the log.txt file to make sure that the program read in values from the csvs as
expected, and exitted normally (exit status 0). If so, several .txt files 
(named like FRT_cfg_L8_191013_20150818_458880.txt) should be created in the
"cfg_files_output" directory. These are the input/config files for FRT. There should be one such file created for
each line of the pocs.csv file.

NOTE: The program produces no output on the run-console, while running. Hence, the only way to know it is running is seeing the
number of FRT config files in the "cfg_files_output" directory; this should increase steadily.

