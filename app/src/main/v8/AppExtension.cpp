#include "main/v8/AppExtension.hpp"

#include <chrono>
#include <ctime>
#include <iostream>
#include <string>
#include <vector>

namespace app
{

namespace v8
{

bool AppExtension::Execute(const CefString &name, CefRefPtr<CefV8Value> object, const CefV8ValueList &arguments, CefRefPtr<CefV8Value> &retval, CefString &exception)
{
    if (name == "deBug")
    {
        return true;
    }
    
    return false;
}

void AppExtension::init()
{
    // ""
    //     "var img;"
    //     "if(!img) {"
    //     "  img = new Image();"
    //     "}"
        
    // register a V8 extension with the below JavaScript code that calls native methods implemented in the handler
    std::string code =
        ""
        "var MyApp;"
        "if (!MyApp) {"
        "  MyApp = {};"
        "}"
        ""
        "(function() {"
        "  MyApp.deBug = function() {"
        "    native function deBug();"
        "    return deBug();"
        "  };"
        ""
        "})();";

    CefRegisterExtension("v8/app", code, new app::v8::AppExtension());
}

} // namespace v8
} // namespace app
