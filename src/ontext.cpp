/*!
 * \file       ontext.cpp
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
#include <memory>
#include <numeric>


/*!
 * \class CoOccurrenceMatrix
 * \brief Main structure for the OntExt algorithm
 */
class CoOccurrenceMatrix {
private:
   size_t n;
   std::vector<std::vector<double> > matrix;
   std::vector<std::string> features;

public:

   /*!
    * \fn CoOccurrenceMatrix(size_t size)
    * \param size Size of the matrix
    * \brief Creates a size X size co-occurrence matrix
    */
   CoOccurrenceMatrix(size_t size) :
      n(size),
      matrix(size, std::vector<double>(size)),
      features(size) { }

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

   /*!
    * \fn void normalize()
    * \brief Normalizes the matrix
    * The normalization is per row, that is, the sum of each row will be
    * always equals 1. Note the main diagonal is always at least 1,
    * so there won't be null rows.
    */
   void normalize() {
      #pragma omp parallel for
      for (size_t i = 0; i < n; ++i ){
         double rowsum = std::accumulate(matrix[i].begin(), matrix[i].end(), 0.0);
         for (size_t j = 0; j < n; ++j) {
            matrix[i][j] /= rowsum;
         }
      }
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


/*!
 * \class OntExt
 * \brief Implementation of the OntExt algorithm
 */
class OntExt {
private:
   template<class TKey, class TValue, class THash=std::hash<TKey> >
   using map = std::unordered_map<TKey, TValue, THash>;

   using categoryPair = std::pair<std::string, std::string>;
   using context = std::pair<std::string, std::string>;
   using counter = map<std::string, unsigned int>;
   using contextCounter = map<context, counter*, hashpair>;
   using occurrences = map<categoryPair, contextCounter*, hashpair>;


   map<std::string, std::unordered_set<std::string>* > instances;
   std::vector<categoryPair> categoryPairs;
   occurrences coOccurrences;

   std::string categoriesFilename;
   std::string instancesDirectoryName;
   std::string svoFilename;

   /*!
   * \fn void readCategoriesFile()
   * \brief Reads the category file (filename) into the global structures
   *
   * The global structures change as follows:
   *    1. categoryPairs will have all the pairs in the category pairs file
   *    2. coOccurrences will point each pair to an empty counter
   *    3. instances will map each category to a list of seeds/instances
   */
   void readCategoriesFile() {
      std::ifstream catstream(categoriesFilename);
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
            std::ifstream instancestream(instancesDirectoryName + category1);
            std::string seed;
            while (instancestream >> seed) {
               newcatset->insert(seed);
            }
         }

         if (instances.count(category2) == 0) {
            auto newcatset = new std::unordered_set<std::string>;
            newcatset->reserve(8192);
            instances[category2] = newcatset;
            std::ifstream instancestream(instancesDirectoryName + category2);
            std::string seed;
            while (instancestream >> seed) {
               newcatset->insert(seed);
            }
         }
      }
   }


   /*!
   * \fn void readSvoFile()
   * \brief Reads the SVO file (filename) into global coOccurrences
   *
   * The coOccurrences mapping will be complete, meaning the full indexing
   * of it will return the count of those objects.
   */
   void readSvoFile() {
      std::ifstream svostream(svoFilename);

      struct svorow {
         std::string s;
         std::string v;
         std::string o;
         std::string nstr;
      };

      std::vector<svorow*> rows;
      rows.reserve((size_t)1 << 25);
      while (svostream.peek() != std::char_traits<char>::eof()) {
         auto r = new svorow;
         std::getline(svostream, r->s, '\t');
         std::getline(svostream, r->v, '\t');
         std::getline(svostream, r->o, '\t');
         std::getline(svostream, r->nstr);
         rows.push_back(r);
      }
      svostream.close();
      
      #ifndef INNER_PARALLEL
      #define INNER_PARALLEL false
      #endif

      #pragma omp parallel for
      for (size_t it1 = 0; it1 < rows.size(); ++it1) {
         auto &row = *rows[it1];
         size_t size = categoryPairs.size();
         #pragma omp parallel for if(INNER_PARALLEL)
         for (size_t it = 0; it < size; ++it) {
            auto &pair = categoryPairs[it];
            if (instances[pair.first]->count(row.s) > 0
                  && instances[pair.second]->count(row.o) > 0) {
               int count = std::stoi(row.nstr);
               context so = std::make_pair(row.s, row.o);
               contextCounter* c = coOccurrences[pair];
               #pragma omp critical (foundpair)
               {
                  if (c->count(so) == 0) {
                     (*c)[so] = new counter;
                  }
                  (*((*c)[so]))[row.v] += count;
               }
            } else if (instances[pair.second]->count(row.s) > 0
                  && instances[pair.first]->count(row.o) > 0) {
               int count = std::stoi(row.nstr);
               context so = std::make_pair(row.s, row.o);
               contextCounter* c = coOccurrences[pair];
               #pragma omp critical (foundpair)
               {
                  if (c->count(so) == 0) {
                     (*c)[so] = new counter;
                  }
                  (*((*c)[so]))[row.v] += count;
               }
            }
         }
      }

      for(size_t it = 0; it < rows.size(); ++it) {
         delete rows[it];
      }

   }


