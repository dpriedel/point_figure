#include "PF_StreamerApp.h"

#include <iostream>

int main(int argc, char *argv[])
{
    try
    {
        PF_StreamerApp app(argc, argv);

        bool startup_OK = app.Startup();
        if (startup_OK)
        {
            app.Run();
            app.Shutdown();
        }
        else
        {
            std::cout << "Problems starting program.  No processing done.\n";
        }
    }
    catch (const std::exception &e)
    {
        std::cerr << "Fatal error: " << e.what() << "\n";
        return 1;
    }

    return 0;
}
