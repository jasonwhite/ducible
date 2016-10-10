TARGET = ducible
CXXFLAGS = -Isrc -std=c++14 -g -Wall -Werror -Wno-unused-const-variable
CFLAGS = -Isrc -g -Wall -Werror

.PHONY: default all clean

default: $(TARGET)
all: default

OBJECTS = $(patsubst %.cpp, %.o, $(wildcard src/*/*.cpp)) \
		  $(patsubst %.c, %.o, $(wildcard src/util/*.c))
HEADERS = $(wildcard src/*.h) src/version.h

VERSION_DEPS=VERSION \
			 .git/HEAD \
			 $(wildcard .git/refs/heads/*)

src/version.h: src/version.h.in $(VERSION_DEPS)
	./scripts/version.py $< $@

src/%.o: src/%.cpp $(HEADERS)
	$(CXX) $(CXXFLAGS) -c $< -o $@

src/%.o: src/%.c $(HEADERS)
	$(CC) $(CFLAGS) -c $< -o $@

$(TARGET): $(OBJECTS)
	$(CXX) $^ -o $@

clean:
	$(RM) $(OBJECTS) $(TARGET) src/version.h
