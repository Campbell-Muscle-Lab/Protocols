// MODULE ISOTWITCHPICONT
//
// isotwitch (in_pipe, iso_level, n_samples, iso_points, iso_gain, out_pipe)
// 		When in_pipe rises above iso_level, out_pipe feeds back to keep in_pipe
//		at iso_level, until out_pipe starts to change direction. It then holds
//		out_pipe constant for iso_points. The total number of samples is n_samples
//
// Module name ISOTWITCH must be distinct from the DAPL command name
// assigned below.

#define  COMMAND  "WINDKESSEL"
#define  ENTRY    WINDKESSEL_entry

#include "C:\Program Files\Microstar Laboratories\DTD\Include\DTDMOD.H"
#include "C:\Program Files\Microstar Laboratories\DTD\Include\DTD.H"

int __stdcall     ENTRY (PIB **plib);

extern "C" __declspec(dllexport) int __stdcall
   ModuleInstall(void *hModule)
{   return (CommandInstall(hModule, COMMAND, ENTRY, NULL)) ; }

// - - - - -  command implementation section  - - - - - - - - - -

int __stdcall  ENTRY (PIB **plib)
{
	// Storage for parameters
	void **argv;
	int argc;
	PIPE * in_force_pipe;
	PIPE * in_fl_pipe;
	long int pre_trigger_samples;
	short int isotonic_level;
	short int isotonic_increment;
	long int max_samples;
	long int isometric_points;
	long int ramp_points;
	long int inter_ramp_points; //added
	long int inter_fl_diff; //added
	short int prop_gain;
	short int Ki; //added
	short int fl_polarity;
	short int fl_switch;
	short int fl_threshold;
	short int smoothing_points;
	short int integral_points; //added
	short int gain_asymmetry;
	PIPE  * out_pipe;

	// Access parameters
	argv = param_process (plib, &argc, 19, 19,
		T_PIPE_W, T_PIPE_W,
		T_VAR_L, T_VAR_W, T_VAR_W, T_VAR_L, T_VAR_L,
		T_VAR_L, T_VAR_L, T_VAR_L, T_VAR_W, T_VAR_W,
		T_VAR_W, T_VAR_W, T_VAR_W, T_VAR_W, T_VAR_W,
		T_VAR_W, T_PIPE_W);

	in_force_pipe = (PIPE *) argv[1];
	in_fl_pipe = (PIPE *) argv[2];
	pre_trigger_samples = *(long int volatile *) argv[3];
	isotonic_level = *(short int volatile *) argv[4];
	isotonic_increment = *(short int volatile *) argv[5];
	max_samples = *(long int volatile *) argv[6];
	isometric_points = *(long int volatile *) argv[7];
	ramp_points = *(long int volatile *) argv[8];
	inter_ramp_points = *(long int volatile *) argv[9];
	inter_fl_diff = *(long int volatile *) argv[10];
	prop_gain = *(short int volatile *) argv[11];
	Ki = *(short int volatile *) argv[12];
	fl_polarity = *(short int volatile *) argv[13];
	fl_switch = *(short int volatile *) argv[14];
	fl_threshold = *(short int volatile *) argv[15];
	smoothing_points =*(short int volatile *) argv[16];
	integral_points =*(short int volatile *) argv[17];
	gain_asymmetry =*(short int volatile *) argv[18];
	out_pipe= (PIPE *) argv[19];

	// Additional variables
	
	int i;

	short int in_force_value;
	short int in_fl_value;
	short int out_value;
	short int last_out;
	short int last_out_min1;

	double speed_fl;
	double state_1;
	double state_2;
	double state_1_new;
	double state_2_new;
	double A1;
	double A2;
	double B1;
	double B2;
	double C1;
	double C2;
	double D;

	short int error_value;
	short int ramp_increment;
	short int out_diff_accumulator;
	short int error_sum; //added

	short int prop_gain2;
	
	int armed=0;
	int was_low;
	int windkessel_hold;
	int isometric_increment;
	int isometric_hold;
	int ramp_back;
	int inter_ramp; //added
	int run_started;

	long int sample_counter;
	long int isometric_counter;
	long int inter_ramp_counter; //added
	long int ramp_counter;
	long int ramp_n;

	short int * force_buffer;
	short int * last_out_buffer;
	short int * error_buffer;

	double double_increment;
	double double_inter_increment; //added
	double double_holder;
	double double_inter_holder; //added

	double windkessel_level;
	short int windkessel_int_level;

	// Perform initializations
	pipe_open(in_force_pipe, P_READ);
	pipe_open(in_fl_pipe, P_READ);
	pipe_open(out_pipe, P_WRITE);
	
	armed=0;
	was_low=0;
	windkessel_hold=0;
	isometric_hold=0;
	ramp_back=0;
	inter_ramp=0; //added
	run_started=0;

	sample_counter=0;
	inter_ramp_counter=0; //added
	isometric_counter=0;
	ramp_counter=0;

	double_inter_increment = double(inter_fl_diff) / double(inter_ramp_points);
	windkessel_int_level = isotonic_level;
	
	last_out = 0;
	last_out_min1 = 0;
	state_1_new = 2.0;
	state_2_new = 0;
	A1 = 0.999916670138793;
	A2 = 0.998501124437711;
	B1 = 0.0001249947918112726;
	B2 = 0.001498875562289;
	C1 = 1.0;
	C2 = -0.03;
	D  = 0.03;
	
	out_diff_accumulator = 0;
	error_sum = 0;

	// Create space for the out and error buffers
	last_out_buffer = (short int *) malloc(smoothing_points * sizeof(short int));
	error_buffer = (short int* ) malloc(integral_points * sizeof(short int));
	
	// Zero buffers
	for (i=0;i<smoothing_points;i++)
	{
		last_out_buffer[i]=(short int)0;
	}
	for (i=0;i<integral_points;i++)
	{
		error_buffer[i]=(short int)0;
	}

	
	int temp_counter=0;	
	long int error_holder;



	// Begin continuous processing
	while (1)
	{
		// Increment counter
		sample_counter++;
		
		// Read input values
		in_force_value = (short int) pipe_get(in_force_pipe);
		in_fl_value = (short int) pipe_get(in_fl_pipe);

		// Check if we are armed - that is, have collected enough samples
		if ((armed==0)&&(sample_counter>pre_trigger_samples))
			armed=1;

		// If we are armed, check we were below the isotonic level
		if ((armed==1)&&(in_force_value<windkessel_int_level))
			was_low=1;
		
		// If we were low, and we are far enough into the record, have we got above the target?
		// The run started variable prevents later re-triggering
		if ((was_low==1)&&(in_force_value>windkessel_int_level)&&(run_started==0))
		{
			run_started=1;
			windkessel_hold=1;
		}

		// Now deduce behavior
		out_value=0;
		
		if (windkessel_hold==1)
		{
			// Calculate rate of fl change in Volts
			speed_fl = ((((double)(last_out-last_out_min1))/32767.0)*5.0)/0.0002;
			printf("%f\n",speed_fl);
			
			// Write new state to current state
			state_1 = state_1_new;
			state_2 = state_2_new;

			// Calculate windkessel_level in Volts and integer values
			windkessel_level = C1*state_1 + C2*state_2 + D*speed_fl;
			windkessel_int_level = (short int)((windkessel_level/5.0)*32767.0);

			// Update the state values
			state_1_new = A1*state_1 + B1*speed_fl;
			state_2_new = A2*state_2 + B2*speed_fl;
			
			// Calculate force error
			error_value = (in_force_value - windkessel_int_level);
			
			// Update error_buffer
			for (i=0;i<(integral_points-1);i++)
			{
				error_buffer[i] = error_buffer[i+1];
			}
			error_buffer[integral_points-1] = error_value;

			// Calculate the error_sum
			error_holder=0;
			for (i=0;i<integral_points;i++)
			{
				error_holder = error_holder +	(long int)error_buffer[i];
			}
			error_sum = (short int)(error_holder/(long int)integral_points);

			
			if (out_diff_accumulator<0)
			{
				prop_gain2 = prop_gain + gain_asymmetry;
			}
			else
			{
				prop_gain2 = prop_gain;
			}
			
			//Calculates out_value with PIDcontrol, 100k is a constant that affrcts the gain params.
			out_value = last_out - 
				(fl_polarity * 
				((prop_gain * error_value) + (Ki * (short int)(error_holder / integral_points))) 
				/ 100000);


			// Test for breaking out of isotonic hold and into isometric hold
			if (fl_switch==0)
			{
				// Code breaks out of control loop if out_diff_accumulator
				// exceeds fl_threshold. Use this to stop isotonic control
				// when the motor reverses direction
				
				// Need to check for both fl_polarity options
				if (((fl_polarity>0)&&(out_diff_accumulator>fl_threshold)) ||
					((fl_polarity<0)&&(out_diff_accumulator<-fl_threshold)))
				{
					windkessel_hold=0;
					inter_ramp=1;
				}
			}
			else
			{
				// Code breaks out of control loop if motor returns to original value
				if (((fl_polarity>0)&&(out_value>fl_threshold)) ||
					((fl_polarity<0)&&(out_value<-fl_threshold)))
				{
					windkessel_hold=0;
					inter_ramp=1;
					printf("%i\n",out_value);
				}
			}

			// Update windkessel_int_level for next iteration by adding increment
			//windkessel_int_level = windkessel_int_level + (double)((double)isotonic_increment/1000.0);

			double_inter_holder = last_out;
		}

		if (inter_ramp==1)
		{
			inter_ramp_counter++;

			double_inter_holder = double_inter_holder + double_inter_increment;

			out_value = short int (double_inter_holder);

			if (inter_ramp_counter>=inter_ramp_points)
			{
				inter_ramp=0;
				isometric_hold=1;
				// out_value=0;
			}
		}
		
		if (isometric_hold==1)
		{
			isometric_counter++;

			// Test for breaking into ramp back
			if (isometric_counter>=isometric_points)
			{
				isometric_hold=0;
				ramp_back=1;

				double_increment = -double(in_fl_value) / double(ramp_points);
				double_holder = last_out;
			}

			out_value = last_out;
		}

		if (ramp_back==1)
		{
			ramp_counter++;

			double_holder = double_holder + double_increment;

			out_value = short int (double_holder);

			if (ramp_counter>=ramp_points)
			{
				ramp_back=0;
				out_value=0;
			}
		}

		// Emergency - force motor back to default for end of record
		if (sample_counter>max_samples)
		{
			out_value = 0;
		}

		// Update last_out_buffer
		for (i=0;i<(smoothing_points-1);i++)
		{
			last_out_buffer[i] = last_out_buffer[i+1];
		}
		last_out_buffer[smoothing_points-1] = last_out;

		// Calculate the out_diff_accumulator
		out_diff_accumulator=0;
		for (i=0;i<(smoothing_points-1);i++)
		{
			out_diff_accumulator = out_diff_accumulator +
				(last_out_buffer[i+1]-last_out_buffer[i]);
		}

		// Hold value for next time
		last_out_min1 = last_out;
		last_out = out_value;
		
			
		// Send to output
		pipe_put(out_pipe,out_value);

	}

	// Tidy up
	free(last_out_buffer);
	free(error_buffer);

	return 0;
}

// End of TRIGMOVEM module