DUCIBLE_TARGET = ducible
PDBDUMP_TARGET = pdbdump
CXXFLAGS = -Isrc -std=c++14 -g -Wall -Werror -Wno-unused-const-variable
CFLAGS = -Isrc -g -Wall -Werror

.PHONY: default all clean

default: $(DUCIBLE_TARGET) $(PDBDUMP_TARGET)
all: default

COMMON_OBJECTS= \
	$(patsubst %.cpp, %.o, $(wildcard src/util/*.cpp src/msf/*.cpp src/pe/*.cpp src/pdb/*.cpp)) \
	$(patsubst %.c, %.o, $(wildcard src/util/*.c))

DUCIBLE_OBJECTS = $(COMMON_OBJECTS) $(patsubst %.cpp, %.o, $(wildcard src/ducible/*.cpp))
PDBDUMP_OBJECTS = $(COMMON_OBJECTS) $(patsubst %.cpp, %.o, $(wildcard src/pdbdump/*.cpp))

HEADERS = $(wildcard src/*/*.h) src/version.h

VERSION_DEPS=VERSION .git/HEAD $(wildcard .git/refs/heads/*)

src/version.h: src/version.h.in $(VERSION_DEPS)
	./scripts/version.py $< $@

src/%.o: src/%.cpp $(HEADERS)
	$(CXX) $(CXXFLAGS) -c $< -o $@

src/%.o: src/%.c $(HEADERS)
	$(CC) $(CFLAGS) -c $< -o $@

$(DUCIBLE_TARGET): $(DUCIBLE_OBJECTS)
	$(CXX) $^ -o $@

$(PDBDUMP_TARGET): $(PDBDUMP_OBJECTS)
	$(CXX) $^ -o $@

clean:
	$(RM) $(PDBDUMP_OBJECTS) $(DUCIBLE_OBJECTS) $(DUCIBLE_TARGET) $(PDBDUMP_TARGET src/version.h
