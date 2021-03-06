# perf_tool

perf_tool is a simple, lightweight multi-threaded application that can be used
to send and receive data. We can create multiple independent threads on the
sender to drive a lot of network traffic to test both small IO network
performance and the maximum available network capacity.

There is a simple traffic shaper that can be used to set rate limits on every
network connection created using perf_tool. This can be used to introduce
controlled background traffic while testing the performance of any application.
Typically network emulators allow introducing controlled background traffic. But
the background traffic introduced will be UDP traffic which is not self-
adjusting. perf_tool allows us to introduce shaped self-adjusting TCP flows
that better replicate the real networks and can come in handy especially for
network protocol testing. These background flows can be short-lived or
long-lived and can run different TCP congestion control algorithms.

There are some sample apps in the 'test' folder. test/testclient.cc instantiates
app::ClientApp and can be used to send traffic to the remote end using multiple
connections. test/testserver.cc instantiates app::ServerApp which creates an
instance of app::TrafficServer for every incoming connection, to process
incoming data in its own thread. This apps currently compile on linux (ubuntu)
and OSX.

To compile:

$ cd test/

$ make

To clean:

$ make clean

To test:

$ ./testclient -h

$ ./testclient -c 192.168.1.11 -p 11200 -l 192.168.1.10 -t 60 -r 500mbit -n 2

$ ./testserver -h

$ ./testserver -l 192.168.1.11 -p 11200
