#include <iostream>
#include <stdlib.h>
#include <string>
#include <unordered_map>
#include <set>
#include <cmath>
#include <fstream>
#include <iostream>

using namespace std;

struct sequence_possibilities
{
  unordered_map<char, float> hashmap;
  set<int> pointers;
  float count;
  float probability;
};

class CopyModel
{
private:
  /**
   *  params
   */
  int k;           // sliding window size
  float alpha;     // smoothning factor
  float threshold; // minimum performance of the model

  /**
   *  structs
   */
  unordered_map<string, sequence_possibilities> sequences_data;
  set<char> alphabet; // file stream

  string ignored_chars = {
      '.', ',', ';', '!', '?', '*', '(', ')', '[', ']', '/', '-', '"', ':', '\\', '\n', '\t',
      '1', '2', '3', '4', '5', '6', '7', '8', '9', '0',
  };

public:
  /*
   *	Instantiation
   *
   *	- Init structures
   *	- Open file
   *	- Calculate file length
   */
  CopyModel(int k, float alpha, float threshold)
  {
    /* Initialize all structure input parameters */
    this->k = k;
    this->alpha = alpha;
    this->threshold = threshold;
  }

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

  /**
   *	Main model workflow
   *
   *	Get chars from the stream and build probability table to get prediction
   *
   *	- Create a sequence o k chars
   *	- Predict the next char based on probability table if sequence in table
   *	- Recalculate probabilities
   *	- Append new chars to table
   */
  void create_model(string filename)
  {

    string window;       // Sequence (in a string format) from the sliding window
    char next_character; // Next character (real one, for comparison)

    int pointer = 0;

    ifstream file;
    int file_length = this->open_file(file, filename);

    // While the file is no fully processed
    while (pointer < file_length)
    {
      char last_ch = next_character;

      file.get(next_character);
      file.seekg(++pointer, ios::beg); // Increments pointer for next iteration (sliding-window)

      if (ignored_chars.find(next_character) != string::npos || last_ch == ' ' && next_character == last_ch)
      {
        next_character = last_ch;
        continue;
      }

      window += next_character;

      alphabet.insert(next_character);

      if (window.size() < k)
      {
        continue;
      }

      if (sequences_data.find(window) == sequences_data.end())
      {
        sequences_data.insert({window, {}});
      }

      sequence_possibilities *st = &sequences_data.at(window);

      (*st).pointers.insert(pointer);

      if ((*st).hashmap.find(next_character) == (*st).hashmap.end())
      {
        (*st).hashmap.insert({next_character, alpha});
      }

      (*st).count += 1;
      (*st).hashmap.at(next_character) += 1;

      window = window.substr(1, k - 1);
    }
  }

  /* Estimate the total number of bits of t (analysis file), using the computed model from Ri (representation file) */
  float process_analysis_file(string analysis_filename)
  {

    /* Open file pointer (or exit if there's an error) */
    ifstream file;
    int file_length = this->open_file(file, analysis_filename);

    string window;       // Vector that stores all the file characters, in order to retrieve the next character after a sequence, given the position
    char next_character; // Next character (real one, for comparison)
    int pointer = 0;

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

      int count;

      float applied_alpha = alpha;

      if (this->sequences_data.find(window) != this->sequences_data.end() && this->sequences_data.find(window)->first == window)
      {
        count = this->sequences_data.at(window).count;
      }
      else
      {
        count = 0;
        applied_alpha = applied_alpha / 1000;
      }

      float probability = (count + applied_alpha) / (file_length - k + 1 + (pow(alphabet.size(), this->k)) * applied_alpha);
      float information = -log2(probability);

      entropy += probability * information;
      total_bits += information;

      window = window.substr(1, k - 1);
    }

    return total_bits;
  }
};