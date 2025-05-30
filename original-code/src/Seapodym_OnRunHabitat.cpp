#include "SeapodymCoupled.h"

///This is the main loop function for Habitat simulations. It is based on the default
///OnRunCoupled function, with modifications to compute either spawning of feeding habitats.
///See SeapodymCoupled_OnRunCoupled.cpp for the description of the main function.

void HabitatConsoleOutput(int t_count, int flag_simulation, string date_str, double norm_habitat_input, double norm_spawning_habitat, double like);
void SeapodymCoupled::prerun_model()
{
	OnRunFirstStep();
	ReadHabitat();
}



//////////////////////////////////////////////////////////////////
//--------------------------------------------------------------//
//		     HABITAT SIMULATION				//
//--------------------------------------------------------------//
//////////////////////////////////////////////////////////////////
/*!
\brief The main loop of habitat simulations.
*/
double SeapodymCoupled::OnRunHabitat(dvar_vector x, const bool writeoutputfiles)
{
	t_count = nbt_building+1;
	mat.mats.initialize();

	past_month=month;
	past_qtr=qtr;

	//routine-specific variables
	int tcur = t_count; //will be used for forcing variable time control
	int nbt_no_forecast = t_count + nbt_spinup_tuna + nbt_total - 1;
	int migration_flag = 0;
	int step_count= 0;
	int jday = 0; 
	ivector Nobs(0,nb_fishery-1); Nobs.initialize();

	if (!param->gcalc()){
		
		if (!param->habitat_run_type)
			cout << endl << "Running Spawning Habitat model" << endl << endl; 		
		else 
			cout << endl << "Running Feeding Habitat model for ages: " << param->habitat_run_age << endl << endl; 		
		
		//need to read oxygen in case if month==past_month
		//(otherwise we may not have it for the first time steps)
		if (param->type_oxy==1 && month==past_month)
			ReadClimatologyOxy(1, month);
		//need to read oxygen in case if qtr==past_qtr 
		if (param->type_oxy==2 && qtr==past_qtr)
			ReadClimatologyOxy(1, qtr);
	}
	dvariable likelihood = 0.0;
	double larvaelike = 0.0;
	reset(x);
	//----------------------------------------------//
	// 	LOCAL MATRICES ALLOCATION SECTION       //
	//----------------------------------------------//	
	
	dvar_matrix Habitat;

	Habitat.allocate(map.imin, map.imax, map.jinf, map.jsup);  
	Habitat.initialize();

	// Aggregated habitat over the entire period, only at obs. locations
	dvar_matrix Habitat_pred_at_obs;
	IVECTOR kinf;
	IVECTOR ksup;
	int nb_larvae_input_agg_groups = param->nb_larvae_input_agg_groups;
	int ntime_agg[nb_larvae_input_agg_groups];

	for (int i = 0; i < nb_larvae_input_agg_groups; ++i) {
		ntime_agg[i] = 0;
	}
	if (param->larvae_input_aggregated_flag[0]){
		// Aggregated spawning habitat over the entire period, only at obs. locations
		kinf.allocate(0, nb_larvae_input_agg_groups-1);
		ksup.allocate(0, nb_larvae_input_agg_groups-1);
		for (int k = 0; k < nb_larvae_input_agg_groups; k++){
			kinf[k] = 0;
			ksup[k] = mat.aggregated_larvae_input_vectors[k].size();
		}
		Habitat_pred_at_obs.allocate(0, nb_larvae_input_agg_groups-1, kinf, ksup);
		Habitat_pred_at_obs.initialize();	
	}


	//precompute thermal habitat parameters
	for (int sp=0; sp < nb_species; sp++)
		func.Vars_at_age_precomp(*param,sp);

	//precompute seasonal switch function
	for (int sp=0; sp < nb_species; sp++){
		if (param->seasonal_migrations[sp]){
			func.Seasonal_switch_year_precomp(*param,mat,map,
						value(param->dvarsSpawning_season_peak[sp]),
						value(param->dvarsSpawning_season_start[sp]),sp);
		}
	}

	//DYM file names
	string fileout;
	if (param->habitat_run_type>0)
               	fileout = param->strdir_output + param->sp_name[0] + "_feeding_habitat_output.dym";
	else			
               	fileout = param->strdir_output + param->sp_name[0] + "_spawning_habitat_output.dym";
	
	//Write DYM headers
	if (writeoutputfiles){
		WriteFileHeaders_submodel(fileout,false);		
		if (!param->gcalc())
			HabitatConsoleOutput(0,0,date_str,0,0,0);
	}
	/////////////////////////////////////////////////////////////////////
	/////////////////////////////////////////////////////////////////////
	////------------------------------------------------------------/////
	////								/////
	////		||| START OF SIMULATION CYCLE |||		/////
	////								/////
	////------------------------------------------------------------/////
	/////////////////////////////////////////////////////////////////////
	/////////////////////////////////////////////////////////////////////

	for (;t_count <= nbt_total; t_count++)
	{
		//----------------------------------------------//
		//              INITIALISATION                  //
		//----------------------------------------------//
		sumP=0;
		sumFprime.initialize();
		sumF.initialize();
		mat.dvarCatch_est.initialize();
		//----------------------------------------------//
		//		       DATE			//
		//----------------------------------------------//
		getDate(jday);
		for (int sp=0; sp < nb_species; sp++){
			func.Seasonal_switch(*param,mat,map,jday,sp);	
		}
		tcur = t_count;
		if (!param->gcalc()){
			tcur = 1; 
			//----------------------------------------------//
			//	DATA READING SECTION: U,V,T,O2,PP	//
			//----------------------------------------------//
			//if ((t_count > nbt_building)) { //CC run with average effort forecast
			if ((t_count > nbt_building) && (t_count <= nbt_no_forecast)) {
				//TIME SERIES 
				t_series = t_count - nbt_building + nbt_start_series;
				ReadTimeSeriesData(tcur,t_series);	
	
			}
			else if (((t_count <= nbt_building) && (month != past_month)) || (t_count > nbt_no_forecast)) {
	
				//AVERAGED CLIMATOLOGY DATA
				ReadClimatologyData(tcur, month);
			}
			if (param->type_oxy==1 && month != past_month) {
				//MONTHLY O2
				ReadClimatologyOxy(tcur, month);
			}
			if (param->type_oxy==2 && qtr != past_qtr) {
				//QUARTERLY O2
				ReadClimatologyOxy(tcur, qtr);
			}
		}
		//spin-up flag: to be removed later
		int pop_built = 0; 
		if (t_count > nbt_building) 
			pop_built = 1;
		//------------------------------------------------------------------------------//
		//	TRANSPORT OF TUNA AGE CLASSES AND PREDICTED CATCH COMPUTATION		//
		//------------------------------------------------------------------------------//
		//------------------------------------------------------------------------------//

		for (int sp=0; sp < nb_species; sp++){

			if (param->habitat_run_type>0){
			//1. Precompute some variables outside of age loop

			//1.0 Forage scaling
			func.Forage_Scaling(*param, mat, map, sp, tcur);
			//1.1 Accessibility by adults (all cohorts)
				ivector tags_age_habitat;
				tags_age_habitat.allocate(0,aN_adult(0));
				tags_age_habitat.initialize();
				func.Faccessibility(*param, mat, map, sp, jday, tcur, pop_built, 0, tags_age_habitat);//checked
			}

			//2. Starting habitat computation (implicit age loop) 
			int age = 0;	
			if (!param->habitat_run_type){
				//2.1 Spawning habitat	
				func.Spawning_Habitat(*param, mat, map, Habitat, 1.0, sp, tcur, jday);

				//2.2 Aggregate spawning habitat on a vector at obs. locations
				if (param->larvae_input_aggregated_flag[0]){
					int iAgg = Utilities::iTimeOfYear(month, param->larvae_input_aggregation);
					for (auto k=0u; k<mat.aggregated_larvae_input_vectors[iAgg].size(); k++){
						Habitat_pred_at_obs(iAgg, k) += Habitat(mat.aggregated_larvae_input_vectors_i[iAgg][k], mat.aggregated_larvae_input_vectors_j[iAgg][k]);
					}
					ntime_agg[iAgg] += 1;
				}
			}

			if (param->habitat_run_type>0){
				int rr_x = (int)nlon/nlon_input;
				int rr_y = (int)nlat/nlat_input;
				if (rr_x<1 || rr_y<1){ 
					cerr << "Model resolution should be divisible without remainder by the resolution of input density field." << 
					         endl<< "Currenly nlon/nlon_input = " << rr_x << ", and nlat/nlat_input = " << rr_y << endl << "Will exit now...";
					exit(1);
				}
			
				for (int ages=0; ages<param->nb_habitat_run_age; ages++){
					migration_flag = 0;
					if (age>=param->age_mature[sp] && param->seasonal_migrations[sp]) migration_flag = 1;

					int a;
					for (a=1; a<param->sp_nb_cohorts[sp]-1; a++){
						if (mean_age_cohort(sp,a)>param->habitat_run_age[ages]){
							age = mean_age_cohort(sp,a);
							break;
						}
					}
					if (a==param->sp_nb_cohorts[sp]-1 && age==0) age = param->sp_nb_cohorts[sp]-1;

					if (param->age_compute_habitat[sp][age]!=param->age_compute_habitat[sp][age-1]) {
						func.Feeding_Habitat(*param,mat,map,Habitat,sp,age,jday,tcur,migration_flag);
					}
					//------------------------------------------------------//
					//	     LIKELIHOOD FOR FEEDING HABITAT		//
					//------------------------------------------------------//
					for (int i=1; i<nlon_input; i++){
						for (int j=1; j<nlat_input; j++){
							double H_obs  = mat.habitat_input(ages,t_count,i,j);
							int ncount = 0;
							if (H_obs>0){
								dvariable Hmean_pred = 0.0;
								for (int ii=0; ii<rr_x; ii++)
								for (int jj=0; jj<rr_y; jj++){
									if (map.carte[rr_x*i+ii][rr_y*j+jj]){
										Hmean_pred += Habitat(rr_x*i+ii,rr_y*j+jj);
										ncount ++;
									}
								}
								if (ncount)
							       		Hmean_pred /= ncount;
								likelihood += (H_obs-Hmean_pred)*(H_obs-Hmean_pred);
							}
						}
					}			
				}
			}	
		}//end of 'sp' loop

		//------------------------------------------------------//
		//	     LIKELIHOOD FOR SPAWNING HABITAT		//
		//------------------------------------------------------//
		if (!param->habitat_run_type){

			int rr_x = (int)nlon/nlon_input;
			int rr_y = (int)nlat/nlat_input;
			if (rr_x<1 || rr_y<1){ 
				cerr << "Model resolution should be divisible without remainder by the resolution of input density field." << 
				         endl<< "Currenly nlon/nlon_input = " << rr_x << ", and nlat/nlat_input = " << rr_y << endl << "Will exit now...";
				exit(1);
			}

			if (param->fit_spawning_habitat_raw){
				for (int i=1; i<nlon_input; i++){
					for (int j=1; j<nlat_input; j++){
						double H_obs = mat.habitat_input[0][t_count][i][j];
						if (H_obs>0){
							int ncount = 0;
							dvariable Hmean_pred = 0.0;
							for (int ii=0; ii<rr_x; ii++){
								for (int jj=0; jj<rr_y; jj++){
									if (map.carte[rr_x*i+ii][rr_y*j+jj]){
										Hmean_pred += Habitat(rr_x*i+ii,rr_y*j+jj);
										ncount ++;
									}
								}
							}
							if (ncount)
								Hmean_pred /= ncount;
							//if (Hmean_pred>0)
							likelihood += (H_obs-Hmean_pred)*(H_obs-Hmean_pred);
						}
					}
				}		
			}else{
				if (!param->larvae_input_aggregated_flag[0]){
					larvaelike += get_larvae_like(likelihood, Habitat, mat.habitat_input[0], t_count-nbt_building);
				}
			}
		}

		if (writeoutputfiles){
			if (!param->gcalc())
				HabitatConsoleOutput(t_count,1,date_str,norm(mat.habitat_input(0,t_count)),norm(value(Habitat)),value(likelihood));

			//write model habitat into a DYM file
 			dmatrix mat2d(0, nbi - 1, 0, nbj - 1);
			mat2d.initialize();
			for (int i=map.imin; i <= map.imax; i++){
					for (int j=map.jinf[i] ; j<=map.jsup[i] ; j++){
							if (map.carte[i][j]){
									//mat2d(i-1,j-1) = mat.habitat_input(0,t_count,i,j);
									mat2d(i-1,j-1) = value(Habitat(i,j));//if multiple, always the last habitat is written!
							}
					}
			}
			double minval=0.0;
			double maxval=1.0;

            rw.wbin_transpomat2d(fileout, mat2d, nbi-2, nbj-2, true);
			//update min-max values in header
			rw.rwbin_minmax(fileout, minval, maxval);

		}
		past_month=month;
		step_count++;
		if (qtr != past_qtr) past_qtr = qtr; 

	} // end of simulation loop

	// Compute larvae likelihood on aggregated habitat
	if (!param->fit_spawning_habitat_raw && param->larvae_input_aggregated_flag[0]){
		// Compute the average larvae habitat over the entire period
		for (int iAgg=0; iAgg<nb_larvae_input_agg_groups; iAgg++){
			for (auto k=0u; k<mat.aggregated_larvae_input_vectors[iAgg].size(); k++){
				Habitat_pred_at_obs(iAgg, k) /= ntime_agg[iAgg];
			}
		}
		larvaelike += get_larvae_like(likelihood, Habitat_pred_at_obs);
	}

	param->total_like = value(likelihood);
	if (!param->scalc())
		cout << "end of forward run, likelihood: " << defaultfloat << value(likelihood) << endl;

	return value(likelihood);
}

