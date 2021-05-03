#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <time.h>
#include <math.h>
/* local includes */
#include "libssdp.h"
#include "io.h"
#include "util.h"
#include "variables.h"
#include "parser.h"
#include "parserutils.h"



/*
BEGIN_DESCRIPTION
PARSEFLAG sim_static SimStatic "C=<in-config> t=<in-array> GHI=<in-array> DHI=<in-array> POA=<out-array>"
DESCRIPTION Computes the POA irradiance for all configured locations.
ARGUMENT C Simulation config variable
ARGUMENT t unix time array.
ARGUMENT GHI global horizontal irradiance as a function of time
ARGUMENT DHI diffuse horizontal irradiance as a function of time
OUTPUT POA plane of array irradiance as a function of time and location (with n time values and m locations the array contains n times m values)
END_DESCRIPTION
*/
void SimStatic(char *in)
{
	int i, j; // loop through space and time
	char *word;
	simulation_config *C;
	array *t, *GH, *DH, out;
	clock_t tsky0, tpoa0;
	clock_t tsky=0, tpoa=0;
	double ttsky, ttpoa;
	int pco=0;
	word=malloc((strlen(in)+1)*sizeof(char));
	
	if (FetchConfig(in, "C", word, &C))
	{
		free(word);
		return;
	}		
	if (!C->sky_init)
	{		
		Warning("Simulation config has no sky initialized\n");
		free(word);
		return;
	}	
	if (!C->topo_init) 
	{	
		Warning("No topological data available, omitting horizon\n");
		InitConfigMaskNoH(C);
		if (ssdp_error_state)
		{
			ssdp_print_error_messages();
			ssdp_reset_errors();
			free(word);
			return;
		}
	}
	if (!C->loc_init) 
	{	
		Warning("Simulation config has no locations initialized\n");
		free(word);
		return;
	}		
	if (FetchArray(in, "t", word, &t))
	{
		free(word);
		return;
	}
	if (FetchArray(in, "GHI", word, &GH))
	{
		free(word);
		return;
	}
	if (FetchArray(in, "DHI", word, &DH))
	{
		free(word);
		return;
	}	
	if ((t->N!=GH->N)||(t->N!=DH->N))
	{
		Warning("Length of t-, GHI-, and DHI-arrays do not match\n");
		free(word);
		return;
	}
	// fetch name of output var
	if (!GetArg(in, "POA", word))
	{
		free(word);
		return;
	}
	out.D=malloc(t->N*C->Nl*sizeof(double));
	if (out.D==NULL)
	{
		free(word);
		return;
	}	
	out.N=t->N*C->Nl;	
	for (j=0;j<t->N;j++)
	{
		// compute sky at evert time instance
		tsky0=clock();
		ssdp_make_perez_all_weather_sky_coordinate(&(C->S), (time_t) t->D[j], C->lon, C->lat, GH->D[j], DH->D[j]);
		tsky+=clock()-tsky0;
		tpoa0=clock();
		for (i=0;i<C->Nl;i++)
			out.D[j*C->Nl+i]=ssdp_total_poa(&(C->S), C->o[i], &(C->M), C->L+i);
		pco=ProgressBar((100*((j+1)*C->Nl))/(t->N*C->Nl), pco, ProgressLen, ProgressTics);
		tpoa+=clock()-tpoa0;
	}
	ttsky=(double)tsky/CLOCKS_PER_SEC;
	ttpoa=(double)tpoa/CLOCKS_PER_SEC;
	printf("Computed %d skies in %g s (%g s/sky)\n", t->N, ttsky, ttsky/((double)t->N));
	printf("Computed %d POA Irradiances in %g s (%g s/POA)\n", t->N*C->Nl, ttpoa, ttpoa/((double)(t->N*C->Nl)));
	if(AddArray(word, out))
	{
		free(word); // failed to make array
		free(out.D);
	}	
}
/*
BEGIN_DESCRIPTION
PARSEFLAG sim_route SimRoute "C=<in-config> t=<in-array> GHI=<in-array> DHI=<in-array> POA=<out-array>"
DESCRIPTION Computes the POA irradiance along a route along all configured locations.
ARGUMENT C Simulation config variable
ARGUMENT t unix time array. As t progresses from t0-tn we move through locations l0-lm
ARGUMENT GHI global horizontal irradiance as a function of time
ARGUMENT DHI diffuse horizontal irradiance as a function of time
OUTPUT POA plane of array irradiance as a function of time
END_DESCRIPTION
*/
void SimRoute(char *in)
{
	int i, j; // loop through space and time
	char *word;
	simulation_config *C;
	array *t, *GH, *DH, out;
	clock_t tsky0, tpoa0;
	clock_t tsky=0, tpoa=0;
	double ttsky, ttpoa;
	int pco=0;
	word=malloc((strlen(in)+1)*sizeof(char));
	
	if (FetchConfig(in, "C", word, &C))
	{
		free(word);
		return;
	}		
	if (!C->sky_init)
	{		
		Warning("Simulation config has no sky initialized\n");
		free(word);
		return;
	}	
	if (!C->topo_init) 
	{	
		Warning("No topological data available\n");
		free(word);
		return;
	}
	if (!C->loc_init) 
	{	
		Warning("Simulation config has no locations initialized\n");
		free(word);
		return;
	}		
	if (FetchArray(in, "t", word, &t))
	{
		free(word);
		return;
	}
	if (FetchArray(in, "GHI", word, &GH))
	{
		free(word);
		return;
	}
	if (FetchArray(in, "DHI", word, &DH))
	{
		free(word);
		return;
	}	
	if ((t->N!=GH->N)||(t->N!=DH->N))
	{
		Warning("Length of t-, GHI-, and DHI-arrays do not match\n");
		free(word);
		return;
	}
	if (t->N<C->Nl)
		Warning("Warning: time array contains less points than there are waypoints\n");
	if (t->N>C->Nl)
		Warning("Warning: time array contains more points than there are waypoints\n");
	// fetch name of output var
	if (!GetArg(in, "POA", word))
	{
		free(word);
		return;
	}
	out.D=malloc(t->N*sizeof(double));
	if (out.D==NULL)
	{
		free(word);
		return;
	}	
	out.N=t->N;	
	for (j=0;j<t->N;j++)
	{
		// compute sky at evert time instance
		tsky0=clock();
		ssdp_make_perez_all_weather_sky_coordinate(&(C->S), (time_t) t->D[j], C->lon, C->lat, GH->D[j], DH->D[j]);
		tsky+=clock()-tsky0;
		tpoa0=clock();
		if (t->N>1)
			i=(j*(C->Nl-1))/(t->N-1);
		else
			i=0;
		out.D[j]=ssdp_total_poa(&(C->S), C->o[i], &(C->M), C->L+i);
		pco=ProgressBar((100*(j+1))/t->N, pco, ProgressLen, ProgressTics);
		tpoa+=clock()-tpoa0;
	}
	ttsky=(double)tsky/CLOCKS_PER_SEC;
	ttpoa=(double)tpoa/CLOCKS_PER_SEC;
	printf("Computed %d skies in %g s (%g s/sky)\n", t->N, ttsky, ttsky/((double)t->N));
	printf("Computed %d POA Irradiances in %g s (%g s/POA)\n", t->N, ttpoa, ttpoa/((double)(t->N)));
	if(AddArray(word, out))
	{
		free(word); // failed to make array
		free(out.D);
	}	
}

