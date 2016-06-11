/*!
 * \file       prontext2.cpp
 * \brief      OntExt implementation linked to Prophet
 * \author     Diorge Brognara
 * \date       2016
 * \copyright  Copyright (C) Diorge Brognara 2016. All rights MIT Licensed.
 */


#include <iostream>
#include <fstream>
#include <unordered_set>
#include <unordered_map>
#include <string>
#include <utility>
#include <vector>
#include <set>


/*!
 * \class CoOccurrenceMatrix
 * \brief Main structure for the OntExt algorithm
 */
class CoOccurrenceMatrix {
private:
   double** matrix;
   std::string* features;
   size_t n;

public:

   /*!
    * \fn CoOccurrenceMatrix(size_t size)
    * \param size Size of the matrix
    * \brief Creates a size X size co-occurrence matrix
    */
   CoOccurrenceMatrix(size_t size) : n(size) {
      matrix = new double*[size];
      features = new std::string[size];
      for (size_t i = 0; i < n; i++) {
         matrix[i] = new double[size];
      }
   }

   ~CoOccurrenceMatrix() {
      for(size_t i = 0; i < n; i++) {
         delete[] matrix[i];
      }
      delete[] matrix;
      delete[] features;
   }

   /*!
    * \fn size_t getN() const
    * \brief Gets the size of the matrix
    */
   size_t getN() const {
      return n;
   }

   /*!
    * \fn void setValue(size_t row, size_t column, double value)
    * \brief Sets the matrix value at [row, column], indexing 0-based
    */
   void setValue(const size_t row, const size_t column, const double value) {
      matrix[row][column] = value;
   }

   /*!
    * \fn double getValue(size_t row, size_t column) const
    * \brief Gets the matrix value at [row, column], indexing 0-based
    */
   double getValue(size_t row, size_t column) const {
      return matrix[row][column];
   }

   /*!
    * \fn void setName(size_t order, std::string value)
    * \brief Sets the feature name at position order
    */
   void setName(size_t order, std::string value) {
      features[order] = value;
   }

   /*!
    * \fn std::string getName(size_t order) const
    * \brief Gets the feature name at position order
    */
   std::string getName(size_t order) const {
      return features[order];
   }

   /*!
    * \fn void print()
    * \brief Prints the matrix - debug only
    */
   void print() {
      for (size_t y = 0; y < getN(); ++y) {
         for (size_t x = 0; x < getN(); ++x) {
            std::cout << getValue(y, x) << '\t';
         }
         std::cout << '\n';
      }
      std::cout << '\n';
   }
};


/*!
 * \struct hashpair
 * \brief Provides a hash-function for std::pair as its operator()
 * Follows Python's implementation for hashing tuples
 */
struct hashpair {
   /*!
    * \fn operator() (const std::pair<T1, T2> &p) const
    * \brief Hash implementation for std::pair, mirroring Python's tuples
    * \tparam T1 First type of the pair
    * \tparam T2 Second type of the pair
    * \param p Pair to be hashed
    */
   template <class T1, class T2>
   std::size_t operator () (const std::pair<T1, T2> &p) const {
      auto value = 0x345678;
      auto h1 = std::hash<T1>{}(p.first);
      auto h2 = std::hash<T2>{}(p.second);
      value = (100003 * value) ^ h1;
      value = (100003 * value) ^ h2;
      return value;
   }
};


using categoryPair = std::pair<std::string, std::string>;
using context = std::pair<std::string, std::string>;
using counter = std::unordered_map<std::string, unsigned int>;
using contextCounter = std::unordered_map<context, counter*, hashpair>;
using occurrences = std::unordered_map<categoryPair, contextCounter*, hashpair>;


/*
 * Global structures
 */
std::unordered_map<std::string, std::unordered_set<std::string>* > instances;
std::vector<categoryPair> categoryPairs;
occurrences coOccurrences;


/*!
 * \fn void readCategoriesFile(const std::string &filename,
 * const std::string &instanceDir)
 * \brief Reads the category file (filename) into the global structures
 * \param filename The category pairs filename
 * \param instanceDir The path to the instances directory
 *
 * The global structures change as follows:
 *    1. categoryPairs will have all the pairs in the category pairs file
 *    2. coOccurrences will point each pair to an empty counter
 *    3. instances will map each category to a list of seeds/instances
 */
