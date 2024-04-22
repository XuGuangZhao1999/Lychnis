#pragma once

#include <lychnisreader.h>
#include <volumeviewercore.h>

#include "include/cef_base.h"

namespace app{
    namespace binding{
        class VolumeViewer : public VolumeViewerCore {
        private:
            // New data members
            static bool bProject_ImageLoaded;
        
            // lychnis: ViewerPanel's data members
            LychnisReader* m_project;
            static std::string m_imagePath2Load;
            static std::string m_projectPath2Load;

            VolumeViewer(LychnisReader& reader);
            ~VolumeViewer();
            void initViewer();
        public:
            static VolumeViewer& getInstance();
            static bool isLoaded();
            void setProjectPath(const std::string& path);
            void setImagePath(const std::string& path);
            bool onLoadProject();
            bool onOpenImage();
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