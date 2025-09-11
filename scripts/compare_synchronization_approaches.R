
# User inputs ---------------------------------------------------------------
nb_ages = 37		# Number of age classes
nb_workers = 37 		# Number of workers
nsteps = 132		# Simulation time
add_randomness = F  # Weither to add randomness in each step computation time
rnorm_sd = 0.1		# Randomness factor (baseline computation time will be multiplied by rnorm(mean=1, sd = rnorm_sd))
show_matrices <- F  # Weither to show time matrices in the output or not
time_init = 9 

# Baseline computation times (age-specific)
ages_ctime = data.frame(age = (1:nb_ages)-1,
						#time = 1 + c(1:nb_ages)*0.1)
						time = 1)
# set.seed(1)

# Functions ---------------------------------------------------------------
get_start <- function(cohort_id, nb_ages){
	quotient <- cohort_id %/% nb_ages
	remainder <- cohort_id %% nb_ages
	if (cohort_id >= nb_ages) {
		age_start <- 0
		t_start <- 2 + (quotient - 1) * nb_ages + remainder
	} else {
		age_start <- remainder
		t_start <- 1
	}
	return(c(age_start, t_start))
}
get_cohort_lifespan <- function(age_start, t_start, nsteps){
	cohort_lifespan <- nb_ages - age_start
	if ((t_start + cohort_lifespan - 1)>nsteps) 
		cohort_lifespan <- nsteps - t_start + 1
	return(cohort_lifespan)
}
print_matrix <- function(C){
	rownames(C) <- 1:nsteps
	colnames(C) <- paste("  ", 0:(nb_cohorts - 1))
	
	newC <- C
	newC <- ifelse(
		C < 1,
		formatC(C, digits = 3),
		formatC(C, digits = 4)
	)
	# newC <- formatC(newC, digits=4)
	newC[is.na(newC)] <- "    ."
	print(noquote(newC))
}

# Setup -------------------------------------------------------------------
nb_cohorts <- nb_ages + nsteps -1
is_cohort_active <- matrix(NA, nrow = nsteps, ncol=nb_cohorts)	# Matrix nsteps x nb_cohorts: whether the cohort needs to be computed at that time step 
age_starts <- rep(NA, nb_cohorts)								# Starting age of each cohort
t_starts <- rep(NA, nb_cohorts)									# Starting time of each cohort 
for (tstep in 1:nsteps){
	for (cohort_id in 0:(nb_cohorts-1)){
		at_start <- get_start(cohort_id, nb_ages)
		age_start <- at_start[1]
		t_start <- at_start[2]
		
		cohort_lifespan <- get_cohort_lifespan(age_start, t_start, nsteps)
		
		is_cohort_active[tstep, cohort_id+1] <- (t_start <= tstep) & (tstep < (t_start + cohort_lifespan))
		if (is.na(age_starts[cohort_id+1])) 
			age_starts[cohort_id+1] <- age_start
		if (is.na(t_starts[cohort_id+1])) 
			t_starts[cohort_id+1] <- t_start
	}
}

# First exp: Same number of workers as age classes ----------------------------------------

## Approach 1 : sync at each time step
C <- matrix(NA, nrow = nsteps, ncol=nb_cohorts) # Matrix that records the time to reach a specific time step for a specific cohort
start_time_tstep = 0
for (tstep in 1:nsteps){
	for (cohort_id in 0:(nb_cohorts-1)){
		is_active <- is_cohort_active[tstep, cohort_id+1]
		if (is_active){
			is_new <- t_starts[cohort_id +1] == tstep
			age_start <- age_starts[cohort_id + 1]
			age <-tstep - t_starts[cohort_id +1] + age_starts[cohort_id +1] 
			t <- ages_ctime$time[age+1]
			if (add_randomness) t <- t * rnorm(1, mean=1, sd=rnorm_sd)
			if (is_new) t = t+time_init
			C[tstep, cohort_id+1] <- start_time_tstep + t
		}
	}
	start_time_tstep <- max(C[tstep,], na.rm=T)
}

