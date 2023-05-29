#pragma once 

#include <iostream>
#include <stdlib.h>
#include <string>
#include <unordered_map>
#include <set>
#include <cmath>
#include <fstream>
#include <iostream>

using namespace std;

struct sequence_data
{
  int count;
  float probability;
};

class EncodingModel
{
private:
  /**
   *  params
   */
  int k;           // sliding window size

  /**
   *  structs
   */
  unordered_map<string, sequence_data> sequences_data;
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
  EncodingModel(int k)
  {
    /* Initialize all structure input parameters */
    this->k = k;
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

      next_character = tolower(next_character);

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

      sequences_data.at(window).count += 1;

      window = window.substr(1, k - 1);
    }

    int n_windows = pointer - k + 1;

    for (auto &pair : sequences_data)
    {
      pair.second.probability = (float)pair.second.count / n_windows;
    }
  }

  void export_model(string output_filename)
  {
    ofstream out;
    out.open(output_filename);
    if (!out.is_open())
    {
      cout << "[ERROR] Can't open file '" << output_filename << "'" << endl;
      exit(EXIT_FAILURE);
    }

    out << alphabet.size() << endl;
    out << k << endl;

    for (const auto &pair : sequences_data)
    {
      out << pair.first << ":" << pair.second.probability << endl;
    }

    out.close();
  }

  void import_model(string input_filename)
  {

    ifstream file;
    open_file(file, input_filename);

    string line;

    getline(file, line);

    for (size_t i = 0; i < stoi(line); i++)
    {
      alphabet.insert(i);
    }

    getline(file, line);
    k = stoi(line);

    while (getline(file, line))
    {
      sequences_data.insert({line.substr(0, k), {.probability = stof(line.substr(k + 1))}});
    }
  }

  /* Estimate the total number of bits of t (analysis file), using the computed model from Ri (representation file) */
  float process_analysis_file(string analysis_filename, int max_bits = -1)
  {

    /* Open file pointer (or exit if there's an error) */
    ifstream file;
    int file_length = this->open_file(file, analysis_filename);

    string window;       // Vector that stores all the file characters, in order to retrieve the next character after a sequence, given the position
    char next_character; // Next character (real one, for comparison)
    int pointer = 0;

    float total_bits;

    while (pointer < file_length)
    {
      char last_ch = next_character;

      file.get(next_character);        // Get the unread character from the file
      file.seekg(++pointer, ios::beg); // Increments pointer for next iteration (sliding-window)

      next_character = tolower(next_character);

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

      float probability;

      if (this->sequences_data.find(window) != this->sequences_data.end())
      {
        probability = this->sequences_data.at(window).probability;
      }
      else
      {
        probability = 1 / (100 * alphabet.size());
      }

      float information = -log2(probability);

      total_bits += information;

      window = window.substr(1, k - 1);
    }

    return total_bits;
  }
};