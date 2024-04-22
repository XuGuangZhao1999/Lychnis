#pragma once

#include <string>
#include <unordered_map>

#include "include/cef_urlrequest.h"
#include "include/wrapper/cef_message_router.h"

#include "main/binding/FunctionFactory.hpp"
#include "main/binding/VolumeViewer.hpp"

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
    bool onTaskLoadProject(CefRefPtr<CefBrowser> browser, CefRefPtr<CefFrame> frame, int64_t queryId, CefRefPtr<CefDictionaryValue> args, bool persistent, CefRefPtr<Callback> callback);
    bool onTaskSaveProject(CefRefPtr<CefBrowser> browser, CefRefPtr<CefFrame> frame, int64_t queryId, CefRefPtr<CefDictionaryValue> args, bool persistent, CefRefPtr<Callback> callback);
    bool onTaskOpenImage(CefRefPtr<CefBrowser> browser, CefRefPtr<CefFrame> frame, int64_t queryId, CefRefPtr<CefDictionaryValue> args, bool persistent, CefRefPtr<Callback> callback);
    bool onTaskImportNodes(CefRefPtr<CefBrowser> browser, CefRefPtr<CefFrame> frame, int64_t queryId, CefRefPtr<CefDictionaryValue> args, bool persistent, CefRefPtr<Callback> callback);
    bool onTaskExportNodes(CefRefPtr<CefBrowser> browser, CefRefPtr<CefFrame> frame, int64_t queryId, CefRefPtr<CefDictionaryValue> args, bool persistent, CefRefPtr<Callback> callback);

private:
    FunctionFactory<MenuBarBinding> tasks;
    VolumeViewer* viewer{nullptr};
    DISALLOW_COPY_AND_ASSIGN(MenuBarBinding);
};

} // namespace binding
} // namespace app
