#pragma once

#include <lychnisreader.h>
#include <volumeviewercore.h>

#include "include/cef_base.h"
#include "include/cef_frame.h"

namespace app{
    namespace binding{
        struct ImportNodesInfo;
        class VolumeViewer : public VolumeViewerCore, public CefBaseRefCounted {
        private:
            // New data members
            static bool bProject_ImageLoaded;
            CefRefPtr<CefFrame> m_frame{nullptr};
        
            // lychnis: ViewerPanel's data members
            LychnisReader* m_project{nullptr};
            static std::string m_imagePath2Load;
            static std::string m_projectPath2Load;
            LychnisProjectReader* m_projectProto2Load{nullptr};
            ImportNodesInfo* m_nodesPath2Load{nullptr};

            // Volume Information
            int m_totalResolution, m_currentResolution;
            int m_blockSize[3]{};
            cv::Point3i m_center;
            QList<ChannelBarInfo *> m_channelInfos;

            VolumeViewer(LychnisReader& reader);
            ~VolumeViewer();
            bool initViewer();
            bool openImageFile();
            void updateBlock();
            void setChannels(const QStringList &names, const QList<cv::Point3d> &colors);
            bool loadBlock(LevelInfo *p,
                size_t start[],
                size_t block[],
                double spacing[],
                const QList<int> &channels,
                void *buffer,
                uint16_t *buffer2);
            void sendBlock2Viewer(void *buffer, double origin[], size_t block[], double spacing[], QList<int> &channels);
        protected:
            void updateScreen() override;
            void fristChangeCurrentNode(LychnisNode *);
        public:
            static VolumeViewer& getInstance();
            static bool isLoaded();
            void setFrame(CefRefPtr<CefFrame> frame);
            void setProjectPath(const std::string& path);
            void setImagePath(const std::string& path);
            bool onLoadProject();
            bool onOpenImage();
            bool onSaveProject(std::string& projectPath, bool bSaveAs);
            void updateResolution(int resId);

            IMPLEMENT_REFCOUNTING(VolumeViewer);
            DISALLOW_COPY_AND_ASSIGN(VolumeViewer);
        };
    }// namespace binding
}// namespace app