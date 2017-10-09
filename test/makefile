CXX=g++
RM=rm -rf
CPPFLAGS=-g -pthread -Wno-sign-compare -Wall -std=c++0x -Werror

SRCS=../ip.cc ../tcp.cc
SRCS+=../ts.cc ../ts-noop.cc ../ts-rl.cc
SRCS+=../traffic.cc ../app.cc
OBJS=$(subst .cc,.o,$(notdir $(SRCS)))

all: testclient testserver

testclient: $(OBJS) testclient.cc
	$(CXX) $(CPPFLAGS) -o testclient $(OBJS) testclient.cc

testserver: $(OBJS) testserver.cc
	$(CXX) $(CPPFLAGS) -o testserver $(OBJS) testserver.cc

$(OBJS): %.o : ../%.cc
	$(CXX) $(CPPFLAGS) -c $<

clean:
	$(RM) $(OBJS) testclient testserver *.dSYM
