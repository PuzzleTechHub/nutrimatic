// See www.openfst.org for extensive documentation on this weighted
// finite-state transducer library.
//
// Finite-State Transducer (FST) archive classes.

#ifndef FST_EXTENSIONS_FAR_FAR_H_
#define FST_EXTENSIONS_FAR_FAR_H_

#include <iostream>
#include <sstream>

#include <fst/log.h>
#include <fst/extensions/far/stlist.h>
#include <fst/extensions/far/sttable.h>
#include <fstream>
#include <fst/fst.h>
#include <fst/vector-fst.h>

namespace fst {

enum FarEntryType { FET_LINE, FET_FILE };

enum FarTokenType { FTT_SYMBOL, FTT_BYTE, FTT_UTF8 };

inline bool IsFst(const std::string &source) {
  std::ifstream strm(source, std::ios_base::in | std::ios_base::binary);
  if (!strm) return false;
  return IsFstHeader(strm, source);
}

// FST archive header class
class FarHeader {
 public:
  const std::string &ArcType() const { return arctype_; }

  const std::string &FarType() const { return fartype_; }

  bool Read(const std::string &source) {
    FstHeader fsthdr;
    if (source.empty()) {
      // Header reading unsupported on stdin. Assumes STList and StdArc.
      fartype_ = "stlist";
      arctype_ = "standard";
      return true;
    } else if (IsSTTable(source)) {  // Checks if STTable.
      ReadSTTableHeader(source, &fsthdr);
      fartype_ = "sttable";
      arctype_ = fsthdr.ArcType().empty() ? "unknown" : fsthdr.ArcType();
      return true;
    } else if (IsSTList(source)) {  // Checks if STList.
      ReadSTListHeader(source, &fsthdr);
      fartype_ = "stlist";
      arctype_ = fsthdr.ArcType().empty() ? "unknown" : fsthdr.ArcType();
      return true;
    } else if (IsFst(source)) {  // Checks if FST.
      std::ifstream istrm(source,
                               std::ios_base::in | std::ios_base::binary);
      fsthdr.Read(istrm, source);
      fartype_ = "fst";
      arctype_ = fsthdr.ArcType().empty() ? "unknown" : fsthdr.ArcType();
      return true;
    }
    return false;
  }

 private:
  std::string fartype_;
  std::string arctype_;
};

enum FarType {
  FAR_DEFAULT = 0,
  FAR_STTABLE = 1,
  FAR_STLIST = 2,
  FAR_FST = 3,
};

// This class creates an archive of FSTs.
template <class A>
class FarWriter {
 public:
  using Arc = A;

  // Creates a new (empty) FST archive; returns null on error.
  static FarWriter *Create(const std::string &source,
                           FarType type = FAR_DEFAULT);

  // Adds an FST to the end of an archive. Keys must be non-empty and
  // in lexicographic order. FSTs must have a suitable write method.
  virtual void Add(const std::string &key, const Fst<Arc> &fst) = 0;

  virtual FarType Type() const = 0;

  virtual bool Error() const = 0;

  virtual ~FarWriter() {}

 protected:
  FarWriter() {}
};

// This class iterates through an existing archive of FSTs.
template <class A>
class FarReader {
 public:
  using Arc = A;

  // Opens an existing FST archive in a single file; returns null on error.
  // Sets current position to the beginning of the achive.
  static FarReader *Open(const std::string &source);

  // Opens an existing FST archive in multiple files; returns null on error.
  // Sets current position to the beginning of the achive.
  static FarReader *Open(const std::vector<std::string> &sources);

  // Resets current position to beginning of archive.
  virtual void Reset() = 0;

  // Sets current position to first entry >= key.  Returns true if a match.
  virtual bool Find(const std::string &key) = 0;

  // Current position at end of archive?
  virtual bool Done() const = 0;

  // Move current position to next FST.
  virtual void Next() = 0;

  // Returns key at the current position. This reference is invalidated if
  // the current position in the archive is changed.
  virtual const std::string &GetKey() const = 0;

  // Returns pointer to FST at the current position. This is invalidated if
  // the current position in the archive is changed.
  virtual const Fst<Arc> *GetFst() const = 0;

  virtual FarType Type() const = 0;

  virtual bool Error() const = 0;

  virtual ~FarReader() {}

 protected:
  FarReader() {}
};

template <class Arc>
class FstWriter {
 public:
  void operator()(std::ostream &strm, const Fst<Arc> &fst) const {
    fst.Write(strm, FstWriteOptions());
  }
};

template <class A>
class STTableFarWriter : public FarWriter<A> {
 public:
  using Arc = A;

  static STTableFarWriter *Create(const std::string &source) {
    auto *writer = STTableWriter<Fst<Arc>, FstWriter<Arc>>::Create(source);
    return new STTableFarWriter(writer);
  }

