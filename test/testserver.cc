#include "../app.h"
#include <iostream>
#include <string.h>
#include <signal.h>
#include <unistd.h>

using namespace std;

app::ServerApp *sapp = NULL;

void
handleSignal(int signum)
{
    cout << "Total bytes received: " << sapp->totalBytesReceived << endl;
    cout << "Exiting...\n";
    delete sapp;
    exit(signum);
}

void
usage()
{
    cout << "Usage:\n";
    cout << "    testserver -l <local ip> -p <listen port>";
    cout << " [-r <Recv buffer size>] [-b <listen backlog>]\n";
}

int
main(int argc, char* argv[]) try
{
    signal(SIGINT, handleSignal);

    char *lAddrStr = NULL;
    int lPort = 0, rcvBufSize = 0, backlog = 128, opt;

    while ((opt = getopt(argc, argv, "l:p:r:b:h")) != -1)
    {
        switch (opt)
        {
        case 'l':
            lAddrStr = optarg;
            break;
        case 'p':
            lPort = atoi(optarg);
            break;
        case 'r':
            rcvBufSize = atoi(optarg);

            if (rcvBufSize <= 0)
                throw std::runtime_error(ERRSTR("Please specify a non zero"
                                                "recv buffer size"));
            break;
        case 'b':
            backlog = atoi(optarg);
            break;
        case 'h':
        default:
            usage();
            exit(0);
        }
    }

    ip::sockaddr addr(lAddrStr, lPort);

    sapp = new app::ServerApp(addr, rcvBufSize, backlog);

    while (true)
    {
        this_thread::sleep_for(chrono::seconds(1));
    }

    delete sapp;
    return 0;
}
catch (std::exception& e)
{
    cout << "Unhandled exception: " << e.what();
    cout << " err: " << strerror(errno) << endl;
}
catch (...)
{
    cout << "Unhandled unknown exception" << endl;
}
