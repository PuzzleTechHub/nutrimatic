// Resource handles for gzip files written to or read from stringstreams.
// These are necessary to provide the compression routines with streams reading
// from or writing to compressed files (or the UNIX standard streams), and are
// not intended for general use.

#ifndef FST_EXTENSIONS_COMPRESS_FSTGZFILE_H_ // NOLINT
#define FST_EXTENSIONS_COMPRESS_FSTGZFILE_H_ // NOLINT

#include <zlib.h>

#include <cstdlib>
#include <cstdio>

#include <iostream>
#include <fstream>
#include <memory>
#include <sstream>
#include <string>

using std::stringstream;
using std::string;
using std::unique_ptr;

namespace fst {

// Gives the zlib gzFile type an OO-like interface. String inputs are all
// C-style strings. The caller is responsible to get the file modes appropriate
// for the IO methods being called, and for opening the file descriptors
// if that constructor is used. The ! operator can be used to check for errors
// after construction or read/writing.
class GzFile {
 public:
  GzFile(const char *filename, const char *mode)
      : gzfile_(gzopen(filename, mode)), error_(check_handle()) { }
  // The caller is responsible to ensure the corresponding FD is open and has
  // the needed modes ("r" for reading, "w" or "a" for writing).
  explicit GzFile(const int fd, const char *mode)
      : gzfile_(gzdopen(fd, mode)), error_(check_handle()), close_me_(false) { }
  // If the instance was constructed from an FD, flush the buffer; otherwise,
  // close the file, which flushes the buffer as a side-effect.
  ~GzFile() { close_me_ ? gzclose(gzfile_) : gzflush(gzfile_, Z_FINISH); }

  inline bool operator!() const { return error_; }

  // Returns false on EOF and sets error if short read does not reach an EOF.
  int read(void *buf, unsigned int size) {
    int bytes_read = gzread(gzfile_, buf, size);
    if ((bytes_read < size) && !gzeof(gzfile_)) error_ = true;
    return bytes_read;
  }

  // Sets error on short writes.
  void write(const char *buf, unsigned int size) {
    if (gzwrite(gzfile_, buf, size) != size) error_ = true;
  }

 private:
  // gzopen and gzdopen signal failure by returning NULL.
  bool check_handle() { return gzfile_ == nullptr; }

  gzFile gzfile_ = nullptr;
  bool error_ = false;
  bool close_me_ = false;
};

// Resource handle for writing stringstream to GzFile.
class OGzFile {
 public:
  explicit OGzFile(const string &filename) : OGzFile(filename.c_str()) { }
  explicit OGzFile(const char *filename) : gz_(GzFile(filename, mode_)) { }
  explicit OGzFile(const int fd) : gz_(GzFile(fd, mode_)) { }

  inline bool operator!() const { return !gz_; }

  void write(const stringstream &ssbuf) {
    string sbuf = ssbuf.str();
    gz_.write(sbuf.data(), sbuf.size());
  }

 private:
  GzFile gz_;
  static constexpr auto &mode_ = "wb";
};

// Resource handle for reading stringstream from GzFile.
class IGzFile {
 public:
  explicit IGzFile(const string &filename) : IGzFile(filename.c_str()) { }
  explicit IGzFile(const char *filename) : gz_(GzFile(filename, mode_)) { }
  explicit IGzFile(const int fd) : gz_(GzFile(fd, mode_)) { }

  inline bool operator!() const { return !gz_; }

  // Implemented in the cc;
  unique_ptr<stringstream> read();

 private:
  GzFile gz_;
  static constexpr auto &mode_ = "rb";
  // This is the same size as the default internal buffer for zlib.
  static const size_t bufsize_ = 8192;
};

}  // namespace fst

#endif  // FST_EXTENSIONS_COMPRESS_FSTGZFILE_H_ // NOLINT
