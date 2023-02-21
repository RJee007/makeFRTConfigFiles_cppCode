
// You might need to add the full path to these files.
#include "common.cpp"
#include "csvs.cpp"
#include "spa.cpp"

int main(){

    cout << " Program running, please wait...";
    cout <<  "All output is redirected to log.txt file...";
    cout << " The FRT config (input) files should be created in the cfg_files_output directory...";
    cout.flush();

    // All 'cout' / output messages are redirected to this log file.
    freopen( "log.txt", "w", stdout );

    // Open the POC (plot-observation-case) csv file.
    // POC: Plot observation case. Also known as "observation". This is defined as an event
    // when an observation (Landsat image acquisition) has happened over a given FFC plot.
    // Such an event is associated with a unique combination of the following: 1) An FFC plot,
    // 2) A Landsat image, with associated acquisition date and footprint.
    std::ifstream file(poc_csv_filename);
    CSVRow row;
    file >> row; // read off the first row (header line; can be discarded)
    int num_strata = -1; // Number of strata for the current sampleplot

    // spid_css: sampleplotid associated with the current strata set (css) in
    // memory (explained later)
    string spid_css = "";
    // List of strata associated with spid_css
    list <Stratum*> l_css;
    bool isStratumListValid = false;

    // Open the strata csv file
    std::ifstream file_st(strata_csv_filename);
    CSVRow row_st; // row in strata table
    file_st >> row_st; // read off the first row (header line; can be discarded)

    // Now, iterate over the lines in the poc csv file.
    // Each time, a line will be read off the csv and "row" will be populated with its contents.
    // That is, row[0] will be the element in the first column, row[1] will be the element in the
    // second column, etc.
    while(file >> row){

        string sp_id = row[0]; // The sampleplot ID
        // sp_id is just whitespace characters; we have reached the end of the csv file
        if(std::string::npos == sp_id.find_first_not_of(' '))
        {
            // exit from while loop
            break;
        }

        string poc_ID = row[1]; // The POC ID (L8 aquisition over the given plot).
        // Define a new 'POC_Instance' object to contain all the info that will be read in.
        POC_Instance currPOC(poc_ID);
        // Now, fill this "currPOC" object with relevant information.
        try{
            cout << "Starting new plot acquisition instance, poc_ID is: " << poc_ID << endl;
            cout << "Sampleplotid is: " << sp_id << endl;

            // **************** BEGIN BLOCK: SOLAR POSITION CODE *******************//
            // Also see: spa.cpp.
            //
            // Set input parameters: Date, Time (UTC) and location
            SPAInput in_spa;
            SPAOutput out_spa;
            string dayString = string(row[2]); // the date_L8 column in the poc csv
            in_spa.year = atoi((dayString.substr(0,4)).c_str());
            in_spa.month = atoi((dayString.substr(4,2)).c_str());
            in_spa.day   = atoi((dayString.substr(6,2)).c_str());
            string timeString = string(row[3]); // the time_L8 column in the samplePlots csv
            // Time is in UTC, so no need to convert it.
            in_spa.hour = atoi((timeString.substr(0,1)).c_str());
            in_spa.minute = atoi((timeString.substr(2,2)).c_str());
            in_spa.second = atoi((timeString.substr(5,2)).c_str());
            // Both lat and lon should be positive for us, as we (Finland) are in the North
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
            double zenith = out_spa.zenith;
            if ((zenith<0) || (zenith>90)){
                cout << "ERROR! Unexpected value of zenith angle: " << zenith << endl;
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

            // pim: period-in-month. If the date is between 1 and 15
            // (eg, 10th September), it is the first half of the month.
            // Hence, pim=1. If the date is after 15th (eg, 20th July, 31st August),
            // pim=2. For some species, the spectral files to use is different for the
            // first half of some months, and for the second half. So, we need this variable.
            currPOC.month = in_spa.month;
            if (in_spa.day <=15){
                currPOC.pim = 1;
            } else {
                currPOC.pim = 2;
            }

            // Read in the fertility class (understory type)
            int fertClass = atoi(row[4].c_str());
            if ((fertClass<1) || (fertClass>6)){
                cout << "ERROR! Unexpected value of fertility class: " << fertClass << endl;
                cout << "Quitting..." << endl;
                exit(1);
            }
            cout << "Fertility class is : " << fertClass << endl;
            // Set the understory spectrum file from 'fertClass', and other fields
            currPOC.setUnderstorySpectrumFile(fertClass, in_spa.lat, in_spa.month);

            num_strata = atoi((string(row[7])).c_str());
            cout << "Number of strata is : " << num_strata << endl;

            // We store the strata information associated with the sampleplot in
            // memory. When a new line is read in from poc.csv, we first check if it
            // refers to a new sampleplot, or the one already in memory. If it is a new one, we
            // read the lines from the strata.csv file corresponding to this new sampleplot.
            if (sp_id != spid_css){

                // This POC is associated with a new sampleplot, compared to the one in memory.
                // Hence, we need to read in a fresh set of strata...
                cout << "sp_id is " << sp_id << " and spid_css is " << spid_css << ". Hence need to read in new set of strata..." << endl;

                // First, go over the current stratum list and 'delete' elements (to free up RAM/memory)...
                if (0 != l_css.size()){
                    std::list<Stratum*>::iterator it;
                    for (it = (l_css).begin(); it != (l_css).end(); ++it){
                        delete(*it);
                    }
                }
                l_css.clear();
                if (0 != l_css.size()){ // Should be 0 now...
                    cout << "ERROR! Unexpected list size. Quitting..." << endl;
                    exit(1);
                }
                isStratumListValid = false;
                // Now read in lines from strata csv file
                for (int sc=1; sc<=num_strata; sc++){
                    if (!(file_st >> row_st)){
                         cout << "ERROR! Something wrong; unexpected end of strata file. Exiting..." << endl;
                         exit(1);
                    }
                    if (0 != sp_id.compare(row_st[0])){
                        cout << "ERROR! Something wrong; unexpected sampleplot id in strata table. Exiting..." << endl;
                        exit(1);
                    }
                     cout << "Reading in new stratum, stratum number : " << sc << endl;
                     int spCode = atoi(row_st[1].c_str()); // Species code
                     // Stem density. Needs to be converted from count/ha to count/m2
                     float sd  = (atof(row_st[2].c_str()))*0.0001;
                     float dia = atof(row_st[3].c_str()); // Diameter (DBH)
                     float ht  = atof(row_st[4].c_str()); // height
                     cout << "SpeciesCode and stemDensity are : " << spCode << ' ' << sd << endl;
                     cout << "Diameter and height are: " << dia << ' ' << ht << endl;

                     // Now, we make a new "Stratum" object to contain all this information.
                     // Then, we add it to the list of strata (l_css).
                     Stratum* st;
                     try {
                         st = new Stratum(spCode, sd, dia, ht); // create a new stratum object
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
            } else {
                cout << "Strata information for this POC is already read in." << endl;

            }
            if (isStratumListValid){
                currPOC.listOfStrata = l_css;
            } else {
                throw plotNotValidException();
            }
        }
        catch (const plotNotValidException& e){
            cout << "This POC_Instance is not valid; it generated a plotNotValidException. Hence, moving on to the next one..." << endl;
            cout << endl << endl;
            continue;
        }
        // Now, the 'currPOC' object is complete; it describes a plot observation case (POC)
        // adequately. It can thus be used to create the corresponding FRT config file.

        // frtCfgFileName: The name of the FRT config/input file
        string frtCfgFileName = "FRT_cfg_" + poc_ID + ".txt";
        cout << "The FRT config file name is: " << frtCfgFileName << endl;
        // Make and write out the FRT config (.txt) file
        makeFRTConfigFile(&currPOC, frtCfgFileName);
        cout << endl << endl;

    } // All lines in the poc csv file is done.

    cout << endl << endl;

    exit(0);
}