void SeapodymCoupled::ReadHabitat()
{
	cout << "Reading input habitat file... " << endl;
	int jday = 0;
	int t_count_init = t_count;
	//mat.createMatHabitat_input(map,param->nb_habitat_run_age,nbt_total);
	int nlevel = 0;
	string file_input;
	if (!param->habitat_run_type){
		if (param->fit_spawning_habitat_raw){
			file_input = param->strdir_output + param->sp_name[0] + "_spawning_habitat_input.dym";
		}else{
			file_input = param->strfile_larvae;
		}
	}else{
		file_input = param->strdir_output + param->sp_name[0] + "_feeding_habitat_input_age1.dym";
	}

    rw.rbin_headpar(file_input, nlon_input, nlat_input, nlevel);

	int nb_larvae_input_agg_groups = param->nb_larvae_input_agg_groups;
	if (nlevel != nb_larvae_input_agg_groups && param->larvae_input_aggregated_flag[0]){
		cerr << "Error[" << __FILE__ << ':' << __LINE__ << "]: The number of nlevels in \"" << file_input << " does not match the number of groups from <larvae_input_aggregation_imonths> in the parameter file." << endl;
		exit(1);
	}

	if (!param->fit_spawning_habitat_raw){
		if (nlon!=nlon_input || nlat != nlat_input){
			cerr << "Error[" << __FILE__ << ':' << __LINE__ << "]: Spatial resolution of \"" << file_input << " does not match the model resolution. Coarser resolution of <file_larvae_data> habitat input is not implemented yet. Use either <fit_spawning_habitat_raw> = \"1\" or <file_larvae_data> that match model resolution." << endl;
			exit(1);
		}
	}


	//equivalent to mat.createMatHabitat_input:
	mat.habitat_input.allocate(0,param->nb_habitat_run_age-1);
    for (int n=0; n<param->nb_habitat_run_age; n++){
		mat.habitat_input[n].allocate(1,nbt_total);
	    for (int t=1; t<=nbt_total; t++){
			mat.habitat_input[n][t].allocate(1, nlon_input, 1, nlat_input);
			mat.habitat_input[n][t].initialize();
		}
	}
	
	int marker_iAggs = 0; vector<int> index_habitat_input;// in order to save the t_count indices corresponding to each of the larvae input aggregation group
	for (; t_count<=nbt_total; t_count++){
		getDate(jday);
		if (t_count > nbt_building) {
			//TIME SERIES 
			t_series = t_count - nbt_building + nbt_start_series;

			//----------------------------------------------//
			//	READING HABITAT DATA			//
			//----------------------------------------------//

			//will need to prepare the input habitat that contains the data only for the selected time period
			int nbytetoskip = (9 +(3* nlat_input * nlon_input) + nlevel + ((nlat_input *nlon_input)* (t_count-1))) * 4;
			if (!param->habitat_run_type){
				//rw.rbin_input2d(file_input, map, mat.habitat_input[0][t_count], nlon_input+2, nlat_input+2, nbytetoskip);
				ifstream litbin(file_input.c_str(), ios::binary | ios::in);
				const int sizeofDymInputType = sizeof(float);
				float buf;
				if (!litbin){
					cerr << "Error[" << __FILE__ << ':' << __LINE__ << "]: Unable to read file \"" << file_input << "\"\n";
					exit(1);
				}
				if (!param->larvae_input_aggregated_flag[0]){
					litbin.seekg(nbytetoskip, ios::cur);
				}else{
					int iAgg = Utilities::iTimeOfYear(month, param->larvae_input_aggregation);

					if (iAgg == marker_iAggs && marker_iAggs < nb_larvae_input_agg_groups){
						index_habitat_input.push_back(t_count);
						marker_iAggs+=1;
					}

					int nbytetoskip_aggregatedfile = (9 +(3* nlat * nlon) + nb_larvae_input_agg_groups + (nlat *nlon*iAgg)) * 4;
					litbin.seekg(nbytetoskip_aggregatedfile, ios::cur);
				}
				for (int j=0;j<nlat_input;j++){
					for (int i=0;i<nlon_input;i++){
						litbin.read(( char *)&buf,sizeofDymInputType);
						double val = buf;
						if (val<=-99)
						    val = 0;

						mat.habitat_input[0][t_count][i+1][j+1]= val;
					}
				}
				litbin.close();
				
			} else {
				for (int n=0; n<param->nb_habitat_run_age; n++){
					std::ostringstream ostr;
               		ostr << n+1;
					file_input = param->strdir_output + param->sp_name[0] + 
						"_feeding_habitat_input_age" + ostr.str() + ".dym";
					//rw.rbin_input2d(file_input, map, mat.habitat_input[n][t_count], nbi, nbj, nbytetoskip);
					ifstream litbin(file_input.c_str(), ios::binary | ios::in);
					if (!litbin){
						cerr << "Error[" << __FILE__ << ':' << __LINE__ << "]: Unable to read file \"" << file_input << "\"\n";
						exit(1);
					}
					litbin.seekg(nbytetoskip, ios::cur);
					const int sizeofDymInputType = sizeof(float);
		            float buf;
		            for (int j=0;j<nlat_input;j++){
						for (int i=0;i<nlon_input;i++){
							litbin.read(( char *)&buf,sizeofDymInputType);
							mat.habitat_input[n][t_count][i+1][j+1]= buf;
						}
					}
					litbin.close();
				}
			}
		}
	}
	t_count = t_count_init;
	if (!param->fit_spawning_habitat_raw && param->larvae_input_aggregated_flag[0]){
		// Vector of non-NA observed density
		for (int iAgg=0; iAgg<nb_larvae_input_agg_groups; iAgg++){
			if (iAgg <= (int) index_habitat_input.size()){
				int index = index_habitat_input[iAgg];
				for (int j=0;j<nlat_input;j++){
					for (int i=0;i<nlon_input;i++){
						if (map.carte[i+1][j+1]){
							if (mat.habitat_input[0][index][i+1][j+1]>=0){
								mat.aggregated_larvae_input_vectors[iAgg].push_back(mat.habitat_input[0][index][i+1][j+1]);
								mat.aggregated_larvae_input_vectors_i[iAgg].push_back(i+1);
								mat.aggregated_larvae_input_vectors_j[iAgg].push_back(j+1);
							}
						}
					}
				}
			}
		}
	}

}


void HabitatConsoleOutput(int t_count, int flag_simulation, string date_str, double norm_habitat_input, double norm_Habitat, double like)
{
        if (flag_simulation){
                cout << setw(4)  << left << t_count<<"| "
                     << setw(14) << date_str<<"| "
                     << setw(14) << norm_habitat_input <<" | "
                     << setw(14) << norm_Habitat<<" | "
                     << like << endl;
        }
        else{
                //CalcSums();
                // Comment: Larvae biomass is the result of spawning function S(t-1) and ADRE at time t
                // we have Larvae(t=1)=0 because S(t-1) is computed after CalcSums()
                cout << endl << "Model temporal dynamics:" << endl;
                cout << setw(4)  << left << "step"<<"| "
                     << setw(14) << "Date"<<"| "
                     << setw(14) << "Input Habitat"<<" | "
                     << setw(14) << "Model_Habitat"<<" | "
                     << setw(14) << "Likelihood"<< endl;

                cout << "----------------------------------------------------------------------------------------------------------"<< endl;
	}
}

