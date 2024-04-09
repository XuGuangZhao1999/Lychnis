#pragma once

#include <lychnisreader.h>
#include <volumeviewercore.h>

#include "include/cef_base.h"

namespace app{
    namespace binding{
        struct ImportNodesInfo;
        class VolumeViewer {
        private:
            LychnisReader reader;
            VolumeViewerCore* viewer{nullptr};
            std::thread* m_thread{nullptr};
            bool m_bRunning{true};
            std::atomic<std::string*> m_imagePath2Load{nullptr};
            std::atomic<LychnisProjectReader *> m_projectProto2Load{nullptr};
            std::atomic<ImportNodesInfo *> m_nodesPath2Load{nullptr};
            std::atomic_int m_nextResolution{-1}, m_nextChannel{-1};


            VolumeViewer();
            ~VolumeViewer();

            void mainLoop();
            void openImageFile();
            void importNodes();
            void operateNodes();
            void updateBlockSize();
            void updateCenter();
            void updateResolution();
            void reloadVolume();
            void updateChannel();
            void updateBlock();
            void exportVolume();
            void loadVolumeROI();
            void saveProject();

            std::string getOpenFileName();
        public:
            std::string m_projectPath;
            static VolumeViewer& getInstance();
            LychnisReader& getReader(std::string filePath);
            VolumeViewerCore& getViewer();
            bool onLoadProject(std::string& projectPath);

            DISALLOW_COPY_AND_ASSIGN(VolumeViewer);
        };
    }// namespace binding
}// namespace app