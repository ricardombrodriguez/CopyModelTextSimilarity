#include <iostream>
#include <fstream>
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

void store_results(vector<tuple<int,string>> lang_segments, string analysis_filename)
{

  ifstream analysis_file;
  analysis_file.open(analysis_filename);
  if (!analysis_file.is_open())
  {
    cout << "[ERROR] Can't open file '" << analysis_filename << "'" << endl;
    exit(EXIT_FAILURE);
  }

  string output_filename = analysis_filename;
  size_t pos = output_filename.find(".txt");
  if (pos != string::npos) {
    output_filename = output_filename.substr(0, pos) + "_result.txt";
  }

  ofstream output_file;
  output_file.open(output_filename);

  if (!output_file.is_open()) {
    cout << "[ERROR] Can't write on file '" << output_filename << "'" << endl; 
    exit(EXIT_FAILURE);
  }


  int pointer = 0;
  int i = 0;
  char* segment;

  // Accessing keys and values
  for (const auto& segment_data : lang_segments) {

    int position = get<0>(segment_data);
    string model_name = get<1>(segment_data);
    int segment_size = position - pointer;

    /* Last segment */
    if (i == lang_segments.size() - 1) {
      /* Get file length */
      analysis_file.seekg(0, ios::end);
      int file_length = analysis_file.tellg();
      cout << "lenghthhhh " << file_length << endl;
      cout << "pointerrrrrrrrrrrrr " << pointer << endl;
      analysis_file.seekg(pointer, ios::beg);
      segment_size = file_length - pointer;
      cout << "ganssjwjsdw" << segment_size << endl;
    }

    cout << "model name " << model_name << endl;
    // cout << segment_size << position << endl;

    string segment_text;

    char ch;
    for (int i=0; i < segment_size; i++) {
      analysis_file.get(ch);
      segment_text += ch;
    }

    cout << segment_text << endl;

    output_file << "[ " << model_name << " ] Segment [" << pointer << "-" << position << "] -> " << segment_text << endl;

    /* Start of the next segment */
    pointer = position;
    // analysis_file.seekg(pointer, ios::end);

    i++;

  }

  analysis_file.close(); 
  output_file.close();

  cout << "File written successfully." << endl;

}

vector<tuple<int,string>> perform_locate_lang(string filePath, list<CopyModel> models, int num_models, int k)
{

  /* Open file pointer (or exit if there's an error) */
  ifstream file;
  int file_length = open_file(file, filePath);

  string window;       // Vector that stores all the file characters, in order to retrieve the next character after a sequence, given the position
  char next_character; // Next character
  int pointer = 0;
  string last_model_lang;
  string current_model_lang;
  vector<tuple<int,string>> lang_segment_positions;    // Stores the position and the language of each segment
  unordered_map<string, sequence_possibilities> model_map;

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

    /* Can't find sequence in current model, search the sequence in other models */
    if (model_map.find(window) == model_map.end())
    {

      if (!model_map.empty()) {
        last_model_lang = current_model_lang;
      }

      for (CopyModel model: models) {
    
        /* Get the total number of bits for the window, given the model */
        total_bits = model.process_segment(window);

        /* Update if the model is better*/
        if (total_bits < best_model_bits) {
          best_model_bits = total_bits;
          current_model_lang = model.filename;
          model_map = model.get_sequences_data();
        }
      }

      if (last_model_lang.empty()) {
        last_model_lang = current_model_lang;
      }

      /* Check if there was a change in the language model and register it */
      if (!model_map.empty() && (last_model_lang != current_model_lang) ) {
        lang_segment_positions.push_back( tuple<int,string>(pointer, last_model_lang) );
      }
      
    }

    cout << "Current model for sequence '" << window << "': " << current_model_lang << endl;

    window = window.substr(1, k - 1);
  }

  lang_segment_positions.push_back( tuple<int,string>(pointer, current_model_lang) );

  return lang_segment_positions;

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
      cp.filename = filePath;
      cout << cp.filename << endl;
    }
  }

  cout << "Language models created!" << endl;
  cout << "Starting locatelang..." << endl;

  /* Execution time start */
  time_t exec_time = time(nullptr);

  vector<tuple<int,string>> lang_segments = perform_locate_lang(analysis_filename, models, num_models, k);

  store_results(lang_segments, analysis_filename);

  /* Execution time end */
  exec_time = time(nullptr) - exec_time;

  cout << "Locatelang is finished!" << endl;

  cout << "exec time = " << exec_time << " seconds" << endl;

  return 0;
}