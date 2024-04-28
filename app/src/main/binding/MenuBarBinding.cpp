#include "main/binding/MenuBarBinding.hpp"
#include "main/net/RequestClient.hpp"
#include "shared/AppConfig.hpp"
#include "shared/util/BindingUtil.hpp"

#include <algorithm>
#include <vector>

#include <channelbarinfo.h>
#include <animationframeio.h>

#include <opencv2/shape/shape_transformer.hpp>

#include "include/cef_parser.h"
#include "include/cef_browser.h"
#include "include/cef_values.h"

namespace app
{
namespace binding
{

class FileDialogCallback : public CefRunFileDialogCallback{
    private:
        CefRefPtr<CefFrame> m_frame;
        CefRefPtr<CefMessageRouterBrowserSide::Callback> m_callback;
        bool m_bProject;
    public:
        FileDialogCallback(CefRefPtr<CefFrame> frame, CefRefPtr<CefMessageRouterBrowserSide::Callback> callback, bool bProject):m_frame{frame}, m_callback{callback}, m_bProject{bProject}{};

        void OnFileDialogDismissed (const std::vector< CefString > &file_paths){
            if(1 == file_paths.size()){
                VolumeViewer& vvInstance = VolumeViewer::getInstance();
                vvInstance.setFrame(m_frame);
                if(m_bProject){
                    vvInstance.setProjectPath(file_paths[0]);
                    vvInstance.onLoadProject();
                }else{
                    vvInstance.setImagePath(file_paths[0]);
                    vvInstance.onOpenImage();
                }
                
                shared::util::BindingUtil::paintEvent(m_frame, vvInstance);

                m_callback->Success("图像加载成功");
            }
        }

        FileDialogCallback(const FileDialogCallback&) = delete;
        FileDialogCallback& operator=(const FileDialogCallback&) = delete;

        IMPLEMENT_REFCOUNTING(FileDialogCallback);
};

MenuBarBinding::MenuBarBinding(){
    viewer = &VolumeViewer::getInstance();

    tasks.RegisterFunction("loadProject", &MenuBarBinding::onTaskLoadProject);
    tasks.RegisterFunction("saveProject", &MenuBarBinding::onTaskSaveProject);
    tasks.RegisterFunction("openImage", &MenuBarBinding::onTaskOpenImage);
    tasks.RegisterFunction("importNodes", &MenuBarBinding::onTaskImportNodes);
    tasks.RegisterFunction("exportNodes", &MenuBarBinding::onTaskExportNodes);
}

bool MenuBarBinding::OnQuery(CefRefPtr<CefBrowser> browser, CefRefPtr<CefFrame> frame, int64_t queryId, const CefString &request, bool persistent, CefRefPtr<Callback> callback)
{
    viewer->setFrame(frame);
    // Only handle messages from application url
    const std::string &url = frame->GetURL();

    if (!(url.rfind(APP_ORIGIN, 0) == 0))
    {
        return false;
    }

    // Parse request
    auto task = CefParseJSON(request, JSON_PARSER_RFC)->GetDictionary();
    auto func = tasks.GetFunction(task->GetString("functionName"));

    if(nullptr != func){
        auto args = task->GetDictionary("args");
        return (this->*func)(browser, frame, queryId, args, persistent, callback);
    }

    return false;
}

bool MenuBarBinding::onTaskLoadProject(CefRefPtr<CefBrowser> browser, CefRefPtr<CefFrame> frame, int64_t queryId, CefRefPtr<CefDictionaryValue> args, bool persistent, CefRefPtr<Callback> callback){
    if(!VolumeViewer::isLoaded()){
        std::vector<CefString> filter={".lyp", ".lyp2", ".json"};
        CefRefPtr<CefRunFileDialogCallback> cbk = new FileDialogCallback(frame, callback, true);
        browser->GetHost()->RunFileDialog(FILE_DIALOG_OPEN, "Please select a project file", "", filter, cbk);
    }

    return true;
}

bool MenuBarBinding::onTaskSaveProject(CefRefPtr<CefBrowser> browser, CefRefPtr<CefFrame> frame, int64_t queryId, CefRefPtr<CefDictionaryValue> args, bool persistent, CefRefPtr<Callback> callback){
    if(VolumeViewer::isLoaded()){

    }

    return true;
}

bool MenuBarBinding::onTaskImportNodes(CefRefPtr<CefBrowser> browser, CefRefPtr<CefFrame> frame, int64_t queryId, CefRefPtr<CefDictionaryValue> args, bool persistent, CefRefPtr<Callback> callback){
    if(VolumeViewer::isLoaded()){

    }
    
    return true;
}

bool MenuBarBinding::onTaskExportNodes(CefRefPtr<CefBrowser> browser, CefRefPtr<CefFrame> frame, int64_t queryId, CefRefPtr<CefDictionaryValue> args, bool persistent, CefRefPtr<Callback> callback){
    if(VolumeViewer::isLoaded()){
        
    }
    
    return true;
}

bool MenuBarBinding::onTaskOpenImage(CefRefPtr<CefBrowser> browser, CefRefPtr<CefFrame> frame, int64_t queryId, CefRefPtr<CefDictionaryValue> args, bool persistent, CefRefPtr<Callback> callback){
    if(!VolumeViewer::isLoaded()){
        std::vector<CefString> filter = {".lym", ".ims", ".h5", ".json"};
        CefRefPtr<CefRunFileDialogCallback> cbk = new FileDialogCallback(frame, callback, false);
        browser->GetHost()->RunFileDialog(FILE_DIALOG_OPEN, "Please select a image file", "", filter, cbk);
    }

    return true;
}

void MenuBarBinding::init(CefRefPtr<CefMessageRouterBrowserSide> router)
{
    router->AddHandler(new MenuBarBinding(), false);
}

} // namespace binding
} // namespace app
