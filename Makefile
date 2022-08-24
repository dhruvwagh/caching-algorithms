CFLAGS = -W -Wall -Wextra -Wno-unused-variable -Wno-unused-function \
-march=native -O3  -std=c++20 -I../../c++/util -fno-exceptions \
-DNDEBUG # -ggdb3 -fno-strict-aliasing
# -fsanitize=undefined
LDFLAGS := $(LDFLAGS)

HEADERS = *.h *.hpp

.PRECIOUS: %.o

OBJS = $(patsubst %.cpp,%.o,$(wildcard *.cpp))

.PHONY: all clean

BINS = bench.out

all: $(BINS) $(OBJS) $(STATIC_OBJS)

clean:
	/bin/rm -f $(BINS) $(OBJS) $(STATIC_OBJS)

%.o: %.cpp ${HEADERS} Makefile
	$(CXX) -c $(CFLAGS) $< -o $@

%.out: %.o %.cpp Makefile
	$(CXX) $< -o $@ ${LDFLAGS}
