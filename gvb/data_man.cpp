#include "data_man.h"

using namespace std;
using namespace gvbsim;

void DataManager::restore() {
   p = 0;
}

void DataManager::restore(int label) {
   p = labels[label];
}

void DataManager::add(const string &s) {
   data.push_back(s);
}

void DataManager::addLabel(int label) {
   labels[label] = data.size();
}

const string &DataManager::get() {
   return data[p++];
}

size_t DataManager::size() const {
   return data.size();
}

bool DataManager::reachesEnd() const {
   return p >= data.size();
}

void DataManager::clear() {
   labels.clear();
   data.clear();
   restore();
}
