#include <iostream>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <string>
#include <getopt.h>

#include "copymodel.hpp"

using namespace std;

int main(int argc, char **argv)
{

	// Command line arguments
	string representation_filename; // text representing the class ri (for example, representing a certain language)
	string analysis_filename;				// text under analysis
	int k = 5;											// default size of the sliding window
	float alpha = 0.1;							// default alpha value for probability
	float threshold = 0.5;					// default probability threshold

	int opt;
	while ((opt = getopt(argc, argv, "t:k:a:t:r:")) != -1)
	{
		switch (opt)
		{
		case 'r':
			representation_filename = string(optarg);
			break;
		case 't':
			analysis_filename = string(optarg);
			break;
		case 'k':
			k = atoi(optarg);
			if (k < 1)
			{
				cout << "[ERROR] Sliding window size must be greater than zero.\n"
						 << endl;
				exit(EXIT_FAILURE);
			}
			break;
		case 'a':
			alpha = atof(optarg);
			if (alpha <= 0)
			{
				cout << "[ERROR] Alpha must be bigger than zero.\n"
						 << endl;
				exit(EXIT_FAILURE);
			}
			break;
		case 'p':
			threshold = atof(optarg);
			if (threshold < 0 || threshold > 1)
			{
				cout << "[ERROR] Probability threshold must be between 0 and 1.\n"
						 << endl;
				exit(EXIT_FAILURE);
			}
			break;
		default:
			cerr << "Usage: " << argv[0] << " -r <representation_filename> -t <analysis_filename> -k <window_size> -a <alpha> -p <threshold>\n";
			return 1;
		}
	}

	/* Execution time start */
	time_t exec_time = time(nullptr);

	/* Create a copy model based on the given representation file (Ri) */
	CopyModel cp(k, alpha, threshold);
	cp.create_model(representation_filename);

	cout << "Acabou o copy model" << endl;

	/* Compress t (analysis file) using the representation file based copy model (Ri) and estimate the number of bits required to compress t. */
	float estimated_bits = cp.process_analysis_file(analysis_filename);

	cout << "Estimated bits: " << estimated_bits << endl;

	/* Execution time end */
	exec_time = float(time(nullptr) - exec_time);

	cout << "exec time = " << exec_time << " seconds" << endl;

	return 0;
}