/*
 * fMRI_Stats - A program to compute common statistics on an fMRI series per voxel.
 *
 * This program is designed to compute statistics (mean, standard deviation, etc.)
 * for fMRI data. It processes the data on a voxel-by-voxel basis and outputs
 * the computed statistics based on user-defined parameters. The program can handle
 * multi-dimensional fMRI datasets and outputs the results in various data types.
 *
 * The input and output data types and statistical operations are user-defined.
 * It is expected to handle 4D data (x, y, z, t), with the ability to calculate
 * voxel-based statistics across time and space.
 *
 * Usage:
 * fMRI_Stats Num_of_Dims size_Dim1 ... size_DimN -Idata_type type -stat_type type
 *            -Odata_type type -NumberFiles N -input fMRI_Data_In1.img ... fMRI_Data_InN.img
 *            -use4d x y z t -use1d l -use3d x y z -output fMRI_Stats_Out.img ... fMRI_Stats_OutN.img
 *
 * Example usage:
 * %> fMRI_Stats 4 64 64 32 125 -Idata_type 3 -Odata_type 4 -NumberFiles 1 -input test.img -use4d 1 1 1 1 -output out.img
 *
 * Compilation:
 * cc -g -o fMRI_Stats fMRI_Stats.c -lm
 * g++ -g -o fmri fmri2.cpp -lm
 *
 * Input Data Types:
 *   Idata_type = 1 ==> 1 BYTE unsigned char data
 *   Idata_type = 2 ==> 2 SHORT int SIGNED data
 *   Idata_type = 3 ==> 3 SHORT int UNSIGNED data
 *   Idata_type = 4 ==> 4 BYTE floating-point data
 *
 * Output Data Types:
 *   Odata_type = 1 ==> 1 BYTE unsigned char data
 *   Odata_type = 2 ==> 2 SHORT int SIGNED data
 *   Odata_type = 3 ==> 3 SHORT int UNSIGNED data
 *   Odata_type = 4 ==> 4 BYTE floating-point data
 *
 * Statistical Types:
 *   stat_type = 1 ==> Voxel-based MEAN
 *   stat_type = 2 ==> Voxel-based STANDARD DEVIATION
 *   stat_type = 3 ==> Intensity values by location and time (time series data)
 *
 * Author: [Your Name]
 * Date: [Date]
 */

#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/uio.h>
#include <fcntl.h>
#include <unistd.h>
#include <math.h>
#include <string.h>
#include <stdlib.h>

#define NameLength 2000

// Global variables to hold the data types, dimensions, etc.
int Idata_type = 1, Odata_type = 1, stat_type = 1, Num_of_Dims, *size_Dim, NumberVoxels, NumberFiles;
unsigned char **Idata_u, **Odata_u, temp_u;
short int **Idata_ss, **Odata_ss, temp_ss;
unsigned short int **Idata_su, **Odata_su, temp_su;
float **Idata_f, **Odata_f, temp_f;

int xx, yy, zz, tt, num, l, ll, print_type;
float f_mean = 0, f_var = 0, f_stats = 0, temp_flt = 0, abs_mean = 0, intensity = 0;

// Template function to compute the mean value of fMRI data for a given voxel
template <class T>
float getMean(T a, int NumberVoxels, int *size_Dim, int Num_of_Dims, int z) {
    int counter = 0;
    float f_mean = 0;
    f_mean = 0;
    counter = 0;
    if (a[z] != 0)
        for (int t = 0; t < size_Dim[Num_of_Dims-1]; t++) {
            f_mean += (float)(a[z + t * NumberVoxels]);  // Summing intensity over time for a given voxel
            counter++;
        }
    if (counter < 2)
        f_mean = 0;  // Avoid computing mean for a single value
    else
        f_mean = f_mean / counter;  // Calculate the mean
    return (f_mean);
}

