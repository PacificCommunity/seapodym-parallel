#include "VarSimtunaFunc.h"

///Main function with memory control and adjoint functions for: 
///mortality rates at age functions. These functions include fixed natural mortality
///rate and variable component, depending on habitat indices defined for the life stage

// scaling factor for young larvae before sst-dependent larvae mortality until 1st time step

///Forward functions are in mortality_sp.cpp


void dv_M_sp_comp(void);
void dv_Scaling_factor_sstdep_larvae_mortality_comp(void);
void save_long_int_value(unsigned long int x);
unsigned long int restore_long_int_value(void);

void VarSimtunaFunc::Mortality_Sp(VarParamCoupled& param, CMatrices& mat, const PMap& map, dvar_matrix& M, dvar_matrix& H, const int sp, const double mean_age_in_dtau, const int age, const int t_count)
{
	M.initialize();

	double Rage = mat.mortality_range_age[sp][age];
	double Hval = param.Hval;

	dvariable Mp_max   = param.dvarsMp_mean_max[sp];
	dvariable Mp_exp   = param.dvarsMp_mean_exp[sp];
	dvariable Ms_slope = param.dvarsMs_mean_slope[sp];
	dvariable Ms_max   = param.dvarsMs_mean_max[sp];
	dvariable range;
	if (age==0)
		range = param.dvarsM_larvae_range[sp]; 
	else 
		range = param.dvarsM_mean_range[sp];

	dvmatr1 = Mp_max;
	dvmatr2 = Mp_exp;
	dvmatr3 = Ms_slope;
	dvmatr4 = Ms_max;
	dvmatr5 = range;

	M_sp_comp(map, M, value(H), value(Mp_max), value(Ms_max), value(Mp_exp), value(Ms_slope), value(range), Rage, Hval, mean_age_in_dtau);

	//cout << __FILE__<< " " << __LINE__ << " " << age << " " << norm(M) << endl;

    if (!param.gcalc()){
                // pH option works only in simulation mode
		if ((age==0) && param.use_ph1)
			M_PH_juv_comp(param,map,mat,M,mat.ph1[t_count],mean_age_in_dtau);	

		mat.MeanVarMortality(map, value(M),value(Mp_max), value(Ms_max), value(Mp_exp), value(Ms_slope), mean_age_in_dtau, sp, age);
	}

	save_identifier_string((char*)"M_sp_comp_begin");
	save_double_value(mean_age_in_dtau);
	range.save_prevariable_value();
	dvmatr5.save_dvar_matrix_position();
	Ms_max.save_prevariable_value();
	dvmatr4.save_dvar_matrix_position();
	Ms_slope.save_prevariable_value();
	dvmatr3.save_dvar_matrix_position();
	Mp_exp.save_prevariable_value();
	dvmatr2.save_dvar_matrix_position();
	Mp_max.save_prevariable_value();
	dvmatr1.save_dvar_matrix_position();
	if (age<param.sp_a0_adult[sp])
		H.save_dvar_matrix_value();
	H.save_dvar_matrix_position();
	M.save_dvar_matrix_position();
	save_int_value(sp);
	save_int_value(age);
	save_int_value(param.sp_a0_adult[sp]);
	save_int_value(t_count);
	unsigned long int pmap = (unsigned long int)&map;
	save_long_int_value(pmap);
	unsigned long int cparam = (unsigned long int)&param;
	save_long_int_value(cparam);
	unsigned long int cmat = (unsigned long int)&mat;
	save_long_int_value(cmat);
	save_identifier_string((char*)"M_sp_comp_end");

	gradient_structure::GRAD_STACK1->set_gradient_stack(dv_M_sp_comp);

}

