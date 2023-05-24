#include <iostream>
#include <stdio.h>
#include <stdlib.h>
#include <fstream>
#include <string.h>
#include <map>
#include <iterator>
#include <random>
#include <vector>
#include <string>
#include <math.h>
#include <unordered_map>

using namespace std;

struct char_data_t
{
  int numHits;
  int numFails;
  float prob;
};

class CopyModel
{
private:
  /**
   *  params
   */
  string filename; // text source file name
  int k;           // sliding window size
  float alpha;     // smoothning factor
  float threshold; // minimum performance of the model
  int file_length; // length of the file

  /**
   *  structs
   */
  ifstream file;                                                                      // file stream
  unordered_map<string, unordered_map<char, struct char_data_t>> sequences_lookahead; // sequence indexed dictionary for char probabilities
  int pointer;                                                                        // file stream pointer
  float accuracy;                                                                     // current model prediction accuracy
  float n_bits;                                                                       // average num bits per symbol
  float expected_total_bits;                                                          // expected total bits of compressed file
  time_t exec_time;                                                                   // execution time

public:
  /*
   *	Instantiation
   *
   *	- Init structures
   *	- Open file
   *	- Calculate file length
   */
  CopyModel(string filename, int k, float alpha, float threshold)
  {

    /* Initialize all structure input parameters */
    this->filename = filename;
    this->k = k;
    this->alpha = alpha;
    this->threshold = threshold;
    this->pointer = 0;

    /* Open file pointer (or exit if there's an error) */
    this->file.open(filename);
    if (!this->file.is_open())
    {
      cout << "[ERROR] Can't open file '" << filename << "'" << endl;
      exit(EXIT_FAILURE);
    }

    /* Get file length */
    this->file.seekg(0, ios::end);
    this->file_length = this->file.tellg();
    this->file.seekg(0, ios::beg);
  }

  /**
   *	Gets the prediction based on the current sequence and the probability table
   *
   *	- Iterates map with key = sequence
   *	- Finds the struct with maximum probability
   *
   *	@param seq sequence to predict
   *	@return predicted char
   */
  char get_next_character_prediction(string seq)
  {
    float maxProb = 0;
    char next;

    for (auto const &p : this->sequences_lookahead[seq])
    {
      if (p.second.prob > maxProb)
      {
        maxProb = p.second.prob;
        next = p.first;
      }
    }

    return next;
  }

