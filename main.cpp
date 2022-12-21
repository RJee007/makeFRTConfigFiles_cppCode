// #################### HELP DOC BLOCK BEGIN  ######################
// Using lists: https://www.geeksforgeeks.org/list-cpp-stl/
// Using strings etc http://www.cplusplus.com/doc/tutorial/variables/
// #################### HELP DOC BLOCK END    ######################

// Using the log file: The following string signals that a (fatal) error has occurred:
// "ERROR!! ERROR!!"

// #################### TODO BLOCK BEGIN  ######################
//
// RENAMING CLASSES AND VARIABLES:
// Some variables were from the time when each row in 'sampleplot.csv' represented
// a single sampleplot. For example, 'currSP'. Now, each line in sampleplot represents
// a 'plot aquisition instance'.
//
// THE BOOST EFFORT:
// It would be good to start using boost libraries. But this seems to be quite hard, as it does not integrate
// that easily with codeblocks. I have installed it (installed_sw\cpp_Boost_library) and tried using it, but
// getting it to compile/link in is not that easy (http://wiki.codeblocks.org/index.php/BoostWindowsQuickRef).
// For example, the task of replacing substrings is much easier with Boost:
// https://stackoverflow.com/questions/4643512/replace-substring-with-another-substring-c
//
// #################### TODO BLOCK END    ######################

// TODO!! TODO!!  TODO!!  Better way of doing this???
#include "D:\ranjitg\projects\FORBIO_project\albedo_work\run_multiple_times\run_multiple_times\common.cpp"
#include "D:\ranjitg\projects\FORBIO_project\albedo_work\run_multiple_times\run_multiple_times\csvs.cpp"
#include "D:\ranjitg\projects\FORBIO_project\albedo_work\run_multiple_times\run_multiple_times\spa.cpp"

