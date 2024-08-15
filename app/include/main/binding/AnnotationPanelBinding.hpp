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
class AnnotationPanelBinding : public CefMessageRouterBrowserSide::Handler
{
public:
    AnnotationPanelBinding();
    virtual ~AnnotationPanelBinding() {}

    // called due to cefQuery execution
    virtual bool OnQuery(CefRefPtr<CefBrowser> browser, CefRefPtr<CefFrame> frame, int64_t queryId, const CefString &request, bool persistent, CefRefPtr<Callback> callback) override;

    // called on AppClient
    static void init(CefRefPtr<CefMessageRouterBrowserSide> router);

    // tasks
    bool onTaskUpdateBlockSize(CefRefPtr<CefBrowser> browser, CefRefPtr<CefFrame> frame, int64_t queryId, CefRefPtr<CefDictionaryValue> args, bool persistent, CefRefPtr<Callback> callback);
    bool onTaskUpdateCenter(CefRefPtr<CefBrowser> browser, CefRefPtr<CefFrame> frame, int64_t queryId, CefRefPtr<CefDictionaryValue> args, bool persistent, CefRefPtr<Callback> callback);
    bool onTaskUpdateUsername(CefRefPtr<CefBrowser> browser, CefRefPtr<CefFrame> frame, int64_t queryId, CefRefPtr<CefDictionaryValue> args, bool persistent, CefRefPtr<Callback> callback);

private:
    FunctionFactory<AnnotationPanelBinding> tasks;
    CefRefPtr<VolumeViewer> viewer{nullptr};
    DISALLOW_COPY_AND_ASSIGN(AnnotationPanelBinding);
};

} // namespace binding
} // namespace app
