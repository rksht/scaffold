# For now just use a in-directory build. It's simple.

CXX = clang++
CC = clang

# Edit these flags if needed
CXXFLAGS = -std=c++11 -g -Wall -fsanitize=address
CFLAGS = -g -Wall

# Foundation library
FOUNDATION_HEADERS := types.h array.h queue.h hash.h memory.h memory_types.h murmur_hash.h string_stream.h math_types.h temp_allocator.h pod_hash.h
FOUNDATION_OBJECTS := memory.o string_stream.o murmur_hash.o
FOUNDATION_LIB := libfoundation.a

# Argparse library
ARGPARSE_OBJECT := argparse.o
ARGPARSE_LIB := libargparse.a

# Scanner code
SCANNER_OBJECT := scanner.o
SCANNER_LIB := libscanner.a

# Scanner code
JSON_OBJECT := json.o
JSON_LIB := libjson.a

# Library command line
LIBS = -L `pwd`  -ljson -lscanner -lfoundation -largparse

# Names of the library archives
LIBNAMES += $(JSON_LIB) $(SCANNER_LIB) $(FOUNDATION_LIB) $(ARGPARSE_LIB)

# Build the libraries

$(FOUNDATION_OBJECTS): %.o: %.cpp $(FOUNDATION_HEADERS)
	$(CXX) -c  $(CXXFLAGS) $< -o $@

$(FOUNDATION_LIB): $(FOUNDATION_OBJECTS)
	ar -rcs -o $(FOUNDATION_LIB) $?

foundation_test: foundation_test.cpp $(FOUNDATION_HEADERS) $(FOUNDATION_LIB)
	$(CXX) $(CXXFLAGS) -o foundation_test foundation_test.cpp $(FOUNDATION_LIB)

$(ARGPARSE_OBJECT): argparse.c argparse.h
	$(CC) -c $(CFLAGS) argparse.c -o argparse.o

$(ARGPARSE_LIB): $(ARGPARSE_OBJECT)
	ar -rcs -o $(ARGPARSE_LIB) $(ARGPARSE_OBJECT)

$(SCANNER_OBJECT): scanner.cpp scanner.h
	$(CXX) -c $(CXXFLAGS) scanner.cpp -o scanner.o

$(SCANNER_LIB): $(SCANNER_OBJECT) $(FOUNDATION_LIB)
	ar -rcs -o $(SCANNER_LIB) $(SCANNER_OBJECT)

$(JSON_OBJECT): json.cpp json.h $(SCANNER_OBJECT) $(FOUNDATION_LIB)
	$(CXX) -c $(CXXFLAGS) json.cpp -o json.o

$(JSON_LIB): $(JSON_OBJECT) $(FOUNDATION_LIB) $(SCANNER_LIB)
	ar -rcs -o $(JSON_LIB) $(JSON_OBJECT)


# Build the app

# Edit these variables
#APP := pod_hash_test
#APP_HEADERS := pod_hash.h
#APP_OBJECTS := pod_hash_test.o

#APP := iloctests
#APP_HEADERS := ilocparse.h ilocparseop.inc.h iloctypes.h iloc.h
#APP_OBJECTS := ilocparse.o iloctests.o

#APP := pod_hash_test
#APP_HEADERS := pod_hash.h
#APP_OBJECTS := pod_hash_test.o

APP := json_test
APP_HEADERS := 
APP_OBJECTS := json_test.o

# Extra compiler and linker flags
## CXXFLAGS +=
## LDFLAGS += -lpthread

$(APP): $(APP_OBJECTS)
	$(CXX) $(CXXFLAGS) -o $(APP) $(APP_OBJECTS) $(LIBS) $(LDFLAGS)

$(APP_OBJECTS): %.o: %.cpp $(APP_HEADERS) $(LIBNAMES)
	$(CXX) -c $(CXXFLAGS) $< -o $@

.PHONY: clean clobber

clean:
	rm -rf foundation_test $(APP_NAME) *.o *.so

clobber:
	rm -rf *.o