// Template function to compute the standard deviation for a given voxel
template <class T>
float getSD(T a, int NumberVoxels, int *size_Dim, int Num_of_Dims, int z) {
    int counter = 0;
    float f_mean = 0;
    float sd = 0;
    f_mean = 0;
    counter = 0;
    if (a[z] != 0)
        for (int t = 0; t < size_Dim[Num_of_Dims-1]; t++) {
            f_mean += (float)(a[z + t * NumberVoxels]);  // Summing intensity over time for a given voxel
            counter++;
        }
    if (counter < 2)
        f_mean = 0;  // Avoid computing mean for a single value
    else
        f_mean = f_mean / counter;  // Calculate the mean

    // Calculate standard deviation
    for (int t = 0; t < size_Dim[Num_of_Dims-1]; t++) {
        sd += (a[z + t * NumberVoxels] - f_mean) * (a[z + t * NumberVoxels] - f_mean);
    }
    sd = (float)(sqrt(sd / counter));  // Standard deviation formula
    return (sd);
}

// Function to print time series data for a single voxel
template <class T>
void printPartialTS(int xx, int yy, int zz, int tt, int ll, T a) {
    printf("l\tx\ty\tz\ttime\tintensity\n");
    printf("%d\t%d\t%d\t%d\t%d\t%d\n", ll, xx, yy, zz, tt, (int)(a[ll]));
}

// Function to print the time series for a voxel across all time points
template <class T>
void printTS(int xx, int yy, int zz, int tmax, int ll, T a) {
    printf("l\tx\ty\tz\ttime\tintensity\n");
    for (int k = 0; k < tmax; k++) {
        printf("%d\t%d\t%d\t%d\t%d\t%d\n", ll, xx, yy, zz, k, (int)(a[ll]));
    }
}

// Main function to execute the code
int main(int argc, char *argv[]) {
    FILE **fp_in, **fp_out;
    int i, z, t, counter = 0;
    
    // Check for proper usage of arguments
    if (argc < 13) {
        printf("USAGE: fMRI_Stats Num_of_Dims size_Dim1 ... size_DimN -Idata_type type -stat_type type -Odata_type type -NumberFiles N -input fMRI_Data_In1.img ... fMRI_Data_InN.img -use4d x y z t -use1d l -use3d x y z -output fMRI_Stats_Out.img ... fMRI_Stats_OutN.img\n");
        exit(0);
    }

    // Parse command-line arguments for dimensions and file types
    Num_of_Dims = atoi(argv[1]);
    NumberVoxels = 1;
    if ((size_Dim = (int *) malloc(Num_of_Dims * sizeof(int))) == NULL) {
        printf("Error in Mem_allocating Dimension_Size (size_Dim). \n");
        exit(0);
    }

    for (z = 0; z < Num_of_Dims; z++) {
        size_Dim[z] = atoi(argv[2 + z]);  // Set the size for each dimension
    }

    int xmax = size_Dim[0];
    int ymax = size_Dim[1];
    int zmax = size_Dim[2];
    int tmax = size_Dim[3];
    NumberVoxels = xmax * ymax * zmax;  // Calculate total number of voxels
    
    // More code to process data...
}


if (stat_type==1)		// Mean
{	/* Number voxels = x*y*z*t, , Num Dims = 3, size_dim = (x,y,z,t) z<(x*y*z)*/
	for(int kk = 0; kk<NumberFiles; kk++)
	{	
		for (z = 0; z < NumberVoxels; z++)
		{	
			if (Idata_type ==1 && Idata_u[z][kk] != 0)
			{	Odata_f[z][kk] = getMean(Idata_u[kk], NumberVoxels, size_Dim, Num_of_Dims, z); }
			else if (Idata_type ==2 && Idata_ss[z][kk] != 0)
			{	Odata_f[z][kk] = getMean(Idata_ss[kk], NumberVoxels, size_Dim, Num_of_Dims, z);}
			else if (Idata_type ==3 && Idata_su[z][kk] != 0)
			{	Odata_f[z][kk] = getMean(Idata_su[kk], NumberVoxels, size_Dim, Num_of_Dims, z);	}
			else if (Idata_type ==4 && Idata_f[z][kk] != 0)
			{	Odata_f[z][kk] = getMean(Idata_f[kk], NumberVoxels, size_Dim,  Num_of_Dims, z);	}
		}	
	}
}   //END::  if (stat_type==1)  // Mean

