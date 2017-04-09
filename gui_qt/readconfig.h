#ifndef READCONFIG_H
#define READCONFIG_H

#include <unordered_map>
#include <string>

namespace gvbsim {


class ConfigReader {
public:
   typedef std::unordered_map<std::string, std::string> Section;
   
private:
   std::unordered_map<std::string, Section> m_map;
   
public:
   void load(const std::string &fn);
   Section &section(const std::string &);
   bool hasSection(const std::string &) const;
};

}

#endif // READCONFIG_H
