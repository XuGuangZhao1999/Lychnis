#pragma once

#include "include/cef_frame.h"

namespace shared
{

namespace util
{

class BindingUtil
{

public:
    static void paintEvent(CefRefPtr<CefFrame> frame, CefRefPtr<CefBinaryValue> imageBinary){
        CefRefPtr<CefProcessMessage> msg = CefProcessMessage::Create("Image");
        msg->GetArgumentList()->SetBinary(0, imageBinary);
        frame->SendProcessMessage(PID_RENDERER, msg);
    }

    static void showMessage(CefRefPtr<CefFrame> frame, CefString message){
        CefRefPtr<CefProcessMessage> msg = CefProcessMessage::Create("Message");
        msg->GetArgumentList()->SetString(0, message);
        frame->SendProcessMessage(PID_RENDERER, msg);
    }
};

} // namespace util
} // namespace shared
