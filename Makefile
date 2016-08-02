TARGET = pepatch
CXX = g++
CXXFLAGS = -g -Wall -Werror
CFLAGS = -g -Wall -Werror

.PHONY: default all clean

default: $(TARGET)
all: default

OBJECTS = $(patsubst %.cpp, %.o, $(wildcard src/*.cpp)) \
		  $(patsubst %.c, %.o, $(wildcard src/*.c))
HEADERS = $(wildcard src/*.h) src/version.h

src/version.h: src/version.h.in VERSION
	./scripts/version.py $< $@

src/%.o: src/%.cpp $(HEADERS)
	$(CXX) $(CXXFLAGS) -c $< -o $@

src/%.o: src/%.c $(HEADERS)
	$(CC) $(CFLAGS) -c $< -o $@

$(TARGET): $(OBJECTS)
	$(CXX) $^ -o $@

clean:
	$(RM) $(OBJECTS) $(TARGET) src/version.h
