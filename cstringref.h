#include <string.h>

/// A CstringRef class
class CstringRef {
  private:
    char *_data;

  public:
    /// Constructor
    CstringRef(char *data) : _data(data) {}

    /// Copy constructor
    CstringRef(const CstringRef &other) : _data(other._data) {}

    /// Move constructor
    CstringRef(CstringRef &&other) {
        if (this != &other) {
            _data = other._data;
            other._data = nullptr;
        }
    }

    /// Destructor
    ~CstringRef() { _data = nullptr; }

    char *data() { return _data; }

    size_t size() { return strlen(_data); }

    /// Pointer to begin
    char *begin() { return _data; }

    /// Pointer to end
    char *end() { return _data + strlen(_data); }
};