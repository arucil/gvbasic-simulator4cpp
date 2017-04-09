#include "readconfig.h"
#include <fstream>
#include <cctype>

using namespace std;
using namespace gvbsim;


bool ConfigReader::hasSection(const string &s) const {
   return m_map.count(s) != 0;
}

ConfigReader::Section &ConfigReader::section(const string &s) {
   return m_map[s];
}

static string &trim(string &s) {
   if (s.empty())
      return s;
   
   size_t i = 0;
   while (i < s.size() && isspace(s[i]))
      ++i;
   
   size_t j = s.size();
   while (j-- > 0 && isspace(s[j]))
      ;
   
   s.erase(j + 1);
   s.erase(0, i);
   return s;
}

void ConfigReader::load(const string &fn) {
   m_map.clear();
   ifstream fin(fn);
   if (!fin.is_open())
      return;
   
   Section *sec = nullptr;
   
   while (!fin.eof()) {
      string buf;
      if (!getline(fin, buf))
         break;
      trim(buf);
      if (buf.empty() || '#' == buf[0])
         continue;
      
      if ('[' == buf[0]) {
         if (buf.back() == ']') {
            sec = &m_map[buf.substr(1, buf.size() - 2)];
         }
      } else {
         auto pos = buf.find('=');
         if (pos != string::npos) {
            string key(buf, 0, pos);
            string value(buf, pos + 1);
            sec->insert(make_pair(trim(key), trim(value)));
         }
      }
   }
}