void dv_M_sp_comp(void)
{
	verify_identifier_string((char*)"M_sp_comp_end");
	unsigned long int pos_mat = restore_long_int_value();
	unsigned long int pos_param = restore_long_int_value();
	unsigned long int pos_map = restore_long_int_value();
	unsigned int t_count  = restore_int_value();
	unsigned int a0_adult = restore_int_value();
	unsigned int age      = restore_int_value();
	unsigned int sp       = restore_int_value();
	const dvar_matrix_position Mpos = restore_dvar_matrix_position();
	const dvar_matrix_position Hpos = restore_dvar_matrix_position();
	dmatrix H;
	if (age<a0_adult) H = restore_dvar_matrix_value(Hpos);
	const dvar_matrix_position Mpmax_pos  = restore_dvar_matrix_position();
	double Mp_max = restore_prevariable_value();
	const dvar_matrix_position Mpexp_pos  = restore_dvar_matrix_position();
	double Mp_exp = restore_prevariable_value();
	const dvar_matrix_position Msslope_pos  = restore_dvar_matrix_position();
	double Ms_slope = restore_prevariable_value();
	const dvar_matrix_position Msmax_pos  = restore_dvar_matrix_position();
	double Ms_max = restore_prevariable_value();
	const dvar_matrix_position Range_pos  = restore_dvar_matrix_position();
	double range = restore_prevariable_value();
	const double mean_age = restore_double_value();
	verify_identifier_string((char*)"M_sp_comp_begin");

	dmatrix dfM 	 = restore_dvar_matrix_derivatives(Mpos);
	dmatrix dfH 	 = restore_dvar_matrix_derivatives(Hpos);
	dmatrix dfMpm 	 = restore_dvar_matrix_derivatives(Mpmax_pos);
	dmatrix dfMpe 	 = restore_dvar_matrix_derivatives(Mpexp_pos);
	dmatrix dfMss 	 = restore_dvar_matrix_derivatives(Msslope_pos);
	dmatrix dfMsm 	 = restore_dvar_matrix_derivatives(Msmax_pos);
	dmatrix dfRng 	 = restore_dvar_matrix_derivatives(Range_pos);

	PMap* map  = (PMap*) pos_map;
	CParam* param  = (CParam*) pos_param;
	CMatrices* mat = (CMatrices*) pos_mat;

	const double Hval   = param->Hval;
	const double expr1  = exp(-mean_age*Mp_exp);


	const double Mp = Mp_max * expr1;
	const double Ms = Ms_max * pow(mean_age,Ms_slope);
	const double M  = Mp + Ms;


	const double dMpm = expr1;
	const double dMpe = -mean_age*Mp_max*expr1;

	const double dMsm = pow(mean_age,Ms_slope);
	const double dMss = Ms_max*pow(mean_age,Ms_slope) * log(mean_age);

	double Rage = mat->mortality_range_age[sp][age];

	//restore Habitat value
	if (age>=a0_adult){
		//dmatrix H(Hpos);
		H.allocate(map->imin1, map->imax1, map->jinf1, map->jsup1);
		H.initialize();
		int ind_adult_habitat = param->age_compute_habitat(sp,age);
	        /*int age_adult_habitat = age;
	        for (int aa=age; aa>1; aa--){
	                if (param->age_compute_habitat(sp,aa-1)!=param->age_compute_habitat(sp,aa)){
	                        age_adult_habitat = aa;
	                        break;
	                }
	        }*/
		H = mat->adult_habitat(sp,t_count,ind_adult_habitat);
	}

	const int imax = map->imax;
	const int imin = map->imin;
	for (int i = imax; i >= imin; i--){
		const int jmin = map->jinf[i];
		const int jmax = map->jsup[i];
		for (int j = jmax; j >= jmin; j--){
			if (map->carte(i,j)){	
				double exprH = 1.0-H(i,j)/Hval;
				double H_var = pow(1.0+range+Rage,exprH); 

				//M(i,j) *= pow(1.0+range,1-H(i,j)/Hval);
				dfH(i,j) -= M * log(1.0+range+Rage) * H_var * dfM(i,j) / Hval;
				dfRng(i,j) += M * exprH * pow(1.0+range+Rage,-H(i,j)/Hval) * dfM(i,j);

				//const double M = (Ms+Mp)*H_var;
				dfMpm(i,j) += dMpm * H_var * dfM(i,j);
				dfMpe(i,j) += dMpe * H_var * dfM(i,j);
				dfMss(i,j) += dMss * H_var * dfM(i,j);
				dfMsm(i,j) += dMsm * H_var * dfM(i,j);
				dfM(i,j) = 0.0;
			}
		}
	}
	dfMpm.save_dmatrix_derivatives(Mpmax_pos); 
	dfMpe.save_dmatrix_derivatives(Mpexp_pos);
	dfMss.save_dmatrix_derivatives(Msslope_pos); 
	dfMsm.save_dmatrix_derivatives(Msmax_pos);
	dfRng.save_dmatrix_derivatives(Range_pos);
	dfH.save_dmatrix_derivatives(Hpos);
	dfM.save_dmatrix_derivatives(Mpos);
}


