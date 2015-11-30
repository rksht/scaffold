#include "murmur_hash.h"
#include "hash.h"
#include "memory.h"
#include "array.h"

#include <stdio.h>
#include <stdlib.h>
#include <utility>
#include <assert.h>

using namespace foundation;
using namespace memory_globals;

struct Student {
    long id;
    long mana;
    char c;

    Student(long id, long mana, char c) : id(id), mana(mana), c(c) {}
};

int main() {
    init();
    {
        const int nr_objects = (2 * 1024 * 1024) / sizeof(Student);
        printf("Size = %f kbytes\n",
               (float)nr_objects * sizeof(Student) / 1024);
        Array<Student *> students(default_allocator());
        Student *student_p;
        for (int i = 0; i < nr_objects; ++i) {
            student_p =
                MAKE_NEW(default_arena_allocator(), Student, i, i, i % 128);
            array::push_back(students, student_p);
        }
        printf("Total allocated: %u bytes\n",
               default_arena_allocator().total_allocated());
        int j = 0;
        for (Student **i = array::begin(students); i != array::end(students);
             ++i) {
            if ((*i)->id != (*i)->mana && (*i)->c == j % 128) {
                printf("%li %li %c at %d, size = %u\n", (*i)->id, (*i)->mana,
                       (*i)->c, j,
                       default_arena_allocator().allocated_size(*i));
                fflush(stdout);
                assert(0);
            } else {
                uint32_t size = default_arena_allocator().allocated_size(*i);
                assert(size == sizeof(Student));
                printf("%li %li %c at %d, size = %u\n", (*i)->id, (*i)->mana,
                       (*i)->c, j, size);
            }
            ++j;
        }
    }
    shutdown();
    return 0;
}
