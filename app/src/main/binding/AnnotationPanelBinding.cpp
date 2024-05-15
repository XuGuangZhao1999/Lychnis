#include "main/binding/AnnotationPanelBinding.hpp"
#include "shared/AppConfig.hpp"
#include "shared/util/BindingUtil.hpp"

#include <algorithm>
#include <vector>

#include <opencv2/shape/shape_transformer.hpp>

#include "include/cef_parser.h"
#include "include/cef_browser.h"
#include "include/cef_values.h"

namespace app{
    namespace binding{
        AnnotationPanelBinding::AnnotationPanelBinding(){
            viewer = &VolumeViewer::getInstance();

            tasks.RegisterFunction("updateBlockSize", &AnnotationPanelBinding::onTaskUpdateBlockSize);
            tasks.RegisterFunction("updateCenter", &AnnotationPanelBinding::onTaskUpdateCenter);
        }

        bool AnnotationPanelBinding::OnQuery(CefRefPtr<CefBrowser> browser, CefRefPtr<CefFrame> frame, int64_t queryId, const CefString &request, bool persistent, CefRefPtr<Callback> callback){
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


        bool AnnotationPanelBinding::onTaskUpdateBlockSize(CefRefPtr<CefBrowser> browser, CefRefPtr<CefFrame> frame, int64_t queryId, CefRefPtr<CefDictionaryValue> args, bool persistent, CefRefPtr<Callback> callback){
            auto blockSize = args->GetDictionary("blockSize");
            viewer->updateBlockSize(blockSize->GetInt("width"), blockSize->GetInt("width"), blockSize->GetInt("height"));

            callback->Success("Success");
            return true;
        }

        bool AnnotationPanelBinding::onTaskUpdateCenter(CefRefPtr<CefBrowser> browser, CefRefPtr<CefFrame> frame, int64_t queryId, CefRefPtr<CefDictionaryValue> args, bool persistent, CefRefPtr<Callback> callback){
            auto center = args->GetDictionary("center");
            viewer->updateCenter(center->GetInt("x"), center->GetInt("y"), center->GetInt("z"));

            callback->Success("Success");
            return true;
        }

        bool AnnotationPanelBinding::onTaskUpdateChannel(CefRefPtr<CefBrowser> browser, CefRefPtr<CefFrame> frame, int64_t queryId, CefRefPtr<CefDictionaryValue> args, bool persistent, CefRefPtr<Callback> callback){
            
            return true;
        }
        
        void AnnotationPanelBinding::init(CefRefPtr<CefMessageRouterBrowserSide> router){
            router->AddHandler(new AnnotationPanelBinding(), false);
        }
    }// namespace binding
}// namespace app