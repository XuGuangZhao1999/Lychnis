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
            bool bResUpdated{false};
            int visableChannelCount{0};

            // lychnis: ViewerPanel's data members
            LychnisReader* m_project{nullptr};
            static std::string m_imagePath2Load;
            static std::string m_projectPath2Load;
            LychnisProjectReader* m_projectProto2Load{nullptr};
            ImportNodesInfo* m_nodesPath2Load{nullptr};

            // Volume Information
            int m_totalResolutions, m_currentResolution;
            int m_blockSize[3]{};
            QList<ChannelBarInfo *> m_channelInfos;
            int m_currentChannel{0};
            cv::Point2i m_worldRange{100, 1000};

            VolumeViewer(LychnisReader& reader);
            ~VolumeViewer();
            bool initViewer();
            void importNodes();
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
            void sendVisualInfo();
            void keyPressKernel(int) override;
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
            void updateChannel(int channelId, int lower, int upper);
            void updateBlockSize(int x, int y, int z);
            void updateCenter(int x, int y, int z);
            void updateChannelColor(int index, double r, double g, double b, double gamma);
            void updateChannelVisibility(int index, bool bVisible);
            void updateChannelContrast(int lower, int upper);

            bool pickVolume(int x, int y) override;

            IMPLEMENT_REFCOUNTING(VolumeViewer);
            DISALLOW_COPY_AND_ASSIGN(VolumeViewer);
        };
    }// namespace binding
}// namespace app