void VarSimtunaFunc::Scaling_factor_sstdep_larvae_mortality(VarParamCoupled& param, const dmatrix& sst, const PMap& map, dvar_matrix& S, const int sp)
{
	int deltaT = param.deltaT;
	dvariable inv_M_max = param.dvarsInv_M_max[sp];
	dvariable inv_M_rate = param.dvarsInv_M_rate[sp];
	dvariable age_larvae_before_sst_mortality = param.dvarsAge_larvae_before_sst_mortality[sp];

	dvmatr1 = inv_M_max;
	dvmatr2 = inv_M_rate;
	dvmatr3 = age_larvae_before_sst_mortality;

	Scaling_factor_sstdep_larvae_mortality_comp(map, S, sst, inv_M_max, inv_M_rate, age_larvae_before_sst_mortality, deltaT);

	unsigned long int pmap   = (unsigned long int)&map;
	save_identifier_string((char*)"Scaling_factor_sstdep_larvae_mortality_comp_begin");
	inv_M_rate.save_prevariable_value();
	inv_M_max.save_prevariable_value();
	age_larvae_before_sst_mortality.save_prevariable_value();
	sst.save_dmatrix_value();
	sst.save_dmatrix_position();
	S.save_dvar_matrix_position();
	dvmatr3.save_dvar_matrix_position();
	dvmatr2.save_dvar_matrix_position();
	dvmatr1.save_dvar_matrix_position();
	save_long_int_value(pmap);
	save_int_value(deltaT);
	save_identifier_string((char*)"Scaling_factor_sstdep_larvae_mortality_comp_end");

	gradient_structure::GRAD_STACK1->set_gradient_stack(dv_Scaling_factor_sstdep_larvae_mortality_comp);
}


void dv_Scaling_factor_sstdep_larvae_mortality_comp(void)
{
	verify_identifier_string((char*)"Scaling_factor_sstdep_larvae_mortality_comp_end");
	int deltaT = restore_int_value();
	unsigned long int pos_map  = restore_long_int_value();
	const dvar_matrix_position inv_M_max_pos  = restore_dvar_matrix_position();
	const dvar_matrix_position inv_M_rate_pos = restore_dvar_matrix_position();
	const dvar_matrix_position age_larvae_before_sst_mortality_pos = restore_dvar_matrix_position();
	const dvar_matrix_position S_pos  = restore_dvar_matrix_position();
	const dmatrix_position sst_pos  = restore_dmatrix_position();
	dmatrix sst = restore_dmatrix_value(sst_pos);
    double age_larvae_before_sst_mortality = restore_prevariable_value();
	double inv_M_max = restore_prevariable_value();
    double inv_M_rate = restore_prevariable_value();
	verify_identifier_string((char*)"Scaling_factor_sstdep_larvae_mortality_comp_begin");

	dmatrix dfInv_M_max = restore_dvar_matrix_derivatives(inv_M_max_pos);
	dmatrix dfInv_M_rate = restore_dvar_matrix_derivatives(inv_M_rate_pos);
	dmatrix dfAge_larvae_before_sst_mortality = restore_dvar_matrix_derivatives(age_larvae_before_sst_mortality_pos);
	dmatrix dfS = restore_dvar_matrix_derivatives(S_pos);
	PMap* map = (PMap*) pos_map;

	const int imax = map->imax;
	const int imin = map->imin;
	for (int i = imax; i >= imin; i--){
		const int jmin = map->jinf[i];
		const int jmax = map->jsup[i];
		for (int j = jmax; j >= jmin; j--){
			if (map->carte(i,j)){
				double h = 1 - 1 / (inv_M_max * exp(inv_M_rate * sst(i,j)));

				dfInv_M_max(i,j) += dfS(i,j) * (age_larvae_before_sst_mortality - deltaT) * pow(h, age_larvae_before_sst_mortality - 31) * (1-h) / inv_M_max;

				dfInv_M_rate(i,j) += dfS(i,j) * (age_larvae_before_sst_mortality - deltaT) * pow(h, age_larvae_before_sst_mortality - 31) * sst(i,j) * (1-h);

				dfAge_larvae_before_sst_mortality(i,j) += dfS(i,j) * log(h) * pow(h, age_larvae_before_sst_mortality - deltaT);

				dfS(i,j) = 0.0;
			}
		}
	}

	dfInv_M_max.save_dmatrix_derivatives(inv_M_max_pos);
	dfInv_M_rate.save_dmatrix_derivatives(inv_M_rate_pos);
	dfAge_larvae_before_sst_mortality.save_dmatrix_derivatives(age_larvae_before_sst_mortality_pos); 
	dfS.save_dmatrix_derivatives(S_pos);
}

