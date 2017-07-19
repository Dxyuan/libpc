CXX		 = g++
CXXFLAGS = -Wall -O -g -std=c++11 -lpc -lpthread -ldl
PROGS    = example_copystack example_pc example_pc_pool example_pc_schedule example_poll \
		   example_setenv example_specific example_thread example_cond
BUILDCXX = $(CXX) -o $@ $< $(CXXFLAGS)

all:$(PROGS)

example_copystack:example_copystack.cpp
	$(BUILDCXX)
example_pc:example_pc.cpp
	$(BUILDCXX)
example_pc_pool:example_pc_pool.cpp
	$(BUILDCXX)
example_pc_schedule:example_pc_schedule.cpp
	$(BUILDCXX)
example_poll:example_poll.cpp
	$(BUILDCXX)
example_setenv:example_setenv.cpp
	$(BUILDCXX)
example_specific:example_specific.cpp
	$(BUILDCXX)
example_thread:example_thread.cpp
	$(BUILDCXX)
example_cond:example_cond.cpp
	$(BUILDCXX)

clean:
	rm -f $(PROGS)
