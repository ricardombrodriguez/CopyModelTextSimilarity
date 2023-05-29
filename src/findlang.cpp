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

  int opt;
  while ((opt = getopt(argc, argv, "t:")) != -1)
  {
    switch (opt)
    {
    case 't':
      analysis_filename = string(optarg);
      break;
    default:
      cerr << "Usage: " << argv[0] << " -t <analysis_filename>\n";
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
      CopyModel cp(0, 0, 0);
      cp.import_model(filePath);

      /* Compress t (analysis file) using the representation file based copy model (Ri) and estimate the number of bits required to compress t. */
      float estimated_bits = cp.process_analysis_file(analysis_filename, best_estimated_bits);

      if (estimated_bits >= 0)
      {
        cout << "The model '" << filePath << "' takes " << estimated_bits << " bits" << endl;

        if (best_estimated_bits < 0 || estimated_bits < best_estimated_bits)
        {
          best_estimated_bits = estimated_bits;
          best_estimated_model = filePath;
        }
      }
      else
      {
        cout << "The model '" << filePath << "' takes more than " << best_estimated_bits << " bits" << endl;
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