  void Add(const std::string &key, const Fst<Arc> &fst) final {
    writer_->Add(key, fst);
  }

  FarType Type() const final { return FAR_STTABLE; }

  bool Error() const final { return writer_->Error(); }

 private:
  explicit STTableFarWriter(STTableWriter<Fst<Arc>, FstWriter<Arc>> *writer)
      : writer_(writer) {}

  std::unique_ptr<STTableWriter<Fst<Arc>, FstWriter<Arc>>> writer_;
};

template <class A>
class STListFarWriter : public FarWriter<A> {
 public:
  using Arc = A;

  static STListFarWriter *Create(const std::string &source) {
    auto *writer = STListWriter<Fst<Arc>, FstWriter<Arc>>::Create(source);
    return new STListFarWriter(writer);
  }

  void Add(const std::string &key, const Fst<Arc> &fst) final {
    writer_->Add(key, fst);
  }

  FarType Type() const final { return FAR_STLIST; }

  bool Error() const final { return writer_->Error(); }

 private:
  explicit STListFarWriter(STListWriter<Fst<Arc>, FstWriter<Arc>> *writer)
      : writer_(writer) {}

  std::unique_ptr<STListWriter<Fst<Arc>, FstWriter<Arc>>> writer_;
};

template <class A>
class FstFarWriter final : public FarWriter<A> {
 public:
  using Arc = A;

  explicit FstFarWriter(const std::string &source)
      : source_(source), error_(false), written_(false) {}

  static FstFarWriter *Create(const std::string &source) {
    return new FstFarWriter(source);
  }

  void Add(const std::string &key, const Fst<A> &fst) final {
    if (written_) {
      LOG(WARNING) << "FstFarWriter::Add: only one FST supported,"
                   << " subsequent entries discarded.";
    } else {
      error_ = !fst.Write(source_);
      written_ = true;
    }
  }

  FarType Type() const final { return FAR_FST; }

  bool Error() const final { return error_; }

  ~FstFarWriter() final {}

 private:
  std::string source_;
  bool error_;
  bool written_;
};

template <class Arc>
FarWriter<Arc> *FarWriter<Arc>::Create(const std::string &source,
                                       FarType type) {
  switch (type) {
    case FAR_DEFAULT:
      if (source.empty()) return STListFarWriter<Arc>::Create(source);
    case FAR_STTABLE:
      return STTableFarWriter<Arc>::Create(source);
    case FAR_STLIST:
      return STListFarWriter<Arc>::Create(source);
    case FAR_FST:
      return FstFarWriter<Arc>::Create(source);
    default:
      LOG(ERROR) << "FarWriter::Create: Unknown FAR type";
      return nullptr;
  }
}

template <class Arc>
class FstReader {
 public:
  Fst<Arc> *operator()(std::istream &strm,
                       const FstReadOptions &options = FstReadOptions()) const {
    return Fst<Arc>::Read(strm, options);
  }
};

template <class A>
class STTableFarReader : public FarReader<A> {
 public:
  using Arc = A;

  static STTableFarReader *Open(const std::string &source) {
    auto *reader = STTableReader<Fst<Arc>, FstReader<Arc>>::Open(source);
    if (!reader || reader->Error()) return nullptr;
    return new STTableFarReader(reader);
  }

  static STTableFarReader *Open(const std::vector<std::string> &sources) {
    auto *reader = STTableReader<Fst<Arc>, FstReader<Arc>>::Open(sources);
    if (!reader || reader->Error()) return nullptr;
    return new STTableFarReader(reader);
  }

  void Reset() final { reader_->Reset(); }

  bool Find(const std::string &key) final { return reader_->Find(key); }

  bool Done() const final { return reader_->Done(); }

  void Next() final { return reader_->Next(); }

  const std::string &GetKey() const final { return reader_->GetKey(); }

  const Fst<Arc> *GetFst() const final { return reader_->GetEntry(); }

  FarType Type() const final { return FAR_STTABLE; }

  bool Error() const final { return reader_->Error(); }

 private:
  explicit STTableFarReader(STTableReader<Fst<Arc>, FstReader<Arc>> *reader)
      : reader_(reader) {}

  std::unique_ptr<STTableReader<Fst<Arc>, FstReader<Arc>>> reader_;
};

template <class A>
class STListFarReader : public FarReader<A> {
 public:
  using Arc = A;

  static STListFarReader *Open(const std::string &source) {
    auto *reader = STListReader<Fst<Arc>, FstReader<Arc>>::Open(source);
    if (!reader || reader->Error()) return nullptr;
    return new STListFarReader(reader);
  }

  static STListFarReader *Open(const std::vector<std::string> &sources) {
    auto *reader = STListReader<Fst<Arc>, FstReader<Arc>>::Open(sources);
    if (!reader || reader->Error()) return nullptr;
    return new STListFarReader(reader);
  }

  void Reset() final { reader_->Reset(); }

