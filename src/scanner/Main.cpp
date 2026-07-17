#include <exception>
#include <iostream>

#include <decimal.hh>

using decimal::Decimal;

#include "scanner/PF_ScannerApp.h"

int main(int argc, char** argv)
{
    int result = 0;

    try
    {
        decimal::context_template = decimal::IEEEContext(decimal::DECIMAL64);
        decimal::context_template.round(decimal::ROUND_HALF_UP);
        decimal::context = decimal::context_template;

        std::ios_base::sync_with_stdio(false);

        PF_ScannerApp myApp(argc, argv);
        bool startup_ok = myApp.Startup();
        if (startup_ok)
        {
            myApp.Run();
            myApp.Shutdown();
        }
    }

    catch (std::system_error& e)
    {
        auto ec = e.code();
        std::cerr << "Category: " << ec.category().name() << ". Value: " << ec.value() <<
                ". Message: " << ec.message() << '\n';
        result = 3;
    }
    catch (std::exception& e)
    {
        std::cout << "Problem running scanner: " << e.what() << '\n';
        result = 4;
    }
    catch (...)
    {
        std::cout << "Unknown problem running scanner." << '\n';
        result = 5;
    }

    return result;
}