/*
BEGIN_DESCRIPTION
PARSEFLAG solpos SolarPos "t=<in-array> lon=<in-float> lat=<in-float> azimuth=<out-array> zenith=<out-array>"
DESCRIPTION Computes the solar position according to the PSA algorithm. See: Blanco-Muriel, Manuel, et al. "Computing the solar vector." Solar energy 70.5 (2001): 431-441
ARGUMENT t unix time array
ARGUMENT lon longitude float
ARGUMENT lat latitude float
OUTPUT azimuth sun azimuth (radians)
OUTPUT zenith sun zenith (radians)
END_DESCRIPTION
*/
void SolarPos(char *in)
{
	int i;
	char *word;
	sky_pos s;
	array *t, azi, zen;
	double lon, lat;
	word=malloc((strlen(in)+1)*sizeof(char));
	if (FetchArray(in, "t", word, &t))
	{
		free(word);
		return;
	}
	
	if (FetchFloat(in, "lon", word, &lon))
	{
		free(word);
		return;
	}
	lon=deg2rad(lon);
	if (FetchFloat(in, "lat", word, &lat))
	{
		free(word);
		return;
	}
	lat=deg2rad(lat);
	azi.D=malloc(t->N*sizeof(double));
	if (azi.D==NULL)
	{
		free(word);
		return;
	}	
	azi.N=t->N;
	zen.D=malloc(t->N*sizeof(double));
	if (zen.D==NULL)
	{
		free(word);
		return;
	}	
	zen.N=t->N;
	for (i=0;i<t->N;i++)
	{
		s=ssdp_sunpos((time_t)t->D[i], lat, lon);
		azi.D[i]=s.a;
		zen.D[i]=s.z;
	}
	if (!GetArg(in, "azimuth", word))
	{
		free(word);
		return;
	}
	if(AddArray(word, azi))
	{
		free(word); // failed to make array
		free(azi.D);
	}	
	word=malloc((strlen(in)+1)*sizeof(char));
	if (!GetArg(in, "zenith", word))
	{
		free(word);
		return;
	}
	if(AddArray(word, zen))
	{
		free(word); // failed to make array
		free(zen.D);
	}	
}

