#include "updater/PF_UpdaterApp.h"

#include <spdlog/spdlog.h>

int main(int argc, char *argv[])
{
    PF_UpdaterApp myApp(argc, argv);

    bool startup_OK = myApp.Startup();
    if (startup_OK)
    {
        myApp.Run();
        myApp.Shutdown();
    }
    else
    {
        std::cout << "Problems starting program.  No processing done.\n";
    }

    return 0;
}
