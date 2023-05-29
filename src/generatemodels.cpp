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
  int k = 5;                // default size of the sliding window
  float alpha = 0.1;        // default alpha value for probability
  float threshold = 0.5;    // default probability threshold

  string filename;

  int opt;
  while ((opt = getopt(argc, argv, "k:a:p:r:")) != -1)
  {
    switch (opt)
    {
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
    case 'r':
      filename = string(optarg);
      break;
    default:
      cerr << "Usage: " << argv[0] << " -k <window_size> -a <alpha> -p <threshold>\n";
      return 1;
    }
  }

  string folderPath = "./sources";

  string outPath = "./models";

  /* Execution time start */
  time_t exec_time = time(nullptr);

  filesystem::create_directories(filesystem::path(outPath));

  // Iterate over each file in the directory
  for (const auto &entry : filesystem::directory_iterator(folderPath))
  {
    if (filesystem::is_regular_file(entry.path()))
    {
      string filePath = entry.path().string();

      if (!filename.empty() && filename != filePath) {
        continue;
      }

      /* Create a copy model based on the given representation file (Ri) */
      CopyModel cp(k, alpha, threshold);
      cp.create_model(filePath);

      cp.export_model(outPath + "/" + entry.path().filename().string());

      cout << "Model for the source '" << filePath << "' has been generated" << endl;
    }
  }

  cout << endl;

  /* Execution time end */
  exec_time = time(nullptr) - exec_time;

  cout << "exec time = " << exec_time << " seconds" << endl;

  return 0;
}