  bool Find(const std::string &key) final { return reader_->Find(key); }

  bool Done() const final { return reader_->Done(); }

  void Next() final { return reader_->Next(); }

  const std::string &GetKey() const final { return reader_->GetKey(); }

  const Fst<Arc> *GetFst() const final { return reader_->GetEntry(); }

  FarType Type() const final { return FAR_STLIST; }

  bool Error() const final { return reader_->Error(); }

 private:
  explicit STListFarReader(STListReader<Fst<Arc>, FstReader<Arc>> *reader)
      : reader_(reader) {}

  std::unique_ptr<STListReader<Fst<Arc>, FstReader<Arc>>> reader_;
};

template <class A>
class FstFarReader final : public FarReader<A> {
 public:
  using Arc = A;

  static FstFarReader *Open(const std::string &source) {
    std::vector<std::string> sources;
    sources.push_back(source);
    return new FstFarReader<Arc>(sources);
  }

  static FstFarReader *Open(const std::vector<std::string> &sources) {
    return new FstFarReader<Arc>(sources);
  }

  explicit FstFarReader(const std::vector<std::string> &sources)
      : keys_(sources), has_stdin_(false), pos_(0), error_(false) {
    std::sort(keys_.begin(), keys_.end());
    streams_.resize(keys_.size(), nullptr);
    for (size_t i = 0; i < keys_.size(); ++i) {
      if (keys_[i].empty()) {
        if (!has_stdin_) {
          streams_[i] = &std::cin;
          has_stdin_ = true;
        } else {
          FSTERROR() << "FstFarReader::FstFarReader: standard input should "
                        "only appear once in the input file list";
          error_ = true;
          return;
        }
      } else {
        streams_[i] = new std::ifstream(
            keys_[i], std::ios_base::in | std::ios_base::binary);
        if (streams_[i]->fail()) {
          FSTERROR() << "FstFarReader::FstFarReader: Error reading file: "
                     << sources[i];
          error_ = true;
          return;
        }
      }
    }
    if (pos_ >= keys_.size()) return;
    ReadFst();
  }

  void Reset() final {
    if (has_stdin_) {
      FSTERROR()
          << "FstFarReader::Reset: Operation not supported on standard input";
      error_ = true;
      return;
    }
    pos_ = 0;
    ReadFst();
  }

  bool Find(const std::string &key) final {
    if (has_stdin_) {
      FSTERROR()
          << "FstFarReader::Find: Operation not supported on standard input";
      error_ = true;
      return false;
    }
    pos_ = 0;  // TODO
    ReadFst();
    return true;
  }

  bool Done() const final { return error_ || pos_ >= keys_.size(); }

  void Next() final {
    ++pos_;
    ReadFst();
  }

  const std::string &GetKey() const final { return keys_[pos_]; }

  const Fst<Arc> *GetFst() const final { return fst_.get(); }

  FarType Type() const final { return FAR_FST; }

  bool Error() const final { return error_; }

  ~FstFarReader() final {
    for (size_t i = 0; i < keys_.size(); ++i) {
      if (streams_[i] != &std::cin) {
        delete streams_[i];
      }
    }
  }

 private:
  void ReadFst() {
    fst_.reset();
    if (pos_ >= keys_.size()) return;
    streams_[pos_]->seekg(0);
    fst_.reset(Fst<Arc>::Read(*streams_[pos_], FstReadOptions()));
    if (!fst_) {
      FSTERROR() << "FstFarReader: Error reading Fst from: " << keys_[pos_];
      error_ = true;
    }
  }

  std::vector<std::string> keys_;
  std::vector<std::istream *> streams_;
  bool has_stdin_;
  size_t pos_;
  mutable std::unique_ptr<Fst<Arc>> fst_;
  mutable bool error_;
};

template <class Arc>
FarReader<Arc> *FarReader<Arc>::Open(const std::string &source) {
  if (source.empty())
    return STListFarReader<Arc>::Open(source);
  else if (IsSTTable(source))
    return STTableFarReader<Arc>::Open(source);
  else if (IsSTList(source))
    return STListFarReader<Arc>::Open(source);
  else if (IsFst(source))
    return FstFarReader<Arc>::Open(source);
  return nullptr;
}

template <class Arc>
FarReader<Arc> *FarReader<Arc>::Open(const std::vector<std::string> &sources) {
  if (!sources.empty() && sources[0].empty())
    return STListFarReader<Arc>::Open(sources);
  else if (!sources.empty() && IsSTTable(sources[0]))
    return STTableFarReader<Arc>::Open(sources);
  else if (!sources.empty() && IsSTList(sources[0]))
    return STListFarReader<Arc>::Open(sources);
  else if (!sources.empty() && IsFst(sources[0]))
    return FstFarReader<Arc>::Open(sources);
  return nullptr;
}

}  // namespace fst

#endif  // FST_EXTENSIONS_FAR_FAR_H_