  /**
   *	Calculates probability of a char being next in a sequence based
   *	on previous hits and fails
   *
   *	@param seq sequence to calculate the probabilities
   *	@param ch next char in sequence
   *	@return new probability
   */
  float calculate_probability(string seq, char ch)
  {
    int hits = this->sequences_lookahead[seq][ch].numHits;
    int fails = this->sequences_lookahead[seq][ch].numFails;

    this->sequences_lookahead[seq][ch].prob = ((hits + this->alpha) / (hits + fails + 2 * this->alpha));
    return float(this->sequences_lookahead[seq][ch].prob);
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
  void start()
  {

    char ch;                       // Current char
    vector<char> full_sequence;    // Vector that stores all the file characters, in order to retrieve the next character after a sequence, given the position
    string seq;                    // Sequence (in a string format) from the sliding window
    char next_character;           // Next character (real one, for comparison)
    char next_character_prevision; // Next character (predicted one)
    int Nh = 0, Nf = 0;            // Number of hits and number of fails
    int cur_Nh = 0, cur_Nf = 0;    // Current number of hits and current number of fails

    // While the file is no fully processed
    while (!this->file.eof())
    {

      this->file.get(ch);          // Get the unread character from the file
      full_sequence.push_back(ch); // Push the character to the full sequence vector

      /* If the full sequence vector has, at least, k characters to form a sequence */
      if (full_sequence.size() >= k)
      {

        string seq(full_sequence.end() - k, full_sequence.end()); // Get the k-char sliding window / sequence

        if (!this->file.get(next_character))
        { // Can't construct a window (file has less than k chars)
          break;
        }

        if (this->sequences_lookahead.count(seq) < 1)
        { // Sequence not in probability table
          this->sequences_lookahead.insert({seq, {}});
        }
        else
        {
          next_character_prevision = get_next_character_prediction(seq); // Get character prevision and increment respective fail / hit counter
          if (next_character == next_character_prevision)
          {
            this->sequences_lookahead[seq][next_character_prevision].numHits += 1;
            cur_Nh++;
          }
          else
          {
            this->sequences_lookahead[seq][next_character_prevision].numFails += 1;
            cur_Nf++;
          }

          calculate_probability(seq, next_character_prevision); // after updating hits and fails, recalculate char probability
          this->accuracy = ((float)cur_Nh) / (cur_Nh + cur_Nf); // recalculate model accuracy

          if (this->accuracy < this->threshold)
          {               // if model below threshold
            Nh += cur_Nh; // reset params
            Nf += cur_Nf;
            cur_Nh = 0;
            cur_Nf = 0;
            this->sequences_lookahead.clear();
            ;
          }
        }
        if (this->sequences_lookahead[seq].count(next_character) < 1)
        {                                   // add newly seen character after sequence to table
          char_data_t charInit = {0, 0, 0}; // with 1/2 probability
          this->sequences_lookahead[seq][next_character] = charInit;
          calculate_probability(seq, next_character);
        }
      }

      this->file.seekg(++this->pointer, ios::beg); // Increments pointer for next iteration (sliding-window)
    }

    /**
     *	Calculate model metrics
     */
    this->accuracy = ((float)Nh) / (Nh + Nf);
    this->n_bits = -log(this->accuracy) / log(2);
    this->expected_total_bits = this->n_bits * this->file_length;

    cout << "accuracy: " << this->accuracy << endl;
    cout << "n_bits: " << this->n_bits << endl;
    cout << "expected_total_bits: " << this->expected_total_bits << endl;
  }

  /* Estimate the total number of bits of t (analysis file), using the computed model from Ri (representation file) */
  float process_analysis_file(string analysis_filename)
  {

    /* Open file pointer (or exit if there's an error) */
    ifstream file;
    file.open(analysis_filename);
    if (!file.is_open())
    {
      cout << "[ERROR] Can't open analysis file '" << analysis_filename << "'" << endl;
      exit(EXIT_FAILURE);
    }

    char ch;                       // Current char
    vector<char> sliding_window;   // Vector that stores all the file characters, in order to retrieve the next character after a sequence, given the position
    string seq;                    // Sequence (in a string format) from the sliding window
    char next_character;           // Next character (real one, for comparison)
    char next_character_prevision; // Next character (predicted one)
    int Nh = 0, Nf = 0;            // Number of hits and number of fails
    pointer = 0;

    while (!file.eof())
    {

      file.get(ch);                 // Get the unread character from the file
      sliding_window.push_back(ch); // Push the character to the sliding window vector

      if (sliding_window.size() == k)
      {

        string seq(sliding_window.begin(), sliding_window.end());

        if (!file.get(next_character))
        { // Can't construct a window (file has less than k chars)
          break;
        }

        if (this->sequences_lookahead.count(seq) > 0)
        {

          next_character_prevision = get_next_character_prediction(seq); // Get character prevision and increment respective fail / hit counter
          if (next_character == next_character_prevision)
          {
            Nh++;
          }
          else
          {
            Nf++;
          }
        }

        sliding_window.erase(sliding_window.begin(), sliding_window.begin() + 1);
      }

      file.seekg(++pointer, ios::beg); // Increments pointer for next iteration (sliding-window)
    }

    /**
     *	Calculate metrics
     */
    accuracy = ((float)Nh) / (Nh + Nf);
    n_bits = -log(accuracy) / log(2);
    expected_total_bits = n_bits * file_length;

    cout << "accuracy: " << accuracy << endl;
    cout << "n_bits: " << n_bits << endl;
    cout << "expected_total_bits: " << expected_total_bits << endl;

    return expected_total_bits;
  }
};