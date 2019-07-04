CXX = g++
CXXFLAGS = -g3 -std=c++14 -D__STDC_CONSTANT_MACROS \
		   -Wall -Wextra -Wextra -pedantic \
		   -Wdisabled-optimization -Wctor-dtor-privacy -Wmissing-declarations \
		   -Woverloaded-virtual -Wshadow -Wno-unused -Winline -fprofile-arcs -ftest-coverage
LDLIBS = -lavformat -lavcodec -lavutil -lswscale -lswresample -lavresample -lSDL2 -pthread -lgtest -lgtest_main -lgcov





src = $(wildcard  main.cpp)


obj = $(src:.cpp=.o)

dep = $(obj:.o=.d)

target = transcoding

all: $(target)



$(target): $(obj)
	$(CXX) -o $@ $^ $(LDLIBS)


-include $(dep)

%.d: %.cpp
	@$(CXX) $(CXXFLAGS) $< -MM -MT $(@:.d=.o) >$@

test: $(target)
	./$(target)

.PHONY: clean
clean:
	$(RM) $(obj) $(target) $(dep)
