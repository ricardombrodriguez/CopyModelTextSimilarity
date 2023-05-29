#include <iostream>
#include <filesystem>
#include <string>
#include <getopt.h>
#include <ctime>
#include <vector>
#include <tuple>
#include <list>

#include "copymodel.hpp"

using namespace std;


int open_file(ifstream &file, string filename)
{
  /* Open file pointer (or exit if there's an error) */
  file.open(filename, ios::binary);
  if (!file.is_open())
  {
    cout << "[ERROR] Can't open file '" << filename << "'" << endl;
    exit(EXIT_FAILURE);
  }

  /* Get file length */
  file.seekg(0, ios::end);
  int file_length = file.tellg();
  file.seekg(0, ios::beg);

  return file_length;
}

void perform_locate_lang(string filePath, list<CopyModel> models, int num_models, int k)
{

  /* Open file pointer (or exit if there's an error) */
  ifstream file;
  int file_length = open_file(file, filePath);

  string window;       // Vector that stores all the file characters, in order to retrieve the next character after a sequence, given the position
  char next_character; // Next character
  int pointer = 0;
  string current_model;    // String that stores the current model filename 
  vector<tuple<int,string>> lang_segment_positions;    // Stores the position and the language of each segment

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
    float best_model_bits = 10e10;
    float total_bits;

    CopyModel *model = nullptr;

    for (CopyModel model: models) {
  
      /* Get the total number of bits for the window, given the model */
      total_bits = model.process_segment(window);

      /* Update if the model is better*/
      if (total_bits < best_model_bits) {
        best_model_bits = total_bits;
        best_model_filename = model.filename;
      }

      cout << "Current model for sequence '" << window << "': " << current_model << " with " << best_model_bits << " bits" << endl;
    }

    /* Check if there was a change in the language model and register it */
    if (current_model.empty() || best_model_filename != current_model) {
      current_model = best_model_filename;
      lang_segment_positions.push_back( tuple<int,string>(pointer,current_model) );
    }

    cout << "Current model for sequence '" << window << "': " << current_model << endl;

    window = window.substr(1, k - 1);
  }
}

int main(int argc, char **argv)
{

  // Command line arguments
  string analysis_filename;       // text under analysis
  int k = 5;                      // default size of the sliding window

  int opt;
  while ((opt = getopt(argc, argv, "t:")) != -1)
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
    default:
      cerr << "Usage: " << argv[0] << " -t <analysis_filename> -k <window_size> -a <alpha> -p <threshold>\n";
      return 1;
    }
  }

  string folderPath = "./models";

  cout << "Creating language models..." << endl;

  /* Used to store the CopyModel class pointers */
  list<CopyModel> models;
  int num_models = 0;
  // Iterate over each file in the directory
  for (const auto &entry : filesystem::directory_iterator(folderPath))
  {
    if (filesystem::is_regular_file(entry.path()))
    {
      string filePath = entry.path().string();

      /* Create a copy model based on the given representation file (Ri) */
      CopyModel cp(0, 0, 0);
      cp.import_model(filePath);
      models.push_back(cp);
    }
  }

  cout << "Language models created!" << endl;
  cout << "Starting locatelang..." << endl;

  /* Execution time start */
  time_t exec_time = time(nullptr);

  perform_locate_lang(analysis_filename, models, num_models, k);

  /* Execution time end */
  exec_time = time(nullptr) - exec_time;

  cout << "Locatelang is finished!" << endl;

  cout << "exec time = " << exec_time << " seconds" << endl;

  return 0;
}