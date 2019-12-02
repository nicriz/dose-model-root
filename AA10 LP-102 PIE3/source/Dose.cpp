#include "../headers/Transport.h"
#include "../headers/Source.h"
#include "../headers/Utility.h"
#include "../headers/Output.h"
#include "../headers/OuterSource.h"
#include "../headers/Dispersion.h"
#include "../headers/Dose.h"

#include "TMath.h"
#include <fstream>

/* LIST OF REFERENCES
[1] Assessment of radiological environmental impact at unplanned events at ESS, ESS-0003690
[2] Activity transport and dose calculation models and tools used in safety analyses at ESS, ESS-0092033
[3] Scooping studies on radiological effects due to release at severe accident at ESS,  ESS - 0001894
[4] IAEA - Generic Model for Use in Assessing the Impact of Discharge of Radioactive Substances to the Environment, 2001
[5] Methodology Handbook for Realistic Analysis of Radiological Consequenses, VPC Report T-NA 10-24
[6] DCFPACK [Eckerman & Leggett, 1996]
[7] ICRP 119, 2012, Table H1
*/

Dose::Dose(Dispersion disp):        dispersion(disp),
                                    dry_soil_density(0.),
                                    lumped_translocation(0.),
                                    inhal_rate(0.),
                                    food_consuption(0.),
                                    time_delay(0.),
                                    time_exposure(0.),
                                    names(),
                                    half_lives(),
                                    inhal_coeff(),
                                    ing_coeff(),
                                    cloudshine_coeff(),
                                    groundshine_coeff(),
                                    deposition_vel(),
                                    dose_inhal(),
                                    dose_ing(),
                                    dose_cloudshine(),
                                    dose_groundshine(),
                                    final_dose(0.){

cout << "\n######### DOSE #########\n";

//Parameters with references
emission_time = dispersion.getSTouter().getEmission_time();
dry_soil_density = 260; //kg/m^2 from [4]
lumped_translocation = 0.01; //m^2/kg from [3]
inhal_rate = 0.000256; //m^3/s [5]
food_consuption = 100; //kg/year [1]
time_exposure = 3600*24*365; //s 1 year of exposure [2]
time_delay = 24*3600*365/2; //s , half year [1]

ifstream in;
    in.open("Dose_Coeff");
    double half_life, cs_c, gs_c, inhal_c, ing_c, dep_vel;
    string name;
    int nlines = 0;

    while(in >> name >> half_life >> cs_c >> gs_c >> inhal_c >> ing_c >> dep_vel){
    	
    	names.push_back(name);
        half_lives.push_back(half_life);
        cloudshine_coeff.push_back(cs_c);
        groundshine_coeff.push_back(gs_c);
        inhal_coeff.push_back(inhal_c);
        ing_coeff.push_back(ing_c);
        deposition_vel.push_back(dep_vel);
    	
        nlines++;
    
    }
in.close();

cout << "\n" << nlines << " dose coefficients have been uploaded\n";

//For now nuclides in Outer_Inventory and Dose_coeff must be the same and in the same order!

}

Dose::~Dose(){}

void Dose::Go(){

    vector<double> realeased_activity = dispersion.getActivityRealeased();
    vector<double> relative_conc = dispersion.getRelativeConcentration();
    double total_inhal_dose =0;
    double total_cs_dose =0;
    double total_gs_dose =0;
    double total_ing_dose =0;

    //Dose from inhalation
    cout << "\nDose from inhalation (mSv):\n";
    for(int i=0; i<names.size(); i++){
        
        double _dose_inhal = realeased_activity.at(i)*relative_conc.at(i)*inhal_coeff.at(i)*inhal_rate;
        total_inhal_dose +=_dose_inhal;
        dose_inhal.push_back(_dose_inhal);
        cout << names.at(i) << "\t" << dose_inhal.at(i)*1000 <<"\n";
    }

    cout << "Total:\t" << total_inhal_dose*1000 << "\n";

    //Dose from external cloud
    cout << "\nDose from external exposure to radioactive cloud (mSv):\n";
    for(int i=0; i<names.size(); i++){
        
        double _dose_cs = realeased_activity.at(i)*relative_conc.at(i)*cloudshine_coeff.at(i);
        total_cs_dose +=_dose_cs;
        dose_cloudshine.push_back(_dose_cs);
        cout << names.at(i) << "\t" << dose_cloudshine.at(i)*1000 <<"\n";
    }

    cout << "Total:\t" << total_cs_dose*1000 << "\n";

    //Dose from external ground
    cout << "\nDose from external exposure to radioactive ground (mSv):\n";
    for(int i=0; i<names.size(); i++){
        
        double lambda = TMath::Log(2)/half_lives.at(i);
        double _dose_gs = realeased_activity.at(i)*relative_conc.at(i)*groundshine_coeff.at(i)*deposition_vel.at(i);
        _dose_gs *= (1-TMath::Exp(-time_exposure*lambda))/lambda;
        _dose_gs *= 1./3.; // this factor is to account for 8h of exposure per day instead of 24h
        total_gs_dose +=_dose_gs;
        dose_groundshine.push_back(_dose_gs);
        cout << names.at(i) << "\t" << dose_groundshine.at(i)*1000 <<"\n";
    }

    cout << "Total:\t" << total_gs_dose*1000 << "\n";

    //Dose from ingestion of food crops
    cout << "\nDose from ingestion of food crops (translocation only) (mSv):\n";
    for(int i=0; i<names.size(); i++){
        
        double lambda = TMath::Log(2)/half_lives.at(i);
        double _dose_ing = realeased_activity.at(i)*relative_conc.at(i)*ing_coeff.at(i)*deposition_vel.at(i)*lumped_translocation*food_consuption;
        _dose_ing *= TMath::Exp(-time_exposure*lambda);
        total_ing_dose +=_dose_ing;
        dose_ing.push_back(_dose_ing);
        cout << names.at(i) << "\t" << dose_ing.at(i)*1000 <<"\n";
    }

    cout << "Total:\t" << total_ing_dose*1000 << "\n";

    final_dose = total_inhal_dose + total_cs_dose + total_gs_dose + total_ing_dose;

    cout <<"\nFinal Dose (mSv):\t" << final_dose*1000;

}

