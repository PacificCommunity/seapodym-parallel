#include "SeapodymCoupled.h"

void SeapodymCoupled::prerun_model()
{
	OnRunFirstStep();
}

///This is the main loop function including the calculation of biomass exchange
///between regions, based on the one time step simulations with non-zero biomass
///only in the donor region and quantification of biomass changes in all.
///See SeapodymCoupled_OnRunCoupled.cpp for the description of the main loop


//////////////////////////////////////////////////////////////////
//--------------------------------------------------------------//
//		     FORAGE-TUNA SIMULATION			//
//--------------------------------------------------------------//
//////////////////////////////////////////////////////////////////
/*!
\brief The tuna population main loop is in this function.
*/
double SeapodymCoupled::OnRunCoupled(dvar_vector x, const bool writeoutputfiles)
{
	InitializeAll();
	SUM_CATCH = 0.0;

	past_month=month;
	past_qtr=qtr;

	//Temporarily reading the tau of the first cohort
	//Need to be just a single number for all cohorts
	int dtau = param->sp_unit_cohort[0][1];
	int nbt_before_first_recruitment = Date::get_nbt_before_first_recruitment(
			param->first_recruitment_date,
			param->ndatini,param->deltaT,param->date_mode); 	
	//counter of number of time steps between recruitments (survival equations)
	//once nt_dtau = dtau, recruitment occurs and nt_dtau=0
	//its initial value is dtau-nbt_before_first_recruitment_date
	int nt_dtau = dtau-nbt_before_first_recruitment; 

	//routine-specific variables
	int tcur = t_count; //will be used for forcing variable time control
	int nbt_no_forecast = t_count + nbt_spinup_tuna + nbt_total - 1;
	bool fishing = false;
	int migration_flag = 0;
	int step_count= 0;
	int step_fishery_count= 0;
	int jday = 0; 
	int nbstoskip = param->nbsteptoskip; // nb of time step to skip before computing likelihood

	if (!param->gcalc()){
		//need to read oxygen in case if month==past_month
		//(otherwise we may not have it for the first time steps)
		if (param->type_oxy==1 && month==past_month)
			ReadClimatologyOxy(1, month);
		//need to read oxygen in case if qtr==past_qtr 
		if (param->type_oxy==2 && qtr==past_qtr)
			ReadClimatologyOxy(1, qtr);
	}

	//----------------------------------------------//
	// 	LIKELIHOOD INITIALISATION SECTION       //
	//----------------------------------------------//	
	lflike = 0.0;
	double taglike = 0;
	double stocklike = 0.0;
	double larvaelike = 0.0;
	dvariable likelihood = 0.0;
	dvariable total_stock = 0.0;
	//Reset model parameters:
	reset(x);

	//----------------------------------------------//
	// 	LOCAL MATRICES ALLOCATION SECTION       //
	//----------------------------------------------//	
	dvar_matrix Spawning_Habitat;
	dvar_matrix Total_pop;
	dvar_matrix Habitat; 
	dvar_matrix IFR; 
	dvar_matrix ISR_denom; 
	dvar_matrix FR_pop;
	dvar_matrix Mortality; 

	Habitat.allocate(map.imin1, map.imax1, map.jinf1, map.jsup1);
	Mortality.allocate(map.imin, map.imax, map.jinf, map.jsup);
	Spawning_Habitat.allocate(map.imin, map.imax, map.jinf, map.jsup);
	Total_pop.allocate(map.imin, map.imax, map.jinf, map.jsup);

	if (param->food_requirement_in_mortality(0)){ 
		//temporal, need to check memory use first 
		IFR.allocate(map.imin, map.imax, map.jinf, map.jsup);
		ISR_denom.allocate(map.imin, map.imax, map.jinf, map.jsup);
		FR_pop.allocate(map.imin, map.imax, map.jinf, map.jsup);
		IFR.initialize();
		ISR_denom.initialize();
		FR_pop.initialize();
	}
	Spawning_Habitat.initialize();
	Habitat.initialize();
	Mortality.initialize();

	// For larvae likelihood
	if (param->larvae_like[0])
		create_init_larvae_vars();		

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

	//TAG vars initialization section
	tags_age_habitat.initialize();
	if (nb_tagpops>0){
		tagpop_age_solve.initialize();
		rec_pred.initialize();
	}
	if (writeoutputfiles){
		InitFileFluxes();
		if (!param->gcalc())
			ConsoleOutput(0,0);
	}
	//Simulation FLAGS
	bool tags_only = param->tags_only;
	int spop0 = 0;
	int mortality_off = 0;
	if (tags_only) {
		spop0 = 1;
		nb_fishery = 0;
		fishing = false;
		param->stock_like.initialize();
		param->frq_like.initialize();
	}

	//------------------------------------------------------//
	//	   COMPUTE FLUXES BETWEEN REGIONS		//
	//------------------------------------------------------//
	if (!param->nb_region && !param->nb_EEZ){
		cerr << "ERROR: In FLUXES mode the (MFCL) regional structure MUST be defined and activated. Revise the parfile, will stop now!" << endl;
		exit(1);
	}
	//Will consider that LF regional structure has always > 20 regions, while MFCL cannot have more than 20 regions:
	if (!param->nb_EEZ && param->nb_region>20){
		cerr << "ERROR: The LF regional structure is ON, desactivate LF likelihood to compute fluxes, will stop now!" << endl;
		exit(1);
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
		if (param->tag_like[0]){
			get_tag_releases(mat.dvarDensity,tagpop_age_solve,nb_rel);
			tags_age_habitat.initialize();
			for (int spop=1; spop<nb_tagpops+1; spop++){
				for (int a=a0_adult(0); a<aN_adult(0); a++){
					//first initialize with the ages, which will be resolved
					tags_age_habitat[a] += tagpop_age_solve(spop-1,t_count-1,a);
				}
			}
			//second correct by the age_compute_habitat indices
			for (int a=a0_adult(0); a<aN_adult(0); a++){
				if (tags_age_habitat[a]){
					int aa = a;
					for (; aa>=a0_adult(0); aa--){
						if (param->age_compute_habitat(0,aa)!=param->age_compute_habitat(0,aa-1))
							break;
					}
					if (!tags_age_habitat[aa]) tags_age_habitat[aa] = -1;
				}
			}
		}
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
		
		//----------------------------------------------//
		//	READING CATCH DATA: C_obs, effort	//
		//----------------------------------------------//
		if (!tags_only && (nb_fishery > 0) && sum(param->mask_fishery_sp) && ( t_count > nbt_building) 
				&& (year>=(int)param->save_first_yr) && (t_count <= nbt_no_forecast)){
			mat.catch_est.initialize();
			rw.get_fishery_data(*param, mat.effort, mat.catch_obs, mat.efflon, mat.efflat, year, month);
			fishing = true;
			if (qtr != past_qtr){
				mat.C_N_sp_age_fishery.initialize();
				mat.dvarLF_est.initialize();
				if (t_count > nbt_building+nbstoskip){
					rw.get_LF_qtr_data(*param, mat.LF_qtr_obs, year, qtr-1);
				}
			}
		}
		//MPAs are no longer supported in the code. Prepare the fishery data as designed and use as input data
                if (param->mpa_simulation){
                        rw.get_fishery_data_mpa(map,*param, mat.effort, mat.catch_obs, mat.efflon, mat.efflat, year, month);
			
                }

		if (t_count > nbt_no_forecast) {
			rw.get_average_effort(*param, mat.effort, mat.efflon, mat.efflat, 10, month);
			//to get actual fishing data if it exists
			//rw.get_fishery_data(*param, mat.effort, mat.catch_obs, year, month);
			mat.catch_obs.initialize();
			mat.catch_est.initialize();
			mat.C_N_sp_age_fishery.initialize();
			mat.dvarLF_est.initialize();
		}
		//----------------------------------------------//
		//	 SPIN-UP PARAMETERS (To be removed)	//
		//----------------------------------------------//
		int pop_built = 0; 
		if (t_count > nbt_building) 
			pop_built = 1;
		//------------------------------------------------------------------------------//
		//	TRANSPORT OF TUNA AGE CLASSES AND PREDICTED CATCH COMPUTATION		//
		//------------------------------------------------------------------------------//
		//------------------------------------------------------------------------------//

		for (int sp=0; sp < nb_species; sp++){
		
			int elarvae_model = param->elarvae_model[sp];
			//if !elarvae_model, elarvae_dt = 0 as elarvae_age = 0
			elarvae_dt = param->elarvae_age[sp]/deltaT; 
			double sigma_fcte_save = param->sigma_fcte;

			//Precompute diagonal coefficients for larvae and juvenile ADREs
			if (!elarvae_model){
				mat.u = mat.un[tcur][0]; mat.v = mat.vn[tcur][0]; 
				pop.precaldia(*param, map, mat);
				pop.caldia(map, *param, mat.diffusion_x, mat.advection_x, mat.diffusion_y, mat.advection_y);
			}

			//store fish density before transport
			for (int a=a0_adult(sp); a<aN_adult(sp); a++)
				mat.density_before(sp,tcur,a) = value(mat.dvarDensity(sp,a));

			//1. Precompute some variables outside of age loop
			
			//1.0 Forage scaling
			func.Forage_Scaling(*param, mat, map, sp, tcur);

			//1.1 Accessibility by adults (all cohorts)
 			func.Faccessibility(*param, mat, map, sp, jday, tcur, pop_built, tags_only, tags_age_habitat);//checked

			//1.2 Food Requirement by population
			if (param->food_requirement_in_mortality(sp)){
				FR_pop_comp(FR_pop, sp);
			//	ISR_denom_comp(ISR_denom, sp, tcur);
			}
			//1.3 precompute local proportions of catch by fishery
			if (fishing && param->fisheries_no_effort_exist[sp]){
				pop.Ctot_proportion_fishery_comp(map,*param,mat,rw,year,month,sp);
			}
			//1.4 precompute selectivity functions
			if (fishing)
				pop.Selectivity_comp(*param,nb_fishery,a0_adult(sp),aN_adult(sp),sp);

			//1.6 Precompute Mortality range at age function
			func.mortality_range_age_comp(*param,mat,sp);

			//2. IMPLICIT AGE LOOP: increment age while moving through life stages 
			int age = 0;	

			//2.0 If activated, the Early Larvae Model (ELM) solved over elarvae_age
			if (elarvae_model){
				//Do the time-splitting for the first age class: 
				//1- early (just a few days after yolk stage) and 
				//2- late (up to one month of age) larvae
		
				bool time_getpred = false;
				if (year>=param->larvae_like_firstyear
						&& year<=param->larvae_like_lastyear 
						&& t_count > nbt_building+nbstoskip)
					time_getpred = true;

				elarvae_model_run(Mortality,sp,tcur,time_getpred,writeoutputfiles);
			}

			//2.1 ADRE for late larvae in ELM or monthly larval class in default model 
			//2.1.1 Spawning habitat 	
			if (!tags_only)
				func.Spawning_Habitat(*param, mat, map, Spawning_Habitat, 1.0, sp, tcur, jday);

			//2.1.2 Transport and mortality of larvae (always one age class)	
			for (int n=0; n<param->sp_nb_cohort_lv[sp]; n++){
				double mean_age = mean_age_cohort[sp][age]; 

				if (!tags_only){
					func.Mortality_Sp(*param, mat, map, Mortality, Spawning_Habitat, sp, mean_age, age, tcur);
					//Compute fluxes here
					FluxesComp(mat.dvarDensity(sp,age), Habitat, Mortality, age, fishing, year, month, jday, step_fishery_count,tcur,1-elarvae_dt);
				
					pop.Precalrec_juv(map, mat, Mortality, tcur,(1-elarvae_dt));//checked
					pop.Calrec_juv(map, mat, mat.dvarDensity[sp][age], Mortality, tcur,(1-elarvae_dt));//checked
				}
				age++;
			}

			//2.2.0 Only in the ELM mode need to reset movement rates for juveniles
			if (elarvae_model){
				param->sigma_fcte = sigma_fcte_save;
				mat.u = mat.un[tcur][0]; mat.v = mat.vn[tcur][0]; 
				//Precompute diagonal coefficients for juvenile ADREs
				pop.precaldia(*param, map, mat);
				pop.caldia(map, *param, mat.diffusion_x, mat.advection_x, mat.diffusion_y, mat.advection_y);
			}
			
			//2.2.1 Juvenile habitat	
			if (!tags_only){
				if (param->cannibalism[sp]){
					Total_Pop_comp(Total_pop,sp,jday,tcur); //adjoint
					func.Juvenile_Habitat_cannibalism(*param, mat, map, Habitat, Total_pop, sp, tcur);
				} else 
					func.Juvenile_Habitat(*param, mat, map, Habitat, sp, tcur);
			}

			//2.2.2 Transport and mortality of juvenile age classes	
			for (int n=0; n<param->sp_nb_cohort_jv[sp]; n++){			
				double mean_age = mean_age_cohort[sp][age];
				if (!tags_only){
					func.Mortality_Sp(*param, mat, map, Mortality, Habitat, sp, mean_age, age, tcur);
					//Compute fluxes here
					FluxesComp(mat.dvarDensity(sp,age), Habitat, Mortality, age, fishing, year, month, jday, step_fishery_count,tcur,1);
					pop.Precalrec_juv(map,  mat, Mortality, tcur,1);
					pop.Calrec_juv(map, mat, mat.dvarDensity[sp][age], Mortality, tcur,1);
				}
				age++;
			}
	
			//----------------------------------------------//
			//	Fishing with effort data available	//
			//----------------------------------------------//
			//3. Compute predicted catch before transport & mortality	
			if (fishing){
				int k = 0;
				for(int f=0; f<nb_fishery; f++){
					if (param->mask_fishery_sp[sp][f]){
						//In Optimization Mode compute predicted (E-based) catch and LF only for
						//fisheries designed to inform parameters (flag mask_fishery_sp_like).  
						//In Simulation Mode - for all fisheries with available effort data.
						if (param->gcalc()){
							if (param->mask_fishery_sp_like[sp][f] && !param->mask_fishery_sp_no_effort[sp][f])
								pop.Predicted_Catch_Fishery(map,*param,mat,rw,sp,f,k,year,month,tcur,step_fishery_count);
						} else {
							if (!param->mask_fishery_sp_no_effort[sp][f])
								pop.Predicted_Catch_Fishery(map,*param,mat,rw,sp,f,k,year,month,tcur,step_fishery_count);
						}			
						k++;
					}
				}
				step_fishery_count++;
			}
			if (fishing && param->fisheries_no_effort_exist(sp))
				pop.Total_exploited_biomass_comp(map,*param,mat,sp,tcur);

			//4. Transport and mortality of adult cohort
			for (int n=0; n<param->sp_nb_cohort_ad[sp]; n++){

				if (fishing && param->fisheries_no_effort_exist(sp))
					pop.Total_obs_catch_age_comp(map,*param,mat,rw,age,sp,year,month,tcur);


				//NOTE: currently current averaging doesn't depend on seasonal migrations
				if (param->vert_movement[sp] && param->age_compute_habitat[sp][age]!=param->age_compute_habitat[sp][age-1]){
					if (!tags_only || tags_age_habitat(age))
						func.Average_currents(*param, mat, map, age, tcur, pop_built);
				}
				
				//2014: catch at age computation for fisheries without effort data
				//pop.Ctot_no_effort_sp_age_comp(map, *param, mat, value(mat.dvarDensity(sp,age)), tcur, sp, age);				
				//this section will work only if seasonality switch is ON 
				//and for <1 maturity at age parameter
				if (param->migrations_by_maturity_flag && param->seasonal_migrations[sp]){
					cout << "In this version no smooth maturity; enter the age at first maturity. Exit now!" << endl; exit(1);
					
				} // end of section with seasonality switch and <1 maturity at age parameter 
				else {
					migration_flag = 0;
					if (age>=param->age_mature[sp] && param->seasonal_migrations[sp]) migration_flag = 1;

					if (param->age_compute_habitat[sp][age]!=param->age_compute_habitat[sp][age-1]) {
						if (!tags_only || tags_age_habitat(age))
							func.Feeding_Habitat(*param,mat,map,Habitat,sp,age,jday,tcur,migration_flag);
					}
					double mean_age = mean_age_cohort[sp][age];

					if (!param->food_requirement_in_mortality(sp)){
						func.Mortality_Sp(*param, mat, map, Mortality, Habitat, sp, mean_age, age, tcur);//checked
					} else {
						Food_Requirement_Index(IFR, FR_pop, ISR_denom, sp, age, tcur, jday);
						func.Mortality_Sp(*param, mat, map, Mortality, IFR, sp, mean_age, age, tcur);//checked
					}
					//Compute fluxes here
					FluxesComp(mat.dvarDensity(sp,age), Habitat, Mortality, age, fishing, year, month, jday, step_fishery_count,tcur,0);

					if (param->age_compute_habitat[sp][age]!=param->age_compute_habitat[sp][age-1]){
						if (!tags_only || tags_age_habitat(age))
							pop.Precaldia_Caldia(map, *param, mat, Habitat, Total_pop, sp, age, tcur, jday);//checked	
					}
					if (!param->gcalc()){
						// only in simulation mode: compute mean speed 
						// in BL/sec and mean diffusion rate in nmi^2/day
						mat.MeanVarMovement(map,value(mat.dvarsAdvection_x),
							value(mat.dvarsAdvection_y),
				 			value(mat.dvarsDiffusion_y),
							param->MSS_species[sp],
							param->sigma_species[sp],
							param->length(sp,age),
							param->length(sp,param->sp_nb_cohorts[sp]-1),
							deltaT,sp,age);
					}
						
					//store adult habitat and update the counter
					if (param->age_compute_habitat[sp][age]!=param->age_compute_habitat[sp][age-1]){
						//cout << age << " " << param->age_compute_habitat[sp][age] << endl;
						mat.adult_habitat(sp,tcur,param->age_compute_habitat[sp][age]) = value(Habitat);
					}
					for (int spop=spop0; spop<nb_tagpops+1; spop++){
						mortality_off = 0;
						if (sum(param->mask_fishery_sp) && !tags_only)
							fishing = true;

						if (spop>0) { 
							Mortality.initialize();
							mortality_off = 1;
							if (fishing) fishing = false;
						}
						if (spop>0){
							if (!tagpop_age_solve(spop-1,t_count-1,age))
								continue;
						}
						pop.Precalrec_Calrec_adult(map,mat,*param,rw,
							mat.dvarDensity[spop][age],Mortality,
							tcur,fishing,age,sp,year,month,jday,
							step_fishery_count,mortality_off);//checked 20150210
					}
					if (sum(param->mask_fishery_sp) && !tags_only) fishing = true;
				}
				age++;
			}//END OF IMPLICIT 'age' LOOP

			//----------------------------------------------//
			//    Now get predicted catch without effort  	//
			//----------------------------------------------//
			if (fishing && param->fisheries_no_effort_exist(sp))
				pop.Predicted_Catch_Fishery_no_effort(map,*param,mat,rw,sp,year,month);

			//store fish density after transport and mortality
			for (int a=0; a<aN_adult(sp); a++)
				mat.density_after(sp,a) = value(mat.dvarDensity(sp,a));
			if (writeoutputfiles){
				CalcMeanTemp(t_count,tcur);
				CalcSums();
			}
			//Compute total stock before the new recruitment (survival)
			if (param->stock_like[sp] && t_count > nbt_building+nbstoskip)
				Total_Stock_comp(total_stock, sp);

			//5. Compute spawning biomass (sum of young and adults density weighted by maturity-at-age)
			if (!tags_only)
				SpawningBiomass_comp(Total_pop, sp);
			//----------------------------------------------//
			//	    TUNA AGEING AND SPAWNING		//
			//----------------------------------------------//

			//6. Ageing and survival
			//6.1. First, update the age structure of tagged cohorts
			if (nt_dtau==dtau){
				for (int a=param->sp_nb_cohorts[sp]-1; a >= 1; a--){
					for (int spop=spop0; spop<nb_tagpops+1; spop++){
						if (spop>0){
							if (tagpop_age_solve(spop-1,t_count-1,a-1)==1){					
								tagpop_age_solve(spop-1,t_count,a-1)=0;
								tagpop_age_solve(spop-1,t_count,a)=1;
							}
						}
						//6.2. Ageing of population density
						Survival(mat.dvarDensity[spop][a], mat.dvarDensity[spop][a-1] , a, sp);
					}
				}
				nt_dtau=0;
			}
			//7. Spawning
			if (!tags_only)
				Spawning(mat.dvarDensity[sp][0],Spawning_Habitat,Total_pop,jday,sp,tcur);//checked

			//8. Extract larvae density
			if (!elarvae_model){
				if (year>=param->larvae_like_firstyear && year<=param->larvae_like_lastyear){
				//if (t_count > nbt_building+nbstoskip){
					if (param->larvae_like[0]){
						extract_larvae(sp,tcur);
					}
				}
			}
			

		}//end of 'sp' loop

		//------------------------------------------------------//
		//		LIKELIHOOD SECTION			//
		//------------------------------------------------------//
		//I. Total abundance likelihood
		//II.  Early-life data likelihood
		if (t_count == nbt_total){

			stocklike += get_stock_like(total_stock, likelihood);

			//II. Early-life data likelihood: only once at last time step
			if (param->larvae_like[0] && param->larvae_input_aggregated_flag[0]){
				get_larvae_at_obs();
				larvaelike += get_larvae_like(likelihood, Agg_larvae_density_pred_at_obs);
			}		
		}
		if (param->larvae_like[0] && !param->larvae_input_aggregated_flag[0] && year>=param->larvae_like_firstyear && year<=param->larvae_like_lastyear){
			// Read larvae input data
			int nbytetoskip = (9 +(3* nlat * nlon) + (nbt_total - nbt_building-nbstoskip) + ((nlat *nlon)* (t_count-nbt_building-nbstoskip-1))) * 4;
			rw.rbin_input2d(param->strfile_larvae, map, mat.larvae_input[tcur], nbi, nbj, nbytetoskip);
			// Compute likelihood
			larvaelike += get_larvae_like(likelihood, Larvae_density_pred, mat.larvae_input, tcur);
		}

		//II. Tag data likelihood
		if (param->tag_like[0])
			taglike += get_tag_like(likelihood, writeoutputfiles);

		//III. Fishing data likelihoods
		if ((fishing) && (t_count > nbt_building+nbstoskip) && (t_count <= nbt_no_forecast))
			get_catch_lf_like(likelihood);

		//------------------------------------------------------//
		//		COUPLAGE THON -FORAGE			//
		//------------------------------------------------------//
		if (param->flag_coupling){
			//----------------------------------------------//
			//		FORAGE RECRUITMENT		//
			//----------------------------------------------//
			for (int n=0; n< nb_forage; n++){
				mat.forage[tcur][n] += mat.mats[n];
				sumFprime[n] = sum(DtoBcell(mat.mats[n]));
			}
			
			PredationMortality(tcur, value(Total_pop));
			
			//----------------------------------------------//
			// 	TRANSPORT + PERTE DU FORAGE		//
			//----------------------------------------------//
			for (int n=0; n< nb_forage; n++){

				AverageCurrents(tcur,n);

				SolveADRE(mat.forage(tcur),n);
			}

		}// end of flag_coupling condition

		if (writeoutputfiles){
			if (!param->gcalc())	
				ConsoleOutput(1,value(likelihood));

			// let's count only those catch which will be used in the likelihood
			if (t_count <= nbt_building+nbstoskip) SUM_CATCH = 0.0;

			WriteFluxes();

			if (tuna_spinup && t_count == nbt_building + 60){//temporal: the nb to skip can be set through parfile
				//cout << date_str << "Saving initial distributions for simulation skipping spinup" << endl;
				SaveDistributions(year, month);
			}
		}
		nt_dtau++;
		past_month=month;
		step_count++;
		if (qtr != past_qtr) past_qtr = qtr; 

	} // end of simulation loop


	if (writeoutputfiles) {SaveDistributions(year, month);
		cout << "total catch in simulation: " << SUM_CATCH << endl;
	}
	param->total_like = value(likelihood);
	double clike = value(likelihood)-lflike-taglike-stocklike;
	if (!param->scalc()) // all but sensitivity analysis
		cout << "end of forward run, likelihood: " << defaultfloat <<
		clike << " " << lflike << " " << taglike << " " << stocklike << endl;

	return value(likelihood);
}

