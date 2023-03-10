#include <iterator>
#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include <string>
#include <list>
#include <iterator>
#include <unordered_set>
#include <cmath>
#include <stdlib.h>     /* for atof */
#include <regex> // For regex_replace, etc

using namespace std;

// The two input csvs (one containing POC info and one, strata info).
string poc_csv_filename = "pocs.csv";
string strata_csv_filename = "strata.csv";

// The location of the template FRT config/section files.
string cfgBeginFile = "sections/frt_begin.txt";
string estFile = "sections/ellipsoid.txt";
string last5Sections = "sections/last5sections.txt";

// These are used to signal (ie, they are thrown) if a valid plot/stratum could
// not be constructed from the given data.
// TODO: This could be inherited from the C++ standard exceptions.
class stratumNotValidException {};
class plotNotValidException {};

// Returns the species group (1, 2 or 3), given the FFC (metsäkeskus) species code.
// The integer values returned means the following:
// 1:  Scots pine or similar (group),
// 2:  Norway Spruce or similar,
// 3:  Birch or similar,
// -1: Error; species group cannot be ascertained (mostly the input mk_code is out of range of 1-30).
//
// mk_code: Metsäkeskus (species) code. Metsäkeskus = FFC (finnish forest center).
int getSpeciesGroup(int mk_code){

    // The 'pine-like' group
    std::unordered_set <int> s1 = {1, 11, 12, 22};
    // The 'spruce-like' group
    std::unordered_set <int> s2 = {2, 8, 10, 14, 16, 19, 23, 30};
    // The 'birch-like' group. Various types of Birch (Rauduskoivu, Hieskoivu & Visakoivu, respectively)
    // are part of this group.
    std::unordered_set <int> s3 = {3, 4, 5, 6, 7, 9, 13, 15, 17, 18, 20, 21, 24, 25, 26, 27, 28, 29};

    // Is mk_code in set s1?
    if (s1.find(mk_code) != s1.end()){
        return(1);
    }
    if (s2.find(mk_code) != s2.end()){
        return(2);
    }
    if (s3.find(mk_code) != s3.end()){
        return(3);
    }
    return (-1);
}

// This class encapsulates all information related to a particular tree stratum.
class Stratum {
    public:
        string treeSpecies; // Either 'Pin_syl', 'Pic_abi' or 'Betula'
        int speciesCode; // the FFC species code (1-30)
        int speciesGroupCode; // see "getSpeciesGroup" above
        float stemDensity, dbh, height;
        float crownradius, crownlength, cbh, slw, bailai, ssc, shl;
        float stump_dia, dlw;
        string br_refl; // bark reflectance spectrum

        // The object constructor
        Stratum(int speciesCode, float sd, float dbh, float height){

        this->speciesCode = speciesCode;
        this->dbh = dbh;
        this->height = height;
        stemDensity = sd;
        // Stump diameter, for calculating dlw
        stump_dia = 2.0 + 1.25*dbh;

        // separate computation for each tree species
        // cbh: crown base height
        int spg = getSpeciesGroup(speciesCode);
        cout << "Species group is: " << spg << endl;
        this->speciesGroupCode = spg;
        if (1==spg){ // Scots Pine (Mänty) group
            treeSpecies = "Pin_syl";
            cbh = -1.453817-0.175251*dbh+0.816958*height;
            crownradius = (0.945031 + 0.152494*dbh)/2;
            slw = 158.0;
            bailai = 0.18;
            ssc = 0.59;
            shl = 0.1;
            br_refl = "bark_pine_Nov2019.txt";
            crownlength = height - cbh;
            cout << "Computed crownlength is: " << crownlength << endl;
            dlw = exp( -1.748 + 14.824*(stump_dia/(stump_dia+4.0))
                       - 12.684*(height/(height+1.0)) + 1.209*log(crownlength)
                       + 0.5*(0.032+0.093) );
            return;
        }
        if (2==spg){ // Norway Spruce (Kuusi)
            treeSpecies = "Pic_abi";
            cbh = -0.029599-0.153803*dbh+0.410031*height;
            crownradius = (1.385994 + 0.134876*dbh)/2;
            slw = 200.0;
            bailai = 0.18;
            ssc = 0.64;
            shl = 0.05;
            br_refl = "bark_spruce_Nov2019.txt";
            crownlength = height - cbh;
            cout << "Computed crownlength is: " << crownlength << endl;
            dlw = exp( -0.085 + 15.222*(stump_dia/(stump_dia+4.0))
                       - 14.446*(height/(height+1.0)) + 1.273*log(crownlength)
                       + 0.5*(0.028+0.087) );
            return;
        }
        if (3==spg){
            treeSpecies = "Betula";
            cbh = -1.319515-0.195148*dbh+0.691154*height;
            crownradius = (1.148961 + 0.197087*dbh)/2;
            crownlength = height - cbh;
            cout << "Computed crownlength is: " << crownlength << endl;
            br_refl = "bark_birch_Nov2019.txt";
            slw = 57.0;
            bailai = 0.15;
            ssc = 1.0;
            shl = 0.4;
            dlw = exp( -29.566 + 33.372*(stump_dia/(stump_dia+2.0)) );
            return;
        }
        // If we have reached this point, it mostly means that the species code was
        // something else than 1-30. We cannot handle this condition,
        // as of now. Hence, throw an exception.
        throw stratumNotValidException();
    }
};

