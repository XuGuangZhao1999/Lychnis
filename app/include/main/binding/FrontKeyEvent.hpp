#pragma once

#include <Qt>

#include "include/cef_values.h"

#include <volumeviewercore.h>

namespace app{
    namespace binding{
        Qt::Key keyTable[] = {Qt::Key_A, Qt::Key_B, Qt::Key_D, Qt::Key_F, Qt::Key_R, Qt::Key_V, Qt::Key_X, Qt::Key_Z,
                              Qt::Key_Left, Qt::Key_Right, Qt::Key_Space, Qt::Key_Home, Qt::Key_End, Qt::Key_Up, Qt::Key_Down, 
                              Qt::Key_Delete, Qt::Key_Escape};

        class FrontKeyEvent{
            private:
                Qt::Key m_key;
                Qt::KeyboardModifiers modifiers{Qt::NoModifier};
                bool m_bAutoRepeat;
            public:
                explicit FrontKeyEvent(CefRefPtr<CefDictionaryValue> args){
                    m_key = keyTable[args->GetInt("key")];

                    CefRefPtr<CefDictionaryValue> FrontModifiers = args->GetDictionary("modifier");
                    if(FrontModifiers->GetBool("Shift")){
                        modifiers |= Qt::ShiftModifier;
                    }
                    if(FrontModifiers->GetBool("Control")){
                        modifiers |= Qt::ControlModifier;
                    }

                    m_bAutoRepeat = args->GetBool("bAutoRepeat");
                }

                VolumeViewerCore::MouseKeyEvent eventConvert(){
                    VolumeViewerCore::MouseKeyEvent e;
                    e.m_modifiers = modifiers;
                    e.m_key = m_key;
                    e.m_bAutoRepeat = m_bAutoRepeat;

                    return e;
                }
        };
    }// namespace binding
} // namespace app
