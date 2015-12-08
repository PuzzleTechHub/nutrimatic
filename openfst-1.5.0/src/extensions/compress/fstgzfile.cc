// Actual buffered reading for IGzStream.

#include <memory>
#include <sstream>

#include <fst/extensions/compress/fstgzfile.h>

using std::stringstream;
using std::unique_ptr;

namespace fst {

// This is a great case for "move", but GCC 4 is missing the C+11 standard
// move constructor for stringstream, so a unique_ptr is the next best thing.
unique_ptr<stringstream> IGzFile::read() {
  char buf[bufsize_];
  unique_ptr<stringstream> sstrm(new stringstream);
  // We always read at least once, and the result of the last read is always
  // pushed onto the stringstream. We use the "write" member because << onto a
  // stringstream stops at the null byte, which might be data!
  int bytes_read;
  while ((bytes_read = gz_.read(buf, bufsize_)) == bufsize_)
    sstrm->write(buf, bufsize_);
  sstrm->write(buf, bytes_read);
  return sstrm;
}

}  // namespace fst