   /*!
   * \fn std::vector<CoOccurrenceMatrix> buildMatrices()
   * \brief Builds the co-occurrence matrices
   */
   std::vector<CoOccurrenceMatrix> buildMatrices() {
      size_t size = categoryPairs.size();
      std::vector<CoOccurrenceMatrix> vec(size, CoOccurrenceMatrix(0));

      #pragma omp parallel for
      for (size_t it = 0; it < size; ++it) {
         const auto &catpair = categoryPairs[it];
         contextCounter* ccounter = coOccurrences[catpair];
         std::set<std::string> foundContexts;
         std::unordered_map<context, unsigned int, hashpair> cooccurring;

         for (const std::pair<context, counter*> &counters : (*ccounter)) {
            for (const std::pair<std::string, unsigned int> &ctx1 : *(counters.second)) {
               foundContexts.insert(ctx1.first);
               for (const std::pair<std::string, unsigned int> &ctx2 : *(counters.second)) {
                  cooccurring[std::make_pair(ctx1.first, ctx2.first)] += 1;
               }
            }
         }

         size_t n = foundContexts.size();
         CoOccurrenceMatrix m(n);

         size_t i, j;
         i = 0;
         for (auto ctx1 : foundContexts) {
            m.setName(i, ctx1);
            j = 0;
            for (auto ctx2 : foundContexts) {
               m.setValue(i, j, cooccurring[std::make_pair(ctx1, ctx2)]);
               ++j;
            }
            ++i;
         }
         m.normalize();
         vec[it] = m;
      }
      return vec;
   }

public:

   /*!
    * \fn OntExt(std::string categoriesFile, std::string instancesDirectory,
    * std::string svoFile)
    * \brief Set-up an OntExt execution with the given inputs
    */
   OntExt(std::string categoriesFile,
         std::string instancesDirectory,
         std::string svoFile) :
      categoriesFilename(categoriesFile),
      instancesDirectoryName(instancesDirectory),
      svoFilename(svoFile) {

      categoryPairs.reserve(256);
   }


   void run() {
      readCategoriesFile();
      readSvoFile();
      auto matrices = buildMatrices();

      // TODO: instead of printing the matrix,
      // should call KMeans on each matrix and output the relations
      for (size_t i = 0; i < categoryPairs.size(); ++i) {
         for (size_t j = 0; j < matrices[i].getN(); ++j) {
            std::cout << matrices[i].getName(j) << '\t';
         }
         std::cout << '\n';
         matrices[i].print();
         std::cout << '\n';
      }
   }


};


/*!
 * \fn int main(int argc, char** argv)
 * \brief Ontext entry point
 *
 * argv[1]: Categories file
 * argv[2]: Instances directory
 * argv[3]: SVO file
 */
int main (int argc, char** argv) {
   std::string categoryPairsFilename(argv[1]);
   std::string instanceDir(argv[2]);
   std::string svoFilename(argv[3]);

   OntExt algo(categoryPairsFilename, instanceDir + "/", svoFilename);
   algo.run();
}