/*
BEGIN_DESCRIPTION
PARSEFLAG export_sky ExportSky "C=<in-config> t=<in-array> GHI=<in-array> DHI=<in-array> index=<in-int> file=<in-string>"
DESCRIPTION Exports a 3D polar plot of a sky. It only expoits one location. You need to specify the index of the location (starting at index 0).
ARGUMENT C config-variable
ARGUMENT t single value unix time
ARGUMENT GHI single value global horizontal irradiance
ARGUMENT DHI single value diffuse horizontal irradiance
ARGUMENT index index of the location
ARGUMENT file filename
OUTPUT file A file with sky intensities. Organized in 4 columns: x y z I[W/sr]
END_DESCRIPTION
*/
void ExportSky(char *in)
{
	int j; 
	char *word;
	simulation_config *C;
	array *t, *GH, *DH;
	word=malloc((strlen(in)+1)*sizeof(char));
	
	if (FetchConfig(in, "C", word, &C))
	{
		free(word);
		return;
	}		
	if (!C->sky_init)
	{		
		Warning("Simulation config has no sky initialized\n");
		free(word);
		return;
	}	
	if (!C->topo_init) 
	{	
		Warning("No topological data available, omitting horizon\n");
		InitConfigMaskNoH(C);
		if (ssdp_error_state)
		{
			ssdp_print_error_messages();
			ssdp_reset_errors();
			free(word);
			return;
		}
	}
	if (!C->loc_init) 
	{	
		Warning("Simulation config has no locations initialized\n");
		free(word);
		return;
	}		
	if (FetchArray(in, "t", word, &t))
	{
		free(word);
		return;
	}
	if (t->N!=1)
	{
		Warning("Length of t array must be 1\n");
		free(word);
		return;
	}	
	if (FetchArray(in, "GHI", word, &GH))
	{
		free(word);
		return;
	}
	if (GH->N!=1)
	{
		Warning("Length of GHI array must be 1\n");
		free(word);
		return;
	}
	if (FetchArray(in, "DHI", word, &DH))
	{
		free(word);
		return;
	}
	if (DH->N!=1)
	{
		Warning("Length of DHI array must be 1\n");
		free(word);
		return;
	}	
	if (FetchInt(in, "index", word, &j))
	{
		free(word);
		return;
	}		
	if (j<0)
	{
		Warning("index must be larger or equal to 0\n");
		free(word);
		return;
	}	
	if (j>=C->Nl)
	{
		Warning("index must be smaller than the number of locations (%d)\n", C->Nl);
		free(word);
		return;
	}	
	
	if (!GetArg(in, "file", word))
	{
		free(word);
		return;
	}
	// compute sky
	ssdp_make_perez_all_weather_sky_coordinate(&(C->S), (time_t) t->D[0], C->lon, C->lat, GH->D[0], DH->D[0]);
	WriteDome4D(word, &(C->S), C->L);
	free(word);
}

