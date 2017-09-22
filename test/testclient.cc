#include "../app.h"
#include <iostream>
#include <string.h>
#include <signal.h>
#include <unistd.h>

using namespace std;

app::ClientApp *capp = NULL;
mutex clientCompletedLock;
condition_variable clientCompletedCV;
int i = 0;

static void
printStats()
{
    uint64_t tput = capp->totalBytesSent / capp->testDurationSec;
    tput *= 8;
    string tputStr = (tput) ? formatThroughput(tput) : "0 bps";

    cout << "Test stats:\n";
    cout << "  Sent: " << capp->totalBytesSent << " bytes\n";
    cout << "  Time: " << capp->testDurationSec << " sec\n";
    cout << "  Throughput: " << tputStr.c_str() << endl;
}

static void
handleSignal(int signum)
{
    cout << "Exiting...\n";
    delete capp;
    exit(signum);
}

void
handleClientAppDone()
{
	std::lock_guard<std::mutex> lock(clientCompletedLock);
	i = 1;
	clientCompletedCV.notify_all();
}

static void
usage()
{
    cout << "Usage:\n";
    cout << "    testclient -c <remote ip> -p <remote port> -l <local ip>";
    cout << " [-t <test duration>] [-n <num of connections>]";
    cout << " [-m <message size>] [-s send buffer size] [-r Rate limit]\n";
}

int
main(int argc, char* argv[]) try
{
    signal(SIGINT, handleSignal);

    char *rAddrStr = NULL, *lAddrStr = NULL, *rate = NULL;
    int rPort = 0, testDuration = 10, numConnections = 1;
    int msgSize = app::large, sndBufSize = 0, opt;

    //TODO: Improve this to parse address of form <ip>:<port>
    while ((opt = getopt(argc, argv, "c:p:l:t:n:m:s:r:h")) != -1)
    {
        switch (opt)
        {
        case 'c':
            rAddrStr = optarg;
            break;
        case 'p':
            rPort = atoi(optarg);
            break;
        case 'l':
            lAddrStr = optarg;
            break;
        case 't':
            testDuration = atoi(optarg);

            if (testDuration <= 0)
                throw std::runtime_error(ERRSTR("Please specify a valid "
                                                "non zero duration"));
            break;
        case 'n':
            numConnections = atoi(optarg);

            if (numConnections < 1)
                throw std::runtime_error(ERRSTR("Need at least one"
                                                " connection"));
            break;

        case 'm':
            msgSize = atoi(optarg);

            if (msgSize < 32 || msgSize > 64*1024)
                throw std::runtime_error(ERRSTR("Need msg size between 32 bytes"
                                                " and 64k bytes"));
            break;
        case 's':
            sndBufSize = atoi(optarg);

            if (sndBufSize <= 0)
                throw std::runtime_error(ERRSTR("Please specify a non zero"
                                                "send buffer size"));
            break;
        case 'r':
            rate = optarg;
            break;
        case 'h':
        default:
            usage();
            exit(0);
        }
    }

    cout <<"Starting the traffic test...\n";
    ip::sockaddr laddr(lAddrStr, 0);
    ip::sockaddr raddr(rAddrStr, rPort);
	func_t cb = std::bind(handleClientAppDone);

    ts::TSDescriptor tsd = {"noop", ""};
    if (rate)
    {
        tsd.name = "rate-limit";
        tsd.args  = rate;
    }

    capp = new app::ClientApp(laddr, raddr, testDuration, cb, tsd,
                              numConnections, (app::MsgSize) msgSize,
                              sndBufSize);

	std::unique_lock<std::mutex> ul(clientCompletedLock);
    clientCompletedCV.wait(ul, []{return i == 1;});

    printStats();
    delete capp;
    return 0;
}
catch (std::exception& e)
{
    cout << "Unhandled exception: " << e.what();
    cout << " err: " << strerror(errno) << endl;
	delete capp;
}
catch (...)
{
    cout << "Unhandled unknown exception" << endl;
	delete capp;
}
