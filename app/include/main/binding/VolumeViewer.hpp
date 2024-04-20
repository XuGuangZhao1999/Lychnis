#pragma once

#include <lychnisreader.h>
#include <volumeviewercore.h>

#include "include/cef_base.h"

namespace app{
    namespace binding{
        class VolumeViewer : public VolumeViewerCore {
        private:
            // New data members
            static std::atomic_bool bProject_ImageLoaded;
        
            // lychnis: ViewerPanel's data members
            LychnisReader* m_project;

            VolumeViewer(LychnisReader& reader);
            ~VolumeViewer();
        public:
            static VolumeViewer& getInstance(const std::string& path);
            static bool isLoaded();
            bool onLoadProject();
            bool onSaveProject(bool bSaveAs, std::string& projectPath);

            // Visual interaction event handlers
            // void mousePressEvent(FrontMouseEvent* event);
            // void mouseMoveEvent(FrontMouseEvent* event);
            // void mouseReleaseEvent(FrontMouseEvent* event);
            // void wheelEvent(FrontWheelEvent* event);
            // void keyPressEvent(FrontKeyEvent* event);
            // void keyReleaseEvent(FrontKeyEvent* event);

            DISALLOW_COPY_AND_ASSIGN(VolumeViewer);
        };
    }// namespace binding
}// namespace app