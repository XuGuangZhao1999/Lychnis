#pragma once

#include <string>
#include <unordered_map>

#include "include/cef_urlrequest.h"
#include "include/wrapper/cef_message_router.h"

#include "main/binding/FunctionFactory.hpp"

namespace app
{
namespace binding
{

// handle messages in the browser process
class MenuBarBinding : public CefMessageRouterBrowserSide::Handler
{
public:
    MenuBarBinding();
    virtual ~MenuBarBinding() {}

    // called due to cefQuery execution
    virtual bool OnQuery(CefRefPtr<CefBrowser> browser, CefRefPtr<CefFrame> frame, int64_t queryId, const CefString &request, bool persistent, CefRefPtr<Callback> callback) override;

    // called on AppClient
    static void init(CefRefPtr<CefMessageRouterBrowserSide> router);

    // tasks
    bool onTaskReverseData(CefRefPtr<CefBrowser> browser, CefRefPtr<CefFrame> frame, int64_t queryId, const CefString &request, bool persistent, CefRefPtr<Callback> callback, const std::string &messageName, const std::string &requestMessage);
    bool onTaskNetworkRequest(CefRefPtr<CefBrowser> browser, CefRefPtr<CefFrame> frame, int64_t queryId, const CefString &request, bool persistent, CefRefPtr<Callback> callback, const std::string &messageName, const std::string &requestMessage);
    bool onTaskLoadProject(CefRefPtr<CefBrowser> browser, CefRefPtr<CefFrame> frame, int64_t queryId, const CefString &request, bool persistent, CefRefPtr<Callback> callback);
    bool onTaskWheelEvent(CefRefPtr<CefBrowser> browser, CefRefPtr<CefFrame> frame, int64_t queryId, const CefString &request, bool persistent, CefRefPtr<Callback> callback, const std::string &messageName, const std::string &requestMessage);

    // callback
    void onRequestComplete(CefRefPtr<Callback> callback, CefURLRequest::ErrorCode errorCode, const std::string &downloadData);

private:
    FunctionFactory<MenuBarBinding> tasks;
    DISALLOW_COPY_AND_ASSIGN(MenuBarBinding);
};

} // namespace binding
} // namespace app
