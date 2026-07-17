#include "loader/PF_LoaderApp.h"

int main(int argc, char *argv[])
{
    PF_LoaderApp myApp(argc, argv);

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