void readCategoriesFile(const std::string &filename,
                        const std::string &instanceDir) {
   std::ifstream catstream(filename);
   std::string category1;
   std::string category2;
   int categoryid;

   while (catstream >> category1 >> category2 >> categoryid) {
      auto pair = std::make_pair(category1, category2);
      categoryPairs.push_back(pair);
      coOccurrences[pair] = new contextCounter;

      if (instances.count(category1) == 0) {
         auto newcatset = new std::unordered_set<std::string>;
         newcatset->reserve(8192);
         instances[category1] = newcatset;
         std::ifstream instancestream(instanceDir + category1);
         std::string seed;
         while (instancestream >> seed) {
            newcatset->insert(seed);
         }
      }

      if (instances.count(category2) == 0) {
         auto newcatset = new std::unordered_set<std::string>;
         newcatset->reserve(8192);
         instances[category2] = newcatset;
         std::ifstream instancestream(instanceDir + category2);
         std::string seed;
         while (instancestream >> seed) {
            newcatset->insert(seed);
         }
      }
   }
}


/*!
 * \fn void readSvoFile(const std::string &filename)
 * \brief Reads the SVO file (filename) into global coOccurrences
 * \param filename Path to SVO file
 *
 * The coOccurrences mapping will be complete, meaning the full indexing
 * of it will return the count of those objects.
 */
void readSvoFile(const std::string &filename) {
   std::ifstream svostream(filename);
   std::string subject;
   std::string verbalphrase;
   std::string object;
   std::string countstr;
   int count;

   while (svostream.peek() != std::char_traits<char>::eof()) {
      std::getline(svostream, subject, '\t');
      std::getline(svostream, verbalphrase, '\t');
      std::getline(svostream, object, '\t');
      std::getline(svostream, countstr);

      for (const auto &pair : categoryPairs) {
         if (instances[pair.first]->count(subject) > 0
               && instances[pair.second]->count(object) > 0) {
            count = std::stoi(countstr);
            context so = std::make_pair(subject, object);
            contextCounter* c = coOccurrences[pair];
            if (c->count(so) == 0) {
               (*c)[so] = new counter;
            }
            (*((*c)[so]))[verbalphrase] += count;
         } else if (instances[pair.second]->count(subject) > 0
               && instances[pair.first]->count(object) > 0) {
            count = std::stoi(countstr);
            context so = std::make_pair(subject, object);
            contextCounter* c = coOccurrences[pair];
            if (c->count(so) == 0) {
               (*c)[so] = new counter;
            }
            (*((*c)[so]))[verbalphrase] += count;
         }
      }
   }
}


/*!
 * \fn void buildMatrices(std::vector<CoOccurrenceMatrix*>* matrices)
 * \brief Builds the co-occurrence matrices
 * \param matrices Vector with the matrices stored
 */
void buildMatrices(std::vector<CoOccurrenceMatrix*> &matrices) {
   matrices.reserve(categoryPairs.size());

   for (const categoryPair &catpair : categoryPairs) {
      contextCounter* ccounter = coOccurrences[catpair];
      std::set<std::string> foundContexts;
      std::unordered_map<context, unsigned int, hashpair> cooccurring;

      for (const std::pair<context, counter*> &counters : (*ccounter)) {
         foundContexts.insert(counters.first.first);
         foundContexts.insert(counters.first.second);
         cooccurring[counters.first] += 1;
      }

      size_t n = foundContexts.size();
      CoOccurrenceMatrix* m = new CoOccurrenceMatrix(n);

      size_t i, j;
      i = 0;
      for (auto ctx1 : foundContexts) {
         m->setName(i, ctx1);
         j = 0;
         for (auto ctx2 : foundContexts) {
            m->setValue(i, j, cooccurring[std::make_pair(ctx1, ctx2)]);
            ++j;
         }
         ++i;
      }
      matrices.push_back(m);
   }
}


/*!
 * \fn int main(int argc, char** argv)
 * \brief Ontext entry point
 *
 * argv[1]: Categories file
 * argv[2]: Instances directory
 * argv[3]: SVO file
 */
int main (int argc, char** argv) {
   /*
    * arguments
    */
   std::string categoryPairsFilename(argv[1]);
   std::string instanceDir(argv[2]);
   std::string svoFilename(argv[3]);

   categoryPairs.reserve(256);
   instanceDir += "/";

   readCategoriesFile(categoryPairsFilename, instanceDir);
   readSvoFile(svoFilename);
   
   std::vector<CoOccurrenceMatrix*> matrices;
   buildMatrices(matrices);

   // TODO: instead of printing the matrix,
   // should call KMeans on each matrix and output the relations
   for (auto m : matrices) {
      m->print();
      std::cout << '\n';
   }

}
