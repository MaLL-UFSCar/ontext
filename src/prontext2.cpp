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


/*
 * Global structures
 */
std::unordered_map<std::string, std::unordered_set<std::string>* > instances;
std::vector<categoryPair> categoryPairs;
occurrences coOccurrences;


/*!
 * \fn void readCategoriesFile(std::string filename, std::string instanceDir)
 * \brief Reads the category file (filename) into the global structures
 * \param filename The category pairs filename
 * \param instancesDir The path to the instances directory
 *
 * The global structures change as follows:
 *    1. categoryPairs will have all the pairs in the category pairs file
 *    2. coOccurrences will point each pair to an empty counter
 *    3. instances will map each category to a list of seeds/instances
 */
void readCategoriesFile(std::string filename, std::string instanceDir) {
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
 * \fn void readSvoFile(std::string filename)
 * \brief Reads the SVO file (filename) into global coOccurrences
 * \param filename Path to SVO file
 *
 * The coOccurrences mapping will be complete, meaning the full indexing
 * of it will return the count of those objects.
 */
void readSvoFile(std::string filename) {
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

int main (int argc, char** argv) {
   /*
    * arguments
    */
   std::string categoryPairsFilename(argv[1]);
   std::string instanceDir(argv[2]);
   std::string svoFilename(argv[3]);

   categoryPairs.reserve(256);
   instanceDir += "/";

   /*
    * Section: reading the categories file
    * Output:
    *    1. categoryPairs will have all the pairs in the file
    *    2. coOccurrences will point each pair to an empty counter
    *    3. instances map will point each category to a list of seeds/instances
    */
   readCategoriesFile(categoryPairsFilename, instanceDir);

   /*
    * Section: reading the SVO file
    * Output: the coOccurrences mapping will be complete
    */
   readSvoFile(svoFilename);


   /*
    * Section: building the co-occurrence matrices
    * Output: output every matrix in the screen
    * TODO: should instead call KMeans on the matrix and output the relations
    */
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