int main(){

    // snowUnderstory: If this variable is set, it means that snow covers the understory of all
    // plots. That is, instead of using understory spectra file such as "herb_rich_Nov2019.txt",
    // "mesic_Nov2019.txt" and "xeric_Nov2019.txt", a spectra file such as "snow_feb2021.txt" is
    // used throught! That is, it is used for ALL plots!!
    bool isUnderstorySnow = false;

    if (isUnderstorySnow){
        cout << "\n\n IMPORTANT!! Snow understory flag is set. It is assumed that ALL plots have understories fully covered by snow. \n\n";
    }
    cout << " Program running, please wait...";
    cout <<  "All output is redirected to log.txt file...";
    cout << " The FRT config (input) files should be created in the cfg_files_output directory...";
    cout.flush();

    // All 'cout' messages are redirected to this log file
    freopen( "log.txt", "w", stdout );
    if (isUnderstorySnow){
        cout << "\n\n IMPORTANT!! Snow understory flag is set. It is assumed that ALL plots have understories fully covered by snow. \n\n";
    }

    // Open the poc (plot-observation-case) csv file
    std::ifstream file(poc_csv_filename);
    CSVRow row;
    file >> row; // Read off the first row (the header)
    int num_strata = -1; // Number of strata for the current sampleplot

    // sampleplotid associated with the current strata set (css) in memory (explained later)
    string spid_css = "";
    // List of strata associated with spid_css
    list <Stratum*> l_css;
    bool isStratumListValid = false;

    // Open the strata csv file
    std::ifstream file_st(strata_csv_filename);
    CSVRow row_st; // row in strata table
    file_st >> row_st; // read off the first row (header info; can be discarded)

    // Now, iterate over each line in the poc csv file.
    // Each time, a line will be read off the csv and "row" will be populated with its contents.
    // That is, row[0] will be the element in the first column, row[1] will be the element in the
    // second column, etc.
    while(file >> row){

        string sp_id = row[0]; // The sampleplot ID
        // sp_id is all whitespace; we have reached the end of the csv file
        if(std::string::npos == sp_id.find_first_not_of(' '))
        {
            // exit from while loop
            break;
        }

        string poc_ID = row[1]; // The POC ID (L8 aquisition over the given plot).
        // Define a new 'POC_Instance' object (currPOC) to contain all the related info.

        // NOTE!!!! CHANGED!!!
        // Was:
        // PlotAqInstance currSP(plotAq_ID);
        POC_Instance currPOC(poc_ID);

        // Now, fill this "currPOC" object with relevant information...
        try{
            cout << "Starting new plot acquisition instance, poc_ID is: " << poc_ID << endl;
            cout << "Sampleplotid is: " << sp_id << endl;

            // **************** BEGIN BLOCK: SOLAR POSITION CODE *******************//
            // TODO: Make sure all is ok here. https://www.esrl.noaa.gov/gmd/grad/solcalc/azel.html
            // Takes the date_L8 and time_L8 column values from the poc.csv file and
            // calculates the sun azimuth and elevation.
            // Also see: spa.cpp.
            //
            // Set input parameters: Date, Time (UTC) and location
            SPAInput in_spa;
            SPAOutput out_spa;
            string dayString = string(row[2]); // the date_L8 column in the poc csv
            in_spa.year = atoi((dayString.substr(0,4)).c_str());
            in_spa.month = atoi((dayString.substr(4,2)).c_str());
            if (isUnderstorySnow && (in_spa.month<2 || in_spa.month>3)){
               cout << "Unexpected values of isUnderstorySnow and month...exiting!!" << sp_id << endl;
               cout.flush();
               exit(1);
            }
            in_spa.day   = atoi((dayString.substr(6,2)).c_str());
            string timeString = string(row[3]); // the time_L8 column in the samplePlots csv
            // Time is in UTC, so no need to convert it.
            // See mail from Petteri, "Question about your solar position calculator code"
            // Feb 2020.
            in_spa.hour = atoi((timeString.substr(0,1)).c_str());
            in_spa.minute = atoi((timeString.substr(2,2)).c_str());
            in_spa.second = atoi((timeString.substr(5,2)).c_str());
            // Both lat and lon are positive for us, as we are in the North
            // hemisphere and east of the prime meridian
            in_spa.lat    = atof(row[6].c_str());
            in_spa.lon    = atof(row[5].c_str());

            cout << "in_spa.year is: " << in_spa.year << endl;
            cout << "in_spa.month is: " << in_spa.month << endl;
            cout << "in_spa.day is: " << in_spa.day << endl;
            cout << "in_spa.hour is: " << in_spa.hour << endl;
            cout << "in_spa.minute is: " << in_spa.minute << endl;
            cout << "in_spa.second is: " << in_spa.second << endl;
            cout << "in_spa.lat is: " << in_spa.lat << endl;
            cout << "in_spa.lon is: " << in_spa.lon << endl;

            // Now calculate both zenith and azimuth using the
            // solar position code
            calculate_solar_position(&in_spa, &out_spa);
            // Also see: https://en.wikipedia.org/wiki/Solar_zenith_angle
            double zenith = out_spa.zenith;
            if ((zenith<0) || (zenith>90)){
                cout << "ERROR!! ERROR!! Unexpected value of zenith angle: " << zenith << endl;
                cout << "Quitting..." << endl;
                exit(1);
            }
            double elevation = 90.0 - zenith;
            double azimuth = out_spa.azimuth;
            cout << "Zenith is   : " << zenith << "  " << endl;
            cout << "Elevation is: " << elevation << "  " << endl;
            cout << "Azimuth is  : " << azimuth << "  " << endl;
            currPOC.sun_zenith = zenith;
            currPOC.lat = in_spa.lat;
            currPOC.lon = in_spa.lon;
            // **************** END BLOCK: SOLAR POSITION CODE *********************//

            // There was a codeblock here in the previous versions to get the
            // Site ID. Deleted now.

            // pim: period-in-month. If the date is between 1 and 15
            // (eg, 10th September), it is the first half of the month.
            // Hence, pim=1. If the date is after 15th (eg, 20th July, 31st August),
            // pim=2. For some species, the spectral files to use is different for the
            // first half of some months, and for the second half. Hence, this variable
            // needs to be set.
            currPOC.month = in_spa.month;
            if (in_spa.day <=15){
                currPOC.pim = 1;
            } else {
                currPOC.pim = 2;
            }
            //
            // Set the understory spectrum file from the 'fertilityclass', and other fields
            //
            currPOC.setUnderstorySpectrumFile(atoi(row[4].c_str()), in_spa.lat, isUnderstorySnow, in_spa.month);

            num_strata = atoi((string(row[7])).c_str());
            cout << "Number of strata is : " << num_strata << endl;


            // We store the strata information associated with the sampleplot in
            // memory. When a new line is read in from poc.csv, we first check if it
            // refers to a new sampleplot, or the one already in memory. If it is a new one, we
            // read the lines from the strata.csv file corresponding to this new sampleplot.

            // Its a new one: we need to read in a fresh set of strata...
            if (sp_id != spid_css){

                cout << "sp_id is " << sp_id << " and spid_css is " << spid_css << ". Hence need to read in new set of strata..." << endl;

                // First, go over the current stratum list and 'delete' elements (to free up memory)...
                if (0 != l_css.size()){
                    std::list<Stratum*>::iterator it;
                    for (it = (l_css).begin(); it != (l_css).end(); ++it){
                        delete(*it);
                    }
                }
                l_css.clear();
                if (0 != l_css.size()){ // Something wrong...
                    cout << "ERROR!! ERROR!! SOMETHING WRONG: UNEXPECTED list size!!" << endl;
                    exit(1);
                }
                isStratumListValid = false;
                // Now read in lines from strata file
                for (int sc=1; sc<=num_strata; sc++){
                    if (!(file_st >> row_st)){
                         cout << "ERROR!! Something wrong: unexpected end of strata file!! Exiting..." << endl;
                         exit(1);
                    }
                    if (0 != sp_id.compare(row_st[0])){
                        cout << "ERROR!! Something wrong: unexpected sampleplot id in strata table!! Exiting..." << endl;
                        exit(1);
                    }
                    cout << "Reading in new stratum, stratum number : " << sc << endl;
                     int spCode = atoi(row_st[1].c_str()); // Species code
                     // Stem density. Needs to be converted from t/ha to t/m2
                     float sd  = (atof(row_st[2].c_str()))*0.0001; // Stem density
                     float dia = atof(row_st[3].c_str()); // Diameter (DBH)
                     float ht  = atof(row_st[4].c_str()); // height
                     cout << "SpeciesCode and stemDensity are : " << spCode << ' ' << sd << endl;
                     cout << "Diameter and height are: " << dia << ' ' << ht << endl;

                     // Now, we make a new "Stratum" object to contain all this information.
                     // Then, we add it to the list of strata (l_css).
                     Stratum* st;
                     try {
                            st = new Stratum(spCode, sd, dia, ht, isUnderstorySnow); // create a new stratum object
                     } catch (const stratumNotValidException& e){
                          cout << "The current stratum was not valid. Hence, throwing plotNotValidException..." << endl;
                          throw plotNotValidException();
                     }
                     l_css.push_back(st); // Insert this new stratum object/element into the list
                }
                // If we reached this point, it means that we have gone through all strata (count as per
                // 'num_strata'), and have not thrown any stratumNotValidException exceptions. Hence, the
                // list "l_css"  represents a list of valid set of strata, for the current FFC plot.
                isStratumListValid = true;
                spid_css = sp_id;
            }
            if (isStratumListValid){
                currPOC.listOfStrata = l_css;
            } else {
                throw plotNotValidException();
            }
        }
        catch (const plotNotValidException& e){
            cout << "This POC_Instance is not valid: it generated a plotNotValidException. Hence, moving on to the next one..." << endl;
            cout << endl << endl;
            continue;
        }
        // Now, the 'currPOC' object is complete and valid, and describes a plot observation case (POC)
        // fully. It can be used to create the corresponding FRT config file.

        // frtCfgFileName: The name of the FRT config/input file
        string frtCfgFileName = "FRT_cfg_" + poc_ID + ".txt";
        cout << "The FRT config file name is: " << frtCfgFileName << endl;
        // Make and write out the FRT config (.txt) file
        makeFRTConfigFile(&currPOC, frtCfgFileName, isUnderstorySnow);
        cout << endl << endl;
    }
    cout << endl << endl;
    exit(0);
}





