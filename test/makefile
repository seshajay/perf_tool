CXX=g++
RM=rm -f
CPPFLAGS=-g -pthread -std=c++0x

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
	$(RM) $(OBJS) testclient testserver
