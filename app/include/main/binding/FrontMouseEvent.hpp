#pragma once

#include <Qt>

#include "include/cef_values.h"

#include <volumeviewercore.h>

namespace app{
    namespace binding{
        class FrontMouseEvent{
            private:
                Qt::MouseButton mouseButton{Qt::NoButton};
                Qt::KeyboardModifiers modifiers{Qt::NoModifier};
                int x;
                int y;
            public:
                explicit FrontMouseEvent(CefRefPtr<CefDictionaryValue> args){
                    x = args->GetDouble("posX");
                    y = args->GetDouble("posY");

                    switch (args->GetInt("button"))
                    {
                    case 0:
                        mouseButton = Qt::LeftButton;
                        break;
                    case 1:
                        mouseButton = Qt::MiddleButton;
                        break;
                    case 2:
                        mouseButton = Qt::RightButton;
                        break;
                    default:
                        break;
                    }

                    if(args->GetBool("Shift")){
                        modifiers |= Qt::ShiftModifier;
                    }
                    if(args->GetBool("Control")){
                        modifiers |= Qt::ControlModifier;
                    }
                }

                VolumeViewerCore::MouseKeyEvent eventConvert(){
                    VolumeViewerCore::MouseKeyEvent e;
                    e.m_modifiers = modifiers;
                    e.m_buttons = mouseButton;
                    e.m_x = x;
                    e.m_y = y;

                    return e;
                }
        };
    }// namespace binding
} // namespace app
