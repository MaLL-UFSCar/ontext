#include <iostream>
#include <fstream>
#include <unordered_set>
#include <map>
#include <string>
#include <utility>
#include <vector>

/**
 * argv[1]: Categories file
 * argv[2]: Instances directory
 * argv[3]: SVO file
 */

int main (int argc, char** argv) {
   std::map<std::string, std::unordered_set<std::string>* > instances;
   std::vector<std::pair<std::string, std::string> > categoryPairs;
   categoryPairs.reserve(256);

   {
      std::ifstream categoriesFile(argv[1]);
      std::string category1;
      std::string category2;
      int categoryid;
      std::string instanceDir(argv[2]);
      instanceDir += "/";

      while (categoriesFile >> category1 >> category2 >> categoryid) {
         categoryPairs.push_back(std::make_pair(category1, category2));
         if (instances.count(category1) == 0) {
            instances[category1] = new std::unordered_set<std::string>;
            instances[category1]->reserve(8192);
            std::ifstream instanceFile(instanceDir + category1);
            std::string seed;
            while (instanceFile >> seed) {
               instances[category1]->insert(seed);
            }
         }
      }
   }

   std::ifstream svoFile(argv[3]);
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

      for (auto &pair : categoryPairs) {
         if (instances[pair.first]->count(subject) > 0
               && instances[pair.second]->count(object) > 0) {
            count = std::stoi(countStr);
            // count this instance f12
         } else if (instances[pair.second]->count(subject) > 0
               && instances[pair.first]->count(object) > 0) {
            count = std::stoi(countStr);
            // count this instance f21
         }
      }
   }

}
