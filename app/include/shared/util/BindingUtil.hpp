#pragma once

#include "include/cef_frame.h"

#include <opencv2/shape/shape_transformer.hpp>

#include "main/binding/VolumeViewer.hpp"

namespace shared
{

namespace util
{

class BindingUtil
{

public:
    static void paintEvent(CefRefPtr<CefFrame> frame, app::binding::VolumeViewer& viewer){
        cv::Mat image = viewer.renderToImage();
        viewer.setScaleNum(image);
                
        std::vector<uchar> tempBuf;
        cv::imencode(".jpg", image, tempBuf);

        CefRefPtr<CefBinaryValue> imageBinary = CefBinaryValue::Create(tempBuf.data(), tempBuf.size()*sizeof(uchar));

        CefRefPtr<CefProcessMessage> msg = CefProcessMessage::Create("Image");
        msg->GetArgumentList()->SetBinary(0, imageBinary);
        frame->SendProcessMessage(PID_RENDERER, msg);
    }

    static void showMessage(CefRefPtr<CefFrame> frame, CefString& message){
        CefRefPtr<CefProcessMessage> msg = CefProcessMessage::Create("Message");
        msg->GetArgumentList()->SetString(0, message);
        frame->SendProcessMessage(PID_RENDERER, msg);
    }

    static void setInfos(CefRefPtr<CefFrame> frame, CefString& infos){
        CefRefPtr<CefProcessMessage> msg = CefProcessMessage::Create("Infos");
        msg->GetArgumentList()->SetString(0, infos);
        frame->SendProcessMessage(PID_RENDERER, msg);
    }
};

} // namespace util
} // namespace shared
