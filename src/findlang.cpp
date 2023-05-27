#include <iostream>
#include <filesystem>
#include <string>
#include <getopt.h>
#include <ctime>

#include "copymodel.hpp"

using namespace std;

int main(int argc, char **argv)
{

  // Command line arguments
  string analysis_filename;       // text under analysis
  int k = 5;                      // default size of the sliding window
  float alpha = 0.1;              // default alpha value for probability
  float threshold = 0.5;          // default probability threshold

  int opt;
  while ((opt = getopt(argc, argv, "t:k:a:t:")) != -1)
  {
    switch (opt)
    {
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

  string folderPath = "./models";

  string best_estimated_model;
  float best_estimated_bits = -1;

  /* Execution time start */
  time_t exec_time = time(nullptr);

  // Iterate over each file in the directory
  for (const auto &entry : filesystem::directory_iterator(folderPath))
  {
    if (filesystem::is_regular_file(entry.path()))
    {
      string filePath = entry.path().string();

      /* Create a copy model based on the given representation file (Ri) */
      CopyModel cp(k, alpha, threshold);
      cp.create_model(filePath);

      /* Compress t (analysis file) using the representation file based copy model (Ri) and estimate the number of bits required to compress t. */
      float estimated_bits = cp.process_analysis_file(analysis_filename);
      cout << "The model '" << filePath << "' takes " << estimated_bits << " bits" << endl;

      if (best_estimated_bits < 0 || estimated_bits < best_estimated_bits) {
        best_estimated_bits = estimated_bits;
        best_estimated_model = filePath;
      }
    }
  }

  cout << endl;

  /* Execution time end */
  exec_time = time(nullptr) - exec_time;

  cout << "exec time = " << exec_time << " seconds" << endl;
  cout << "best model: " << best_estimated_model << endl;

  return 0;
}
