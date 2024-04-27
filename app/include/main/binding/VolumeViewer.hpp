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

            // Volume Information
            int m_totalResolution, m_currentResolution;
            int m_blockSize[3]{};
            cv::Point3i m_center;
            QList<ChannelBarInfo *> m_channelInfos;

            VolumeViewer(LychnisReader& reader);
            ~VolumeViewer();
            void initViewer();
            void updateBlock();
        protected:
        
        public:
            static VolumeViewer& getInstance();
            static bool isLoaded();
            void setProjectPath(const std::string& path);
            void setImagePath(const std::string& path);
            bool onLoadProject();
            bool onOpenImage();
            bool onSaveProject(std::string& projectPath, bool bSaveAs);

            DISALLOW_COPY_AND_ASSIGN(VolumeViewer);
        };
    }// namespace binding
}// namespace app