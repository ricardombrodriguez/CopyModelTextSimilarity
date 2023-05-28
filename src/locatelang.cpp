#include <iostream>
#include <filesystem>
#include <string>
#include <getopt.h>
#include <ctime>
#include <vector>

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
      cerr << "Usage: " << argv[0] << " -t <analysis_filename> -k <window_size> -a <alpha> -p <threshold>\n";
      return 1;
    }
  }

  string folderPath = "./models";

  cout << "Creating language models..." << endl;

  /* Used to store the CopyModel class pointers */
  CopyModel models[];
  int i = 0;

  // Iterate over each file in the directory
  for (const auto &entry : filesystem::directory_iterator(folderPath))
  {
    if (filesystem::is_regular_file(entry.path()))
    {
      string filePath = entry.path().string();

      /* Create a copy model based on the given representation file (Ri) */
      CopyModel cp(k, alpha, threshold);
      cp.create_model(filePath);
      models[i++] = cp;

    }
    
  }

  cout << "Language models created!" << endl;
  cout << "Starting locatelang..." << endl;

  /* Execution time start */
  time_t exec_time = time(nullptr);

  perform_locate_lang(analysis_filename, models);

  /* Execution time end */
  exec_time = time(nullptr) - exec_time;

  cout << "Locatelang is finished!" << endl;

  cout << "exec time = " << exec_time << " seconds" << endl;

  return 0;
}

void perform_locate_lang(string filePath, CopyModel models[]) {

  /* Open file pointer (or exit if there's an error) */
  ifstream file;
  int file_length = this->open_file(file, analysis_filename);

  string window;       // Vector that stores all the file characters, in order to retrieve the next character after a sequence, given the position
  char next_character; // Next character
  int pointer = 0;
  string current_model;    // String that stores the current model filename 
  vector<tuple<int,string> lang_segment_positions;    // Stores the position and the language of each segment

  string ignored_chars = {
      '.', ',', ';', '!', '?', '*', '(', ')', '[', ']', '/', '-', '"', ':', '\\', '\n', '\t',
      '1', '2', '3', '4', '5', '6', '7', '8', '9', '0',
  };

  float total_bits, entropy;

  while (pointer < file_length)
  {
    char last_ch = next_character;

    file.get(next_character);        // Get the unread character from the file
    file.seekg(++pointer, ios::beg); // Increments pointer for next iteration (sliding-window)

    if (ignored_chars.find(next_character) != string::npos || last_ch == ' ' && next_character == last_ch)
    {
      next_character = last_ch;
      continue;
    }

    window += next_character;

    if (window.size() < k)
    {
      continue;
    }

    string best_model_filename;
    float best_model_bits = INF;
    float total_bits;

    /* Check for the best model for the given window, according to the number of bits (least = better) */
    for (int i = 0; i < sizeof(models)/sizeof(*models); i++) { 

      /* Get the total number of bits for the window, given the model */
      total_bits = cp.process_segment(window);

      /* Update if the model is better*/
      if (total_bits < best_estimated_bits) {
        best_model_bits = total_bits;
        best_model_filename = cp.filename;
      }

    }

    /* Check if there was a change in the language model and register it */
    if (!current_model || best_model_filename != current_model) {
      current_model = best_model_filename;
      lang_segment_positions.push_back( tuple<int,string>(pointer,current_model) );
    }

    window = window.substr(1, k - 1);    

  }
}
