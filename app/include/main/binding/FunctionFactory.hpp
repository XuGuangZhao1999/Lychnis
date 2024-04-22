#pragma once

#include <string>
#include <unordered_map>

#include "include/wrapper/cef_message_router.h"

namespace app{
    namespace binding{
        // FunctionFactory is used to map functionName to the actual function and register the mapping.
        template <class handlerImplement>
        class FunctionFactory{
            // Define a type to represent onTask*(...).
            using taskFuncTemplate = bool (handlerImplement::*)(CefRefPtr<CefBrowser>, CefRefPtr<CefFrame>, int64_t, CefRefPtr<CefDictionaryValue>, bool, CefRefPtr<CefMessageRouterBrowserSide::Callback>);
        public:
            void RegisterFunction(const std::string& name, taskFuncTemplate func){
                factory[name] = func;
            }
            taskFuncTemplate GetFunction(const std::string& name){
                return factory[name];
            }
        private:
            // tasks will store all registered functions.
            std::unordered_map<std::string, taskFuncTemplate> factory;
        };
    }
}