// plot observation case (POC) instance.
class POC_Instance {
    public:
        string poc_ID; // plot observation case (POC) ID.
        // List of pointers to "Stratum" objects.
        // All the strata in the sampleplot are stored via this list.
        list <Stratum*> listOfStrata;
        // Month: 1 is January, 2 is February, ..., 12 is December.
        // pim: Period-in-month. For dates 1 to 15 (both inclusive), pim is 1.
        // For dates 16 to 31, pim is 2.
        int month=-1, pim=-1;
        float lat=-999.99, lon=-999.99;
        string understorySpecFile="NOT_VALID";
        float sun_zenith=-999.99;
        // The view (satellite position) nadir and azimuth angle is assumed to be zero.
        int view_nadir=0;
        int view_azimuth=0;

        // Constructor for this class
        POC_Instance (string pocID){
            this->poc_ID = pocID;
        }

        // Destructor. Empty for now
        ~POC_Instance(){
        }

        // fc: fertility class (1-6).
        void setUnderstorySpectrumFile(int fc, float plot_lat, int month_num){

            if (1==fc || 2==fc){
                this->understorySpecFile = "herb_rich_Nov2019.txt";
            }
            if (3==fc || 4==fc){
                this->understorySpecFile = "mesic_Nov2019.txt";
            }
            if (5==fc || 6==fc){
                this->understorySpecFile = "xeric_Nov2019.txt";
            }
            // If plot is above the arctic circle, and...
            if ( (plot_lat>66.5) && (4==fc || 5==fc || 6==fc) ) {
                this->understorySpecFile = "ForkMoss_Apr2020.txt";
            }
        }
};

// Foliage spectrum:
// Returns the seasonal spectra file for the leaf or shoot.
// This depends on the species group, season and period-in-month
// spg: Species group. That is, 1=pine, 2=spruce, 3=birch.
// For month and periodInMonth, see class POC_Instance documentation.
string getSeasonalSpectraFile(int spg, int month, int periodInMonth){
    if (1==spg){ // Scots Pine (Mänty) group
        if (month<=5){ // Feb, march, april, may
            return("PINSYL_SHOOT_EGS.txt");
        }
        if (month==6){ // June
            return("PINSYL_SHOOT_June.txt");
        }
        if (month>=9){ // Sep, Oct...
            return("PINSYL_SHOOT_Sept.txt");
        }
        return ("shoot_pine_Nov2019.txt");
    }
    if (2==spg){ // Norway Spruce (Kuusi)
        if (month<=5){ // Feb, march, april, may
            return("PICABI_SHOOT_EGS.txt");
        }
        if (month==6){ // June
            return("PICABI_SHOOT_June.txt");
        }
        if (month>=9){ // Sep, Oct...
            return("PICABI_SHOOT_Sept.txt");
        }
        return ("shoot_spruce_Nov2019.txt");
    }
    if (3==spg){ // "Betula"
        if (month<6){ // Jan to May
            return("BETPEN_LEAF_mnth05_pim2.txt");
        }
        if ((6==month) && (1==periodInMonth)){
            return("BETPEN_LEAF_mnth06_pim1.txt");
        }
        if ((6==month) && (2==periodInMonth)){
            return("BETPEN_LEAF_mnth06_pim2.txt");
        }
        if ((8==month) || ((9==month) && (1==periodInMonth))){
            return("BETPEN_LEAF_mnth08_pim2.txt");
        }
        if ( ((2==periodInMonth) && (9==month)) || (month>9)){
            return("BETPEN_LEAF_mnth10_pim1.txt");
        }
        // If none of the above...
        return("leaf_birch_Nov2019.txt");
    }
    cout << "ERROR! Unexpected species group in getSeasonalSpectraFile. Exiting..." << endl;
    exit(1);
}

