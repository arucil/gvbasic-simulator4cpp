#include "file.h"
#include <cassert>

using namespace std;
using namespace gvbsim;


const char *File::toString(Mode mode) {
   switch (mode) {
   case Mode::INPUT:
      return "INPUT";
   case Mode::OUTPUT:
      return "OUTPUT";
   case Mode::RANDOM:
      return "RANDOM";
   case Mode::APPEND:
      return "APPEND";
   default:
      assert(0);
   }
}


File::File() : m_fp(nullptr) {

}

File::~File() {
   if (m_fp)
      close();
}

void File::close() {
   fclose(m_fp);
   m_fp = nullptr;
}

bool File::eof() {
   return feof(m_fp) != 0;
}

size_t File::size() {
   auto org = ftell(m_fp);
   fseek(m_fp, 0, SEEK_END);
   auto sz = ftell(m_fp);
   fseek(m_fp, org, SEEK_SET);
   return static_cast<size_t>(sz);
}
