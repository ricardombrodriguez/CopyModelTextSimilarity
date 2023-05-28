#include <iostream>
#include <stdlib.h>
#include <string>
#include <unordered_map>
#include <set>
#include <list>
#include <cmath>
#include <fstream>
#include <iostream>

using namespace std;

struct sequence_possibilities
{
  list<int> pointers;
  float probability;
  int n_hits;
  int n_fails;
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
      '.', ',', ';', '!', '?', '*', '(', ')', '[', ']', '/', '-', '_', '"', ':', '\\', '\n', '\t',
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

      window = window.substr(1, k - 1);
    }

    pointer = 0;
    window = "";

    file.seekg(pointer, ios::beg);

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

      if (window.size() < k)
      {
        continue;
      }

      if (sequences_data.find(window) == sequences_data.end())
      {
        sequences_data.insert({window, {}});
        sequences_data.at(window).probability = (float)1 / alphabet.size();
      }

      sequence_possibilities *st = &sequences_data.at(window);

      if (!(*st).pointers.empty())
      {
        char predicted_character;
        file.seekg((*st).pointers.front(), ios::beg);
        file.get(predicted_character);
        file.seekg(pointer, ios::beg);

        if (predicted_character == next_character)
        {
          (*st).n_hits++;
        }
        else
        {
          (*st).n_fails++;
        }

        (*st).probability = ((*st).n_hits + alpha) / ((*st).n_hits + (*st).n_fails + alphabet.size() * alpha);

        if ((*st).probability < this->threshold)
        {
          (*st).n_hits = 0;
          (*st).n_fails = 0;
          (*st).pointers.pop_front();
        }
      }

      (*st).pointers.push_back(pointer);

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

      float information;

      if (this->sequences_data.find(window) != this->sequences_data.end())
      {
        information = -log2(this->sequences_data.at(window).probability);
      }
      else
      {
        information = log2(pow(k, alphabet.size()));
      }

      total_bits += information;

      window = window.substr(1, k - 1);
    }

    return total_bits;
  }
};