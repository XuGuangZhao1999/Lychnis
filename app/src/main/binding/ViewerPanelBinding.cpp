#include "main/binding/ViewerPanelBinding.hpp"
#include "shared/AppConfig.hpp"
#include "shared/util/BindingUtil.hpp"
#include "main/binding/FrontMouseEvent.hpp"
#include "main/binding/FrontKeyEvent.hpp"

#include <algorithm>
#include <vector>

#include <opencv2/shape/shape_transformer.hpp>

#include "include/cef_parser.h"
#include "include/cef_browser.h"
#include "include/cef_values.h"

namespace app{
    namespace binding{
        ViewerPanelBinding::ViewerPanelBinding(){
            viewer = &VolumeViewer::getInstance();

            tasks.RegisterFunction("mousePressEvent", &ViewerPanelBinding::onTaskMousePressEvent);
            tasks.RegisterFunction("mouseMoveEvent", &ViewerPanelBinding::onTaskMouseMoveEvent);
            tasks.RegisterFunction("mouseReleaseEvent", &ViewerPanelBinding::onTaskMouseReleaseEvent);
            tasks.RegisterFunction("wheelEvent", &ViewerPanelBinding::onTaskWheelEvent);
            tasks.RegisterFunction("keyPressEvent", &ViewerPanelBinding::onTaskKeyPressEvent);
            tasks.RegisterFunction("keyReleaseEvent", &ViewerPanelBinding::onTaskKeyReleaseEvent);
            
            tasks.RegisterFunction("updateResolution", &ViewerPanelBinding::onTaskUpdateResolution);
            tasks.RegisterFunction("updateChannelColor", &ViewerPanelBinding::onTaskUpdateChannelColor);
            tasks.RegisterFunction("updateChannelVisibility", &ViewerPanelBinding::onTaskUpdateChannelVisibility);
            tasks.RegisterFunction("updateChannelContrast",&ViewerPanelBinding::onTaskUpdateChannelContrast);
        }