else if (stat_type==2)  // SD
{	for(int kk = 0; kk<NumberFiles; kk++)
	{	for (z = 0; z < NumberVoxels/size_Dim[Num_of_Dims-1]; z++)
		{	if (Idata_type ==1 && Idata_u[z][kk] != 0)
			{	Odata_f[z][kk] = getSD(Idata_u[kk], NumberVoxels, size_Dim, Num_of_Dims, z); }
			else if (Idata_type ==2 && Idata_ss[z][kk] != 0)
			{	Odata_f[z][kk] = getSD(Idata_ss[kk], NumberVoxels, size_Dim, Num_of_Dims, z);	}
			else if (Idata_type ==3 && Idata_su[z][kk] != 0)
			{	Odata_f[z][kk] = getSD(Idata_su[kk], NumberVoxels, size_Dim, Num_of_Dims, z);	}
			else if (Idata_type ==4 && Idata_f[z][kk] != 0)
			{	Odata_f[z][kk] = getSD(Idata_f[kk], NumberVoxels, size_Dim,  Num_of_Dims, z);	}
		}	
	}	
}  // END:: if (stat_type==2)  // SD

else if (stat_type==3)
{	if (print_type == 1)	
	{	for(int kk = 0; kk<NumberFiles; kk++)
		{	if (Idata_type ==1)
			{	printPartialTS(xx, yy, zz, tt, ll, Idata_u[kk]);	 }
			else if (Idata_type ==2)
			{	printPartialTS(xx, yy, zz, tt, ll, Idata_ss[kk]);	}
			else if (Idata_type ==3)
			{	printPartialTS(xx, yy, zz, tt, ll, Idata_su[kk]);	}
			else if (Idata_type ==4)
			{	printPartialTS(xx, yy, zz, tt, ll, Idata_f[kk] );	}
		}	
	}	


	else if (print_type ==2)
	{	for(int kk = 0; kk<NumberFiles; kk++)
		{	if (Idata_type ==1)
			{	printTS(xx, yy, zz, tmax, ll, Idata_u[kk]);	 }
			else if (Idata_type ==2)
			{	printTS(xx, yy, zz, tmax, ll, Idata_ss[kk]);	}
			else if (Idata_type ==3)
			{	printTS(xx, yy, zz, tmax, ll, Idata_su[kk]);	}
			else if (Idata_type ==4)
			{	printTS(xx, yy, zz, tmax, ll, Idata_f[kk] );	}
			}	
	}
}


else 
{	 printf("\n.\n.\n.\n Statistical Analysis is not implemented yet!!!!!!\n\n\n");   }



// Save 3D volume out

for(int kk = 0; kk<NumberFiles; kk++)	
{	if (Odata_type == 1)
	{   for (z = 0; z < NumberVoxels/size_Dim[Num_of_Dims-1]; z++)
		Odata_u[z][kk] = (unsigned char)(Odata_f[z][kk]);
		fwrite (Odata_u[kk], NumberVoxels/size_Dim[Num_of_Dims-1]*sizeof(temp_u), 1 ,fp_out[kk]);
		fclose(fp_out[kk]);		}
	else if (Odata_type == 2)
	{     for (z = 0; z < NumberVoxels/size_Dim[Num_of_Dims-1]; z++)
		Odata_ss[z][kk] = (short int)(Odata_f[z][kk]);
		fwrite (Odata_ss[kk], NumberVoxels/size_Dim[Num_of_Dims-1]*sizeof(temp_ss), 1 ,fp_out[kk]);
		fclose(fp_out[kk]);		}
	else if (Odata_type == 3)
	{   for (z = 0; z < NumberVoxels/size_Dim[Num_of_Dims-1]; z++)
		Odata_su[z][kk] = (unsigned short int)(Odata_f[z][kk]);
		fwrite (Odata_su[kk], NumberVoxels/size_Dim[Num_of_Dims-1]*sizeof(temp_su), 1 ,fp_out[kk]);
		fclose(fp_out[kk]);	}
	else if (Odata_type == 4)
	{  	fwrite (Odata_f[kk], NumberVoxels/size_Dim[Num_of_Dims-1]*sizeof(temp_f), 1 ,fp_out[kk]);
		fclose(fp_out[kk]);	}
	}
	
printf("\n.\n.\n.\nDONE computing the 3D voxel-based statistics of the 4D fMRI volume! \n");
exit(0);
}
