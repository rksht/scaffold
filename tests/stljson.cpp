#include <map>
#include <vector>
#include <string>
#include <iostream>

namespace json {

template <typename T> class printer {
  public:
    static void print(std::ostream &os, const T &m) { os << m; }
};

template <typename V> class printer<std::map<std::string, V>> {
  public:
    static void print(std::ostream &os, const std::map<std::string, V> &m) {
        os << "{";
        unsigned n = 0;
        for (auto i = m.cbegin(), e = m.cend(); i != e; i++) {
            os << i->first << ": ";
            printer<V>::print(os, i->second);
            if (++n < m.size()) {
                os << ", ";
            }
        }
        os << "}";
    }
};

template <typename V> class printer<std::map<const char *, V>> {
  public:
    static void print(std::ostream &os, const std::map<const char *, V> &m) {
        os << "{";
        unsigned n = 0;
        for (auto i = m.cbegin(), e = m.cend(); i != e; i++) {
            os << i->first << ": ";
            printer<V>::print(os, i->second);
            if (++n < m.size()) {
                os << ", ";
            }
        }
        os << "}";
    }
};

template <typename V> class printer<std::vector<V>> {
  public:
    static void print(std::ostream &os, const std::vector<V> &v) {
        os << "[";
        for (auto i = v.cbegin(); i != v.cend(); i++) {
            printer<V>::print(os, *i);
            if (i + 1 < v.cend() - 1) {
                os << ", ";
            }
        }
        os << "]";
    }
};
}

using score_card = std::map<std::string, unsigned>;
using student_list = std::map<std::string, score_card>;

int main() {

    score_card a;
    a["English"] = 90;
    a["History"] = 80;
    a["Math"] = 95;
    score_card b;
    b["English"] = 80;
    b["History"] = 85;
    b["Math"] = 90;

    student_list lst;
    lst["John"] = std::move(a);
    lst["Jane"] = std::move(b);

    json::printer<student_list>::print(std::cout, lst);
    std::cout << "\n";
}
