#pragma once

#include <lychnisreader.h>
#include <volumeviewercore.h>

#include "include/cef_base.h"
#include "include/cef_frame.h"

namespace app{
    namespace binding{
        // Forward declaration.
        struct ImportNodesInfo;
        struct VolumeReloadInfo;
        class VolumeViewer : public VolumeViewerCore {
        private:
            // New data members
            CefRefPtr<CefFrame> m_frame;
            static std::atomic_bool bProject_ImageLoaded;

            // lychnis: VolumeViewer's data members
            cv::Size m_windowSize, m_fixedViewerSize, m_autoViewerSize;
            double m_devicePixelRatio{1}, m_viewerRatio{1};
        
            // lychnis: ViewerPanel's data members
            LychnisReader* m_project;
            bool m_bRunning{true};
            std::atomic_bool m_bUpdated{false};

            int m_blockSize[3]{}, m_blockSize3D[3]{};
            std::thread* m_thread{nullptr};
            
            std::atomic_int m_currentRes3D{-1}, m_nextNodeOperation{-1};
            std::atomic_int m_nextResolution{-1}, m_nextChannel{-1};
            std::atomic<std::string*> m_imagePath2Load{nullptr};
            std::atomic<LychnisProjectReader *> m_projectProto2Load{nullptr};
            std::atomic<ImportNodesInfo *> m_nodesPath2Load{nullptr};
            std::atomic<cv::Point3i*> m_nextBlockSize{nullptr}, m_nextCenter{nullptr};
            std::atomic<cv::Point2i *> m_nextWorldRange{nullptr};
            std::atomic<VolumeReloadInfo *> m_volumeReloadInfo{nullptr};
            std::atomic<LychnisProjectReader::ProjectInfo *> m_projectInfo2Save{nullptr}, m_projectInfo2AutoSave{nullptr};

            VolumeViewer(LychnisReader& reader, CefRefPtr<CefFrame> frame);
            ~VolumeViewer();

            void mainLoop();
            void importNodes();
            void operateNodes();
            void openImageFile();
            void updateBlockSize();
            void updateCenter();
            void updateResolution();
            void reloadVolume();
            void updateChannel();
            void updateBlock();
            void saveProject();

            bool loadBlock(LevelInfo *p,
				 size_t start[],
				 size_t block[],
				 double spacing[],
				 const QList<int> &channels,
				 void *buffer,
				 uint16_t *buffer2);

            void setChannels(const QStringList &names, const QList<cv::Point3d> &colors);
            void markModified();
            void sendBlock2Viewer(void *buffer, double origin[], size_t block[], double spacing[], QList<int> &channels);
            void paintEvent();

        protected:
            void updateScreen() override;

        public:
            static VolumeViewer& getInstance(CefRefPtr<CefFrame> frame);
            static bool isLoaded();
            void onLoadProject(std::string projectPath);
            void onSaveProject(bool bSaveAs, std::string& projectPath);
            QVariantMap exportDisplayParams();

            // Visual interaction event handlers
            // void mousePressEvent(FrontMouseEvent* event);
            // void mouseMoveEvent(FrontMouseEvent* event);
            // void mouseReleaseEvent(FrontMouseEvent* event);
            // void wheelEvent(FrontWheelEvent* event);
            // void keyPressEvent(FrontKeyEvent* event);
            // void keyReleaseEvent(FrontKeyEvent* event);

            void showMessage(const std::string& message);

            DISALLOW_COPY_AND_ASSIGN(VolumeViewer);
        };
    }// namespace binding
}// namespace app