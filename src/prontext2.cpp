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
   std::map<std::string, std::unordered_set<std::string> > instances;

   std::ifstream categoriesFile(argv[1]);
   std::string category1;
   std::string category2;
   int categoryid;

   while (categoriesFile >> category1 >> category2 >> categoryid) {
   }
}