///This function uses regional structure defined in the parfile to compute fluxes as biomass flow rates between regions.
void SeapodymCoupled::FluxesComp(dvar_matrix Density, dvar_matrix Habitat, dvar_matrix Mortality, const int age, const bool fishing, const int year, const int month, const int jday, const int step_fishery_count, const int tcur, const double ts_fraction)
{
	if (fluxes_between_polygons){
		FluxesComp_polygons(Density,Habitat,Mortality,age,fishing,year,month,jday,step_fishery_count,tcur,ts_fraction);
		return;
	}

//temporal here:
int movement_fluxes_only = 1;
	const int sp = 0;
	const int nb_reg = param->nb_region_sp_B[sp];

	dvar_matrix Mortality_copy;
	dvar_matrix Habitat_copy;
	Habitat_copy.allocate(map.imin1, map.imax1, map.jinf1, map.jsup1);
	Mortality_copy.allocate(map.imin, map.imax, map.jinf, map.jsup);
	Habitat_copy = Habitat;
	Mortality_copy = Mortality;

	if (movement_fluxes_only) Mortality_copy.initialize();	

	if (fluxes_dt_qtr && (month==1 || month==4 || month==7 || month==10)){
		mat.fluxes_region(age).initialize();
	}

	if (nb_reg>0){
	    for (int r=0; r<nb_reg; r++){
		int reg = param->area_sp_B[sp][r]-1;

		//For MFCL need to extract quarterly metrics
		if (fluxes_dt_qtr && (month==1 || month==4 || month==7 || month==10)){	
			Density_region(reg,age).initialize();
			//only biomass in region reg is non-zero
	        	for (int i=map.imin; i <= map.imax; i++){
		     		for (int j=map.jinf[i] ; j<=map.jsup[i] ; j++){
	        			if (map.carte[i][j]){
						if ((i>=map.regimin[reg]) && (i<map.regimax[reg])
						 && (j>=map.regjmin[reg]) && (j<map.regjmax[reg]))
							Density_region(reg,age,i,j) = Density(i,j);
					}
		  		}
		 	}
		}
		if (age<a0_adult[sp]){
			pop.Precalrec_juv(map, mat, Mortality_copy, tcur,ts_fraction);
			pop.Calrec_juv(map, mat, Density_region(reg,age), Mortality_copy, tcur,ts_fraction);
		}
		if (age>=a0_adult[sp]){
			pop.Precaldia_Caldia(map, *param, mat, Habitat_copy, Mortality_copy, sp, age, tcur,jday);
			pop.Precalrec_Calrec_adult(map,mat,*param,rw,Density_region(reg,age),Mortality_copy,tcur,fishing,age,sp,year,month,jday,step_fishery_count,0);
		}

		//now integrate biomass in each region, region 'reg' included (what's left in it after movement)
		for (int rr=0; rr<nb_reg; rr++){
	       		int oth_reg = param->area_sp_B[sp][rr]-1;
			double tot_mass_region = 0.0;
			double weight = param->weight[sp][age]*0.001;
		
			for (int i=map.imin; i <= map.imax; i++){
				for (int j=map.jinf[i] ; j<=map.jsup[i] ; j++){
	       				if (map.carte[i][j]){
						if ((i>=map.regimin[oth_reg]) && (i<map.regimax[oth_reg])
						 && (j>=map.regjmin[oth_reg]) && (j<map.regjmax[oth_reg]))
							tot_mass_region += value(Density_region(reg,age,i,j))*weight*param->cell_surface_area(j);		
					}
			     	}
			}
			mat.fluxes_region(age,reg,oth_reg) = tot_mass_region;
		}
	    }
	}
}

