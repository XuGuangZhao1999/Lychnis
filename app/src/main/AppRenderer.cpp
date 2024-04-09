#include "main/AppRenderer.hpp"
#include "include/wrapper/cef_message_router.h"
#include "main/v8/AppExtension.hpp"

namespace app
{
class MyReleaseCallback: public CefV8ArrayBufferReleaseCallback{
    public:
        virtual void ReleaseBuffer(void* buffer){
            delete[] static_cast<char*>(buffer);
        }

    IMPLEMENT_REFCOUNTING(MyReleaseCallback);
};

CefRefPtr<CefRenderProcessHandler> RendererApp::GetRenderProcessHandler()
{
    return this;
}

void RendererApp::OnWebKitInitialized()
{
    CefMessageRouterConfig config;
    messageRouter = CefMessageRouterRendererSide::Create(config);

    app::v8::AppExtension::init();
}

void RendererApp::OnContextCreated(CefRefPtr<CefBrowser> browser, CefRefPtr<CefFrame> frame, CefRefPtr<CefV8Context> context)
{
    messageRouter->OnContextCreated(browser, frame, context);
}

void RendererApp::OnContextReleased(CefRefPtr<CefBrowser> browser, CefRefPtr<CefFrame> frame, CefRefPtr<CefV8Context> context)
{
    messageRouter->OnContextReleased(browser, frame, context);
}

bool RendererApp::OnProcessMessageReceived(CefRefPtr<CefBrowser> browser, CefRefPtr<CefFrame> frame, CefProcessId source_process, CefRefPtr<CefProcessMessage> message)
{
    if(message->GetName() == "Image"){
        CefRefPtr<CefV8Context> context = frame->GetV8Context();
        context->Enter();
        CefRefPtr<CefListValue> args = message->GetArgumentList();
        
        auto binaryValue = args->GetBinary(0);
        size_t size = binaryValue->GetSize();
        char* data = new char[size];
        binaryValue->GetData(data, size, 0);
        
        CefRefPtr<CefV8Value> arrayBuffer = CefV8Value::CreateArrayBuffer(data, size, new MyReleaseCallback());
        CefRefPtr<CefV8Value> global = context->GetGlobal();
        CefRefPtr<CefV8Value> paintFunction = global->GetValue("paint");

        if(paintFunction.get() && paintFunction->IsFunction()){
            CefV8ValueList args;
            args.push_back(arrayBuffer);
            paintFunction->ExecuteFunction(global, args);
        }
        context->Exit();

        return true;
    }


    return messageRouter->OnProcessMessageReceived(browser, frame, source_process, message);
}

} // namespace app