## Approach 2: each cohort runs for its life time
## wait time = wait for needed previous cohorts to reach needed steps
C2 <- matrix(NA, nrow = nsteps, ncol=nb_cohorts) 
C2[1,1:nb_ages] <- ages_ctime$time + time_init
for (tstep in 2:nsteps){
	for (cohort_id in 0:(nb_cohorts-1)){
		is_active <- is_cohort_active[tstep, cohort_id+1]
		is_new <- t_starts[cohort_id +1] == tstep
		if (is_active){
			if (is_new){
				age <- age_starts[cohort_id + 1]
				waiting_time <- max(C2[tstep-1,], na.rm=T)
				nt <-  ages_ctime$time[age + 1]
				if (add_randomness) nt = nt * rnorm(1, mean=1, sd=rnorm_sd)
				nt <- nt+time_init
				C2[tstep, cohort_id+1] <- nt + waiting_time
			}else{
				age <-tstep - t_starts[cohort_id +1] + age_starts[cohort_id +1] 
				C2[tstep, cohort_id+1] <- ages_ctime$time[age + 1] + C2[tstep-1, cohort_id+1]
			}
		}
	}
}


# Second exp: Number of workers < number age classes ----------------------------------

## Approach 1 : sync at each time step
C3 <- matrix(NA, nrow = nsteps, ncol=nb_cohorts)
for (tstep in 1:nsteps){
	active_cohorts <- which(is_cohort_active[tstep,]) - 1
	active_workers <- 1:nb_workers
	cohorts_wk <- data.frame(id = 1:length(active_cohorts),
						   cohort_id = active_cohorts) 
	cohorts_wk$wk <- with(cohorts_wk, (id-1) %% nb_workers + 1)
	for (wk in 1:nb_workers){
		cohort_ids <- cohorts_wk$cohort_id[cohorts_wk$wk==wk]
		previous_cohort <- NA
		for (cohort_id in cohort_ids){
			# if(tstep==2 & cohort_id==5) stop()
			is_new <- t_starts[cohort_id +1] == tstep
			if (is_new){
				age <- age_starts[cohort_id + 1]
			}else{
				age <-tstep - t_starts[cohort_id +1] + age_starts[cohort_id +1] 
			}
			step_time <- ages_ctime$time[age + 1]
			if (add_randomness) step_time <- step_time * rnorm(1, mean=1, sd=rnorm_sd)
			if (is_new) step_time = step_time+time_init
					
			if (is.na(previous_cohort)){
				if (tstep==1){
					C3[tstep, cohort_id+1] <- step_time
				}else{
					C3[tstep, cohort_id+1] <- max(C3[tstep-1,], na.rm=T) + step_time
				}
			}else{
				C3[tstep, cohort_id+1] <- C3[tstep, previous_cohort+1] + step_time
			}
			previous_cohort <- cohort_id
		}
	}
}


## Approach 2: each cohort runs for its life time
## wait time = wait for available worker and for needed previous cohorts to reach needed steps
C4 <- matrix(NA, nrow = nsteps, ncol=nb_cohorts)
workers <- matrix(0:(nb_workers-1), nrow=1, ncol=nb_workers)# Will keep track of the cohorts performed by the workers
running_cohorts <- 0:(nb_workers-1)
cohort_ids <- 0:(nb_cohorts-1)
C4[1, 1:nb_workers] <- ages_ctime$time[1:nb_workers]
for (cohort_id in cohort_ids){
	# if (cohort_id==3) stop()
	t_start = t_starts[cohort_id+1]
	age_start = age_starts[cohort_id+1]
	
	# Waiting time for previous cohorts to be completed
	if (t_start==1){
		needed_cohorts <- NULL
	}else{
		needed_cohorts <- (cohort_id - nb_ages):(cohort_id-1)
	}
	if (!is.null(needed_cohorts)){
		waiting_time_cohorts <- max(C4[t_start-1,needed_cohorts+1], na.rm=T)
	}else{
		waiting_time_cohorts <- 0
	}
	
	# Waiting time for a worker to be available
	if (cohort_id < nb_workers){
		available_worker <- cohort_id + 1
		waiting_time_workers <- 0
	}else{
		running_cohorts_times <- unlist(lapply(running_cohorts, function(c) max(C4[,c+1], na.rm=T)))
		imin <- which.min(running_cohorts_times)[1]
		min_time <- running_cohorts_times[imin]
		fastest_cohort <- running_cohorts[imin]
		running_cohorts[imin] <- cohort_id
		
		available_worker <- imin
		waiting_time_workers <- min_time
	}
	
	# Filling times
	waiting_time = max(waiting_time_cohorts, waiting_time_workers)
	cohort_lifespan <- get_cohort_lifespan(age_start, t_start, nsteps)

	for (age in age_start:(age_start+cohort_lifespan-1)){
		tstep <- t_start - age_start + age
		is_new <- t_starts[cohort_id +1] == tstep
		step_time <- ages_ctime$time[age+1]
		if (add_randomness){
			step_time <- step_time * rnorm(1, mean=1, sd=rnorm_sd)
		}
		if (is_new) step_time = step_time+time_init
		if (age==age_start){
			C4[tstep, cohort_id + 1] = step_time + waiting_time
		}else{
			if (tstep==1){
				C4[tstep, cohort_id + 1] = step_time + waiting_time
			}else{
				C4[tstep, cohort_id + 1] = C4[tstep-1, cohort_id + 1] + step_time
			}
		}
	}
}

