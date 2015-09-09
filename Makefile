CXX = clang++
CC = clang

# Edit these flags if needed
CXXFLAGS = -std=c++11 -g -Wall
CFLAGS = -g -Wall

SRC_DIR := /home/snyp/gits/scaffold

# Foundation library
FOUNDATION_HEADERS := types.h array.h queue.h hash.h memory.h memory_types.h murmur_hash.h \
  string_stream.h math_types.h temp_allocator.h pod_hash.h pod_hash_usuals.h

FOUNDATION_HEADERS := $(patsubst %,$(SRC_DIR)/%,$(FOUNDATION_HEADERS))

FOUNDATION_SOURCES := memory.cpp string_stream.cpp murmur_hash.cpp pod_hash_usuals.cpp

FOUNDATION_OBJECTS := $(patsubst %.cpp,%.o,$(FOUNDATION_SOURCES))

FOUNDATION_SOURCES := $(patsubst %,$(SRC_DIR)/%,$(FOUNDATION_SOURCES))

FOUNDATION_LIB := libfoundation.a

$(info $(FOUNDATION_OBJECTS))

# Scanner code
SCANNER_OBJECT := scanner.o
SCANNER_LIB := libscanner.a

# Scanner code
JSON_OBJECT := json.o
JSON_LIB := libjson.a

# Library command line
LIBS = -L `pwd` -ljson -lscanner -lfoundation
# Names of the library archives
LIBNAMES += $(JSON_LIB) $(SCANNER_LIB) $(FOUNDATION_LIB)

# Build the libraries

$(FOUNDATION_OBJECTS): %.o: $(SRC_DIR)/%.cpp $(FOUNDATION_HEADERS)
	$(CXX) -c $(CXXFLAGS) $< -o $@

$(FOUNDATION_LIB): $(FOUNDATION_OBJECTS)
	ar -rcs -o $(FOUNDATION_LIB) $?

foundation_test: $(SRC_DIR)/foundation_test.cpp $(FOUNDATION_HEADERS) $(FOUNDATION_LIB)
	$(CXX) $(CXXFLAGS) -o foundation_test foundation_test.cpp $(FOUNDATION_LIB)

$(SCANNER_OBJECT): $(SRC_DIR)/scanner.cpp $(SRC_DIR)/scanner.h
	$(CXX) -c $(CXXFLAGS) $(SRC_DIR)/scanner.cpp -o $@

$(SCANNER_LIB): $(SCANNER_OBJECT) $(FOUNDATION_LIB)
	ar -rcs -o $@ $(SCANNER_OBJECT)

$(JSON_OBJECT): $(SRC_DIR)/json.cpp $(SRC_DIR)/json.h $(SCANNER_OBJECT) $(FOUNDATION_LIB)
	$(CXX) -c $(CXXFLAGS) $(SRC_DIR)/json.cpp -o $@

$(JSON_LIB): $(JSON_OBJECT) $(FOUNDATION_LIB) $(SCANNER_LIB)
	ar -rcs -o $@ $(JSON_OBJECT)

# Build the app
# Edit these variables
##APP :=  scan_test
##APP_HEADERS := 
##APP_SOURCES  := scan_test.cpp

APP := json_test
APP_HEADERS := 
APP_SOURCES  := json_test.cpp
# Prepend build dir
APP_OBJECTS := $(patsubst %.cpp,%.o,$(APP_SOURCES))
APP_SOURCES := $(patsubst %,$(SRC_DIR)/%,$(APP_SOURCES))
APP :=  $(APP)

$(info FOUNDATION_OBJECTS = $(FOUNDATION_OBJECTS))
$(info SCANNER_OBJECT = $(SCANNER_OBJECT))
$(info JSON_OBJECT = $(JSON_OBJECT))
$(info APP_OBJECTS = $(APP_OBJECTS))

# Extra compiler and linker flags
## CXXFLAGS +=
## LDFLAGS += -lpthread

$(APP): $(APP_OBJECTS)
	$(CXX) $(CXXFLAGS) -o $(APP) $(APP_OBJECTS) $(LIBS) $(LDFLAGS)

$(APP_OBJECTS): %.o: $(SRC_DIR)/%.cpp $(APP_HEADERS) $(LIBNAMES)
	$(CXX) -c $(CXXFLAGS) $< -o $@

.PHONY: clean clobber

clean:
	rm -rf foundation_test $(APP_NAME) *.o *.a

clobber:
	rm -rf *.o *.a
