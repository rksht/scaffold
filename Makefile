# For now just use a in-directory build. It's simple.

CXX = clang++
CC = clang

# Edit these flags if needed
CXXFLAGS = -std=c++11 -g -Wall
CFLAGS = -g -Wall

# Foundation library
FOUNDATION_HEADERS := types.h array.h queue.h hash.h memory.h memory_types.h murmur_hash.h string_stream.h math_types.h temp_allocator.h
FOUNDATION_OBJECTS := memory.o string_stream.o murmur_hash.o
FOUNDATION_LIB := libfoundation.a

# Argparse library
ARGPARSE_OBJECT := argparse.o
ARGPARSE_LIB := libargparse.a

# Scanner code
SCANNER_OBJECT := scanner.o
SCANNER_LIB := libscanner.a

LIBS = 

LIBS += $(FOUNDATION_LIB) $(ARGPARSE_LIB) $(SCANNER_LIB)

$(FOUNDATION_OBJECTS): %.o: %.cpp $(FOUNDATION_HEADERS)
	$(CXX) -c $(CXXFLAGS) $< -o $@

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

$(SCANNER_LIB): $(SCANNER_OBJECT)
	ar -rcs -o $(SCANNER_LIB) $(SCANNER_OBJECT)


# Edit and assign to these variables
APP = scan_test
##APP_HEADERS :=
APP_OBJECTS := scan_test.o

## LDFLAGS := -lpthread

$(APP): $(APP_OBJECTS)
	$(CXX) $(CXXFLAGS) -o $(APP) $(APP_OBJECTS) $(LIBS) $(LDFLAGS)

$(APP_OBJECTS): %.o: %.cpp $(APP_HEADERS) $(LIBS)
	$(CXX) -c $(CXXFLAGS) $< -o $@

.PHONY: clean clobber

clean:
	rm -rf foundation_test $(APP_NAME) *.o *.a

clobber:
	rm -rf *.o
