CFLAGS = -W -Wall -Wextra -Wno-unused-variable -Wno-unused-function \
-march=native -O3  -std=c++20 -I../../c++/util -fno-exceptions
# -ggdb3 -fno-strict-aliasing
# -fsanitize=undefined
LDFLAGS = 

HEADERS = *.hpp

.PRECIOUS: %.o

OBJS = $(patsubst %.cpp,%.o,$(wildcard *.cpp))

.PHONY: all

BINS = bench.out parallel.out

all: $(BINS)

clean:
	/bin/rm -f $(BINS) $(OBJS) $(STATIC_OBJS)

%.o: %.cpp
	$(CXX) -c $(CFLAGS) $< -o $@

%.out: %.o %.cpp
	$(CXX) $< -o $@ ${LDFLAGS}