        bool ViewerPanelBinding::OnQuery(CefRefPtr<CefBrowser> browser, CefRefPtr<CefFrame> frame, int64_t queryId, const CefString &request, bool persistent, CefRefPtr<Callback> callback){
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

        bool ViewerPanelBinding::onTaskMousePressEvent(CefRefPtr<CefBrowser> browser, CefRefPtr<CefFrame> frame, int64_t queryId, CefRefPtr<CefDictionaryValue> args, bool persistent, CefRefPtr<Callback> callback){
            FrontMouseEvent e(args);
            VolumeViewerCore::MouseKeyEvent e1 = e.eventConvert();
            viewer->mousePressEvent(&e1);

            // shared::util::BindingUtil::paintEvent(frame, *viewer);
            callback->Success("Success");
            return true;
        }

        bool ViewerPanelBinding::onTaskMouseMoveEvent(CefRefPtr<CefBrowser> browser, CefRefPtr<CefFrame> frame, int64_t queryId, CefRefPtr<CefDictionaryValue> args, bool persistent, CefRefPtr<Callback> callback){
            FrontMouseEvent e(args);
            VolumeViewerCore::MouseKeyEvent e1 = e.eventConvert();
            viewer->mouseMoveEvent(&e1);

            // shared::util::BindingUtil::paintEvent(frame, *viewer);
            callback->Success("Success");
            return true;
        }

        bool ViewerPanelBinding::onTaskMouseReleaseEvent(CefRefPtr<CefBrowser> browser, CefRefPtr<CefFrame> frame, int64_t queryId, CefRefPtr<CefDictionaryValue> args, bool persistent, CefRefPtr<Callback> callback){
            FrontMouseEvent e(args);
            VolumeViewerCore::MouseKeyEvent e1 = e.eventConvert();
            viewer->mouseReleaseEvent(&e1);

            // shared::util::BindingUtil::paintEvent(frame, *viewer);
            callback->Success("Success");
            return true;
        }

        bool ViewerPanelBinding::onTaskWheelEvent(CefRefPtr<CefBrowser> browser, CefRefPtr<CefFrame> frame, int64_t queryId, CefRefPtr<CefDictionaryValue> args, bool persistent, CefRefPtr<Callback> callback){
            VolumeViewerCore::MouseKeyEvent e1;
            e1.m_angleDelta = args->GetInt("deltaY");
            e1.m_x = args->GetDouble("posX");
            e1.m_y = args->GetDouble("posY");
            viewer->wheelEvent(&e1);
            
            // shared::util::BindingUtil::paintEvent(frame, *viewer);
            callback->Success("Success");
            return true;
        }

        bool ViewerPanelBinding::onTaskKeyPressEvent(CefRefPtr<CefBrowser> browser, CefRefPtr<CefFrame> frame, int64_t queryId, CefRefPtr<CefDictionaryValue> args, bool persistent, CefRefPtr<Callback> callback){
            FrontKeyEvent e(args);
            VolumeViewerCore::MouseKeyEvent e1 = e.eventConvert();
            viewer->keyPressEvent(&e1);

            callback->Success("Success");
            return true;
        }

        bool ViewerPanelBinding::onTaskKeyReleaseEvent(CefRefPtr<CefBrowser> browser, CefRefPtr<CefFrame> frame, int64_t queryId, CefRefPtr<CefDictionaryValue> args, bool persistent, CefRefPtr<Callback> callback){
            FrontKeyEvent e(args);
            VolumeViewerCore::MouseKeyEvent e1 = e.eventConvert();
            viewer->keyReleaseEvent(&e1); 

            callback->Success("Success");
            return true;
        }

        bool ViewerPanelBinding::onTaskUpdateResolution(CefRefPtr<CefBrowser> browser, CefRefPtr<CefFrame> frame, int64_t queryId, CefRefPtr<CefDictionaryValue> args, bool persistent, CefRefPtr<Callback> callback){
            int resId = args->GetInt("resId") - 1;
            viewer->updateResolution(resId);

            callback->Success("Success");
            return true;
        }

        bool ViewerPanelBinding::onTaskUpdateChannel(CefRefPtr<CefBrowser> browser, CefRefPtr<CefFrame> frame, int64_t queryId, CefRefPtr<CefDictionaryValue> args, bool persistent, CefRefPtr<Callback> callback){
            
            return true;
        }

        bool ViewerPanelBinding::onTaskUpdateChannelColor(CefRefPtr<CefBrowser> browser, CefRefPtr<CefFrame> frame, int64_t queryId, CefRefPtr<CefDictionaryValue> args, bool persistent, CefRefPtr<Callback> callback){
            viewer->updateChannelColor(args->GetInt("index"), args->GetInt("r")/255.0, args->GetInt("g")/255.0, args->GetInt("b")/255.0, args->GetDouble("gamma"));

            callback->Success("Success");
            return true;
        }

        bool ViewerPanelBinding::onTaskUpdateChannelVisibility(CefRefPtr<CefBrowser> browser, CefRefPtr<CefFrame> frame, int64_t queryId, CefRefPtr<CefDictionaryValue> args, bool persistent, CefRefPtr<Callback> callback){
            viewer->updateChannelVisibility(args->GetInt("index"), args->GetBool("visible"));

            callback->Success("Success");
            return true;
        }
        
        bool ViewerPanelBinding::onTaskUpdateChannelContrast(CefRefPtr<CefBrowser> browser, CefRefPtr<CefFrame> frame, int64_t queryId, CefRefPtr<CefDictionaryValue> args, bool persistent, CefRefPtr<Callback> callback){
            viewer->updateChannelContrast(args->GetInt("lower"), args->GetInt("upper"));

            callback->Success("Success");
            return true;
        }

        void ViewerPanelBinding::init(CefRefPtr<CefMessageRouterBrowserSide> router){
            router->AddHandler(new ViewerPanelBinding(), false);
        }
    }// namespace binding
}// namespace app