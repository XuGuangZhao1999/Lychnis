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
class ViewerPanelBinding : public CefMessageRouterBrowserSide::Handler
{
public:
    ViewerPanelBinding();
    virtual ~ViewerPanelBinding() {}

    // called due to cefQuery execution
    virtual bool OnQuery(CefRefPtr<CefBrowser> browser, CefRefPtr<CefFrame> frame, int64_t queryId, const CefString &request, bool persistent, CefRefPtr<Callback> callback) override;

    // called on AppClient
    static void init(CefRefPtr<CefMessageRouterBrowserSide> router);

    // tasks
    bool onTaskMousePressEvent(CefRefPtr<CefBrowser> browser, CefRefPtr<CefFrame> frame, int64_t queryId, CefRefPtr<CefDictionaryValue> args, bool persistent, CefRefPtr<Callback> callback);
    bool onTaskMouseMoveEvent(CefRefPtr<CefBrowser> browser, CefRefPtr<CefFrame> frame, int64_t queryId, CefRefPtr<CefDictionaryValue> args, bool persistent, CefRefPtr<Callback> callback);
    bool onTaskMouseReleaseEvent(CefRefPtr<CefBrowser> browser, CefRefPtr<CefFrame> frame, int64_t queryId, CefRefPtr<CefDictionaryValue> args, bool persistent, CefRefPtr<Callback> callback);
    bool onTaskWheelEvent(CefRefPtr<CefBrowser> browser, CefRefPtr<CefFrame> frame, int64_t queryId, CefRefPtr<CefDictionaryValue> args, bool persistent, CefRefPtr<Callback> callback);

private:
    FunctionFactory<ViewerPanelBinding> tasks;
    VolumeViewer* viewer{nullptr};
    DISALLOW_COPY_AND_ASSIGN(ViewerPanelBinding);
};

} // namespace binding
} // namespace app
