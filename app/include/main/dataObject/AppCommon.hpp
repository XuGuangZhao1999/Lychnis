#ifndef APP_COMMON
#define APP_COMMON

#include <lychnisreader.h>
#include <volumeviewercore.h>
#include "include/cef_base.h"

namespace app{
    namespace dataObject{
        class AppCommon : public CefBaseRefCounted
        {
        public:
            static AppCommon& getInstance();
            LychnisReader& getReader(std::string filePath);
            VolumeViewerCore& getViewer();
            void rlease();
        private:
            LychnisReader* reader;
            VolumeViewerCore* viewer;

            AppCommon();
            ~AppCommon();

            IMPLEMENT_REFCOUNTING(AppCommon);
            DISALLOW_COPY_AND_ASSIGN(AppCommon);
        };
    }// namespace dataObject
}// namespace app

#endif