# Output ------------------------------------------------------
cat("\nComparison of two approaches:\n")
cat("\t- Approach 1: Synchronization between cohorts at each time step. Need to wait for all cohorts to complete their time step to move forward.\n")
cat("\t- Approach 2: Each cohort runs for its life span, independetly from the others. At each time step, the newborn cohort still needs for the necessary previous cohorts to reach the previous time step.\n")

cat("\n\n###########################################\n")
cat("   User input \n")
cat("###########################################\n")
cat(sprintf("\n%s age classes, %s time steps -> %s cohorts\n", nb_ages, nsteps, nb_cohorts))
cat(sprintf("\nTime spent for prerun (read forcing, etc.): %s\n",time_init))
cat(sprintf("\nBaseline computation time at age: %s\n", paste(ages_ctime$time, collapse=" ")))
if (add_randomness){
	cat(sprintf("\nWith a random aspect to each step's computation time (* rnorm(mean = 1, sd = %s))\n", rnorm_sd)) 
}else{
	cat("\nNo randomness in computation time.\n") 
}

if (show_matrices){
	options(width=120)# so that it uses more width of the terminal to print the output
	cat("\n\n\n###########################################\n")
	cat("   Time matrices\n")
	cat("###########################################\n")
	cat("Here are displayed for each case and approach the time necessary to complete each each time step (row) for each cohort (col)\n\n")
	
	cat("#### Number of workers = number of age classes\n")
	cat("\tApproach 1:\n")
	print_matrix(C)
	cat("\n\tApproach 2:\n")
	print_matrix(C2)
	
	cat("\n\n#### Number of workers < number of age classes\n")
	cat("\tApproach 1:\n")
	print_matrix(C3)
	cat("\n\tApproach 2:\n")
	print_matrix(C4)
}


cat("\n###########################################\n")
cat("   Outcome\n")
cat("###########################################\n")

time_sequential = time_init + sum(rep(nsteps, sum(ages_ctime$time)))
cat(sprintf("\nSequential approach: %.01fms\n", time_sequential))

cat("\n#### Number of workers = number of age classes\n")
cat(sprintf("\tApproach 1: %.01fms (%.01f quicker than sequential)\n\tApproach 2: %.01fms (%.01f quicker than sequential)\n", max(C, na.rm=T), time_sequential / max(C, na.rm=T), max(C2, na.rm=T), time_sequential / max(C2, na.rm=T)))

cat("\n#### Number of workers <= number of age classes\n")
cat(sprintf("%s workers\n", nb_workers))

cat(sprintf("\tApproach 1: %.01fms (%.01f quicker than sequential)\n\tApproach 2: %.01fms (%.01f quicker than sequential)\n", max(C3, na.rm=T), time_sequential / max(C3, na.rm=T), max(C4, na.rm=T), time_sequential / max(C4, na.rm=T)))

