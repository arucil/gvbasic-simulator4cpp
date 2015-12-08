#include <sstream>
#include "error.h"
#include "tree.h"

using std::stringstream;

string GVBError::description() const {
    stringstream a;
    a << what() << " at line " << line << " (label: " << label << ")";
    return a.str();
}