// Make the stratum section of the FRT config file, corresponding to given stratum (*st).
// sname: stratum name.
// month: month. See documentation for class POC_Instance.
// pim: Period in month. See documentation for class POC_Instance.
string makeStratumSection(Stratum* st, string sname, int month, int pim){

    ifstream t(estFile);
    stringstream buffer;
    buffer << t.rdbuf();
    string s = buffer.str();
    // Replace tokens in txt file with values from the given stratum
    s = std::regex_replace(s, regex("INSERT_STRATUM_NAME"), sname);
    s = std::regex_replace(s, regex("INSERT_SPECIES"), st->treeSpecies);
    s = std::regex_replace(s, regex("INSERT_STEM_DENSITY"), to_string(st->stemDensity));
    s = std::regex_replace(s, regex("INSERT_HEIGHT"), to_string(st->height));
    s = std::regex_replace(s, regex("INSERT_CROWNLENGTH"), to_string(st->crownlength));
    s = std::regex_replace(s, regex("INSERT_CROWNRADIUS"), to_string(st->crownradius));
    s = std::regex_replace(s, regex("INSERT_DBH"), to_string(st->dbh));
    s = std::regex_replace(s, regex("INSERT_DLW"), to_string(st->dlw));
    s = std::regex_replace(s, regex("INSERT_SLW"), to_string(st->slw));
    s = std::regex_replace(s, regex("INSERT_BAILAI"), to_string(st->bailai));
    s = std::regex_replace(s, regex("INSERT_SSC"), to_string(st->ssc));
    s = std::regex_replace(s, regex("INSERT_SHL"), to_string(st->shl));
    s = std::regex_replace(s, regex("INSERT_BARK_SPECTRA_FILE"), st->br_refl);
    s = std::regex_replace(s, regex("INSERT_TRUNK_SPECTRA_FILE"), st->br_refl);

    // leaf seasonal spectra file
    string ssf = getSeasonalSpectraFile(st->speciesGroupCode, month, pim);
    // cout << "getSeasonalSpectraFile_verify SpeciesGroup code, month, pim, spec_file: "<< st->speciesGroupCode<<" "<<month<<" "<<pim<<" "<<ssf<<endl;
    s = std::regex_replace(s, regex("INSERT_LEAF_OR_SHOOT_SPECTRA_FILE"), ssf);
    return(s);
}

// Makes the FRT config file for this POC
void makeFRTConfigFile(POC_Instance* currPOC, string frtCfgFileName){

    string sim_name = "sim_" + currPOC->poc_ID;

    ifstream t1(cfgBeginFile);
    stringstream buffer1;
    buffer1 << t1.rdbuf();
    // fb: FRT beginning sections
    string fb = buffer1.str();
    // Replace tokens in txt file with values from the given stratum
    fb = std::regex_replace(fb, regex("INSERT_SIMULATION_NAME"), sim_name);

    // Now, get the tree class lines
    string treeclass_lines = ""; // Lines for each tree class
    string stratum_section = "\n"; // Lines for stratum section
    string sname; // stratum name
    // Three variable for keeping count. Here, ps is for Pinus sylvestris (scots pine),
    // pa is for Picea abies (norway spruce) & be is for Betula (birch).
    int ps=0, pa=0, be=0;

    std::list<Stratum*>::iterator it;
    for (it = (currPOC->listOfStrata).begin(); it != (currPOC->listOfStrata).end(); ++it){
        int speciesGroupCode = (*it)->speciesGroupCode;
        cout << "Stratum...Species group code is: " << speciesGroupCode << endl;

        if (1==speciesGroupCode){ // Scots Pine (Mänty)
                ps++;
                treeclass_lines += "  treeclass ellipsoid Pin_syl"+to_string(ps)+"\n";
                sname = "Pin_syl"+to_string(ps);
        }
        if (2==speciesGroupCode){ // Norway Spruce (Kuusi)
                pa++;
                treeclass_lines += "  treeclass ellipsoid Pic_abi"+to_string(pa)+"\n";
                sname = "Pic_abi"+to_string(pa);
        }
        if (3==speciesGroupCode){
                be++;
                treeclass_lines += "  treeclass ellipsoid Betula"+to_string(be)+"\n";
                sname = "Betula"+to_string(be);
        }
        stratum_section += makeStratumSection(*it, sname, currPOC->month, currPOC->pim) + "\n\n";
    }
    // This pop_back is to remove the last '\n' character
    treeclass_lines.pop_back();
    stratum_section += "\n";

    cout << "treeclass_lines lines below: " << endl;
    cout << treeclass_lines;
    cout << endl << "treeclass_lines lines done." << endl;

    fb = std::regex_replace(fb, regex("INSERT_MULTIPLE_TREECLASS_LINES"), treeclass_lines);
    fb += "\n";

    ifstream t2(last5Sections);
    stringstream buffer2;
    buffer2 << t2.rdbuf();
    // l5s: Last five sections
    string l5s = buffer2.str();
    // Replace tokens in txt file with values from the given stratum
    l5s = std::regex_replace(l5s, regex("INSERT_GROUND_SPECTRA_FILE"), currPOC->understorySpecFile);
    l5s = std::regex_replace(l5s, regex("INSERT_SUN_ZENITH_ANGLE"), to_string(currPOC->sun_zenith));
    l5s = std::regex_replace(l5s, regex("INSERT_VIEW_NADIR_ANGLE"), to_string(currPOC->view_nadir));
    l5s = std::regex_replace(l5s, regex("INSERT_VIEW_AZIMUTH_ANGLE"), to_string(currPOC->view_azimuth));

    std::ofstream out("cfg_files_output/"+frtCfgFileName);
    out << (fb+stratum_section+l5s);
    out.close();

    return;
}
