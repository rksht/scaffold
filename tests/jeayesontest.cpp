#include "jeayeson/jeayeson.hpp"

int main() {
    json_value val{
        {"hello", "world"},
        {"arr", {1.1, 2.2, 3.3}},
        {"person", {{"name", "Tom"}, {"age", 36}, {"weapon", nullptr}}}
    };
}