#include <iostream>
#include <fstream>
#include <unordered_set>
#include <unordered_map>
#include <string>
#include <utility>
#include <vector>
#include <set>

/**
 * argv[1]: Categories file
 * argv[2]: Instances directory
 * argv[3]: SVO file
 */

struct CoOccurrenceMatrix {
public:
   double** matrix;
   std::string* features;
   unsigned int n;

   ~CoOccurrenceMatrix() {
      for(unsigned int i = 0; i < n; i++)
         delete[] matrix[i];
      delete[] matrix;
      delete[] features;
   }
};

void print_matrix(CoOccurrenceMatrix* m) {
   for (unsigned int i = 0; i < m->n; i++) {
      for (unsigned int j = 0; j < m->n; j++) {
         std::cout << (m->matrix)[i][j] << '\t';
      }
      std::cout << '\n';
   }
   std::cout << '\n';
}

struct hashpair {
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


int main (int argc, char** argv) {
   /*
    * arguments
    */
   std::string categoryPairsFilename(argv[1]);
   std::string instanceDir(argv[2]);
   std::string svoFilename(argv[3]);

   /*
    * "global" structures
    */
   std::unordered_map<std::string, std::unordered_set<std::string>* > instances;
   std::vector<categoryPair> categoryPairs;
   occurrences coOccurrences;

   categoryPairs.reserve(256);

   /*
    * Section: reading the categories file
    * Output:
    *    1. categoryPairs will have all the pairs in the file
    *    2. coOccurrences will point each pair to an empty counter
    *    3. instances map will point each category to a list of seeds/instances
    */
   std::ifstream categoriesFile(categoryPairsFilename);
   std::string category1;
   std::string category2;
   int categoryid;
   instanceDir += "/";

   while (categoriesFile >> category1 >> category2 >> categoryid) {
      auto pair = std::make_pair(category1, category2);
      categoryPairs.push_back(pair);
      coOccurrences[pair] = new contextCounter;

      if (instances.count(category1) == 0) {
         instances[category1] = new std::unordered_set<std::string>;
         instances[category1]->reserve(8192);
         std::ifstream instanceFile(instanceDir + category1);
         std::string seed;
         while (instanceFile >> seed) {
            instances[category1]->insert(seed);
         }
      }
      if (instances.count(category2) == 0) {
         instances[category2] = new std::unordered_set<std::string>;
         instances[category2]->reserve(8192);
         std::ifstream instanceFile(instanceDir + category2);
         std::string seed;
         while (instanceFile >> seed) {
            instances[category2]->insert(seed);
         }
      }
   }

   std::ifstream svoFile(svoFilename);
   std::string subject;
   std::string verbalPhrase;
   std::string object;
   std::string countStr;
   int count;

   while (svoFile.peek() != std::char_traits<char>::eof()) {
      std::getline(svoFile, subject, '\t');
      std::getline(svoFile, verbalPhrase, '\t');
      std::getline(svoFile, object, '\t');
      std::getline(svoFile, countStr);

      for (const auto &pair : categoryPairs) {
         if (instances[pair.first]->count(subject) > 0
               && instances[pair.second]->count(object) > 0) {
            count = std::stoi(countStr);
            context so = std::make_pair(subject, object);
            contextCounter* c = coOccurrences[pair];
            if (c->count(so) == 0) {
               (*c)[so] = new counter;
            }
            (*((*c)[so]))[verbalPhrase] += count;
         } else if (instances[pair.second]->count(subject) > 0
               && instances[pair.first]->count(object) > 0) {
            count = std::stoi(countStr);
            context so = std::make_pair(subject, object);
            contextCounter* c = coOccurrences[pair];
            if (c->count(so) == 0) {
               (*c)[so] = new counter;
            }
            (*((*c)[so]))[verbalPhrase] += count;
         }
      }
   }



   for (const categoryPair &catpair : categoryPairs) {
      std::cout << catpair.first << " <-> " << catpair.second << '\n';
      contextCounter* ccounter = coOccurrences[catpair];

      std::set<std::string> contextosEncontrados;
      std::unordered_map<context, unsigned int, hashpair> coocorre;

      for (const std::pair<context, counter*> &counters : (*ccounter)) {
         contextosEncontrados.insert(counters.first.first);
         contextosEncontrados.insert(counters.first.second);
         coocorre[counters.first] += 1;
      }

      size_t n = contextosEncontrados.size();
      CoOccurrenceMatrix m;
      m.n = n;
      m.matrix = new double*[n];
      m.features = new std::string[n];
      for (size_t k = 0; k < n; ++k)
         m.matrix[k] = new double[n];

      size_t i, j;
      i = 0;
      for (auto ctx1 : contextosEncontrados) {
         j = 0;
         for (auto ctx2 : contextosEncontrados) {
            m.matrix[i][j++] = coocorre[std::make_pair(ctx1, ctx2)];
         }
         ++i;
      }

      print_matrix(&m);
      std::cout << '\n';
   }
}