///This function uses EEZ mask, which can contain both EEZ contours and arbitrary, non-overlapping polygons, and computes fluxes as biomass flow rates between different polygons. 
void SeapodymCoupled::FluxesComp_polygons(dvar_matrix Density, dvar_matrix Habitat, dvar_matrix Mortality, const int age, const bool fishing, const int year, const int month, const int jday, const int step_fishery_count, const int tcur, const double ts_fraction)
{
//temporal here:
int movement_fluxes_only = 1;
	const int sp = 0;

	dvar_matrix Mortality_copy;
	dvar_matrix Habitat_copy;
	Habitat_copy.allocate(map.imin1, map.imax1, map.jinf1, map.jsup1);
	Mortality_copy.allocate(map.imin, map.imax, map.jinf, map.jsup);
	Habitat_copy = Habitat;
	Mortality_copy = Mortality;

	if (movement_fluxes_only) Mortality_copy.initialize();	

	if (fluxes_dt_qtr && (month==1 || month==4 || month==7 || month==10)){
		mat.fluxes_region(age).initialize();
	}

	//rd - donor region
	for (int rd=0; rd<param->nb_EEZ; rd++){
		//For MFCL need to extract quarterly metrics
		if (fluxes_dt_qtr && (month==1 || month==4 || month==7 || month==10)){	
			Density_region(rd,age).initialize();
			//only biomass in region rd is non-zero
			//initialized with the model state N(a,t,x,y)
	        	for (int i=map.imin; i <= map.imax; i++){
		     		for (int j=map.jinf[i] ; j<=map.jsup[i] ; j++){
	        			if (map.carte[i][j] && (map.maskEEZ[i][j] == param->EEZ_ID[rd])){
						Density_region(rd,age,i,j) = Density(i,j);
					}
		  		}
		 	}
		}
		if (age<a0_adult[sp]){
			pop.Precalrec_juv(map, mat, Mortality_copy, tcur,ts_fraction);
			pop.Calrec_juv(map, mat, Density_region(rd,age), Mortality_copy, tcur,ts_fraction);
		}
		if (age>=a0_adult[sp]){
			pop.Precaldia_Caldia(map, *param, mat, Habitat_copy, Mortality_copy, sp, age, tcur,jday);
			pop.Precalrec_Calrec_adult(map,mat,*param,rw,Density_region(rd,age),
							Mortality_copy,tcur,fishing,age,sp,year,
							month,jday,step_fishery_count,0);
		}
		//now integrate biomass in each region, region 'reg' included (what's left in it after movement)
		//rr - recipient region
		for (int rr=0; rr<param->nb_EEZ; rr++){
			double tot_mass_region = 0.0;
			double weight = param->weight[sp][age]*0.001;
		
			for (int i=map.imin; i <= map.imax; i++){
				for (int j=map.jinf[i] ; j<=map.jsup[i] ; j++){
	       				if (map.carte[i][j] && (map.maskEEZ[i][j] == param->EEZ_ID[rr])){
						tot_mass_region += value(Density_region(rd,age,i,j))*
									weight*param->cell_surface_area(j);		
					}
			     	}
			}
			mat.fluxes_region(age,rd,rr) = tot_mass_region;
		}
	}
}

