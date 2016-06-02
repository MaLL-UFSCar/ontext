#include <iostream>
#include <fstream>
#include <unordered_set>
#include <map>
#include <string>

/**
 * argv[1]: Categories file
 * argv[2]: Instances directory
 * argv[3]: SVO file
 */

int main (int argc, char** argv) {
   std::map<std::string, std::unordered_set<std::string>* > instances;

   std::ifstream categoriesFile(argv[1]);
   std::string category1;
   std::string category2;
   int categoryid;
   std::string instanceDir(argv[2]);
   instanceDir += "/";

   while (categoriesFile >> category1 >> category2 >> categoryid) {
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
