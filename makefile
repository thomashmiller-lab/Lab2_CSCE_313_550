CXX=g++
CXXFLAGS=-std=c++17 -g -pedantic -Wall -Wextra -Werror -fsanitize=address,undefined -fno-omit-frame-pointer
LDLIBS=

SRCS=shell.cpp
DEPS=Command.cpp Tokenizer.cpp
BINS=$(SRCS:%.cpp=%.exe)
OBJS=$(DEPS:%.cpp=%.o)


# ============================================================
# Build
# ============================================================

all: clean $(BINS)


%.o: %.cpp %.h
	$(CXX) $(CXXFLAGS) -c -o $@ $<


%.exe: %.cpp $(OBJS)
	$(CXX) $(CXXFLAGS) -o $(patsubst %.exe,%,$@) $^ $(LDLIBS)


# ============================================================
# Run
# ============================================================

run: all
	./shell


# ============================================================
# Public Tests
# ============================================================

test: all
	chmod u+x lab2-tests.sh
	./lab2-tests.sh


# ============================================================
# Clean
# ============================================================

clean:
	rm -f shell
	rm -f *.o
	rm -f a b
	rm -f test.txt output.txt result.txt
	rm -f test-output.txt
	rm -f test-append.txt
	rm -f test-pipe-output.txt
	rm -f out.trace
	rm -f ./test-files/cmd*.txt
	rm -f ./test-files/out*.txt


.PHONY: all clean run test
