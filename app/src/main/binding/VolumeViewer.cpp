#include "main/binding/VolumeViewer.hpp"

#include <commdlg.h>
#include <tchar.h>
#include <chrono>
#include <vector>

#include <lychnisreaderfactory.h>
#include <animationframe.h>
#include "common.h"
#include "channelbarinfo.h"

#include <QFileInfo>
#include <QDebug>

namespace app{
    namespace binding{
        bool VolumeViewer::bProject_ImageLoaded = false;
        std::string VolumeViewer::m_imagePath2Load = "";
        std::string VolumeViewer::m_projectPath2Load = "";

        VolumeViewer::VolumeViewer(LychnisReader& reader) : VolumeViewerCore(&reader) {
            m_project = &reader;
        }

        VolumeViewer::~VolumeViewer(){
            bProject_ImageLoaded = false;
        }

        void VolumeViewer::initViewer(){
            bProject_ImageLoaded = true;

            cv::Size viewerSize(1600, 1200);
            resizeEvent(viewerSize, 1);

            double offset[] = {0, 0, 0};
            QVariantMap projectInfo = m_project->m_loadedProjectInfo;
            importNodes(projectInfo, m_project->m_voxelSize, offset);

            int levelSize = (m_project->m_imageReader->getLevelIndexes()).size();
            auto level = m_project->m_imageReader->getLevel(levelSize / 2);

            double bounds[6];
            for (int i = 0; i < 3; i++) {
                auto v1 = ((double *)&m_project->m_origin)[i], v2 = v1 + m_project->m_dims[i];
                bounds[2 * i] = v1;
                bounds[2 * i + 1] = v2;
            }
            setBounds(bounds);

            QList<ChannelBarInfo *> channelInfos;
            auto channelList = projectInfo["channels"].toList();
            const auto numChannels = level->dataIds[0].size();
            if (channelList.size() == numChannels) {
                for (auto &channel : channelList) { channelInfos.append(new ChannelBarInfo(channel.toMap())); }
            } else { for (int i = 0; i < numChannels; i++) { channelInfos.append(nullptr); }}
            setChannelInfos(channelInfos);

            AutoBuffer buffer, singleChanBuffer, dstBuffer;
            size_t numVoxels = level->dims[2] * level->dims[1] * level->dims[0];
            auto singleChanBuf = (uint16_t *)singleChanBuffer.buffer(numVoxels * 2);
            auto buf = (uint16_t *)buffer.buffer(singleChanBuffer.length() * numChannels);

            size_t start[] = {0, 0, 0};
            QList<int> channels;
            for (int i = 0; i < numChannels; i++) {
                m_project->m_imageReader->readBlock(level, start, level->dims, singleChanBuf, 0, i);
                uint16_t *pBuffer = buf + i, *pBuffer2 = singleChanBuf;
                for (size_t j = 0; j < numVoxels; j++, pBuffer2++, pBuffer += numChannels) { *pBuffer = *pBuffer2; }
                channels.append(i);
            }

            size_t dims[3]; 
            double spacing[3], origin[3];
            for (int i = 0; i < 3; i++) {
                dims[i] = level->dims[2 - i]; // zyx to xyz
                spacing[i] = level->spacing[2 - i] * ((double *)&m_project->m_voxelSize)[i];
                origin[i] = (double)start[2 - i] * spacing[i] + ((double *)&m_project->m_origin)[i];
            }
            setVolume(buf, dims, spacing, origin, channels, true, false);
            setShowScaleBar(false);
        }

        bool VolumeViewer::onLoadProject(){
            if(!m_project->load(m_projectPath2Load)){
                return false;
            }

            if(!m_project->openImage()){
                return false;
            }
            initViewer();

            return true;
        }

        bool VolumeViewer::onSaveProject(std::string& projectPath, bool bSaveAs){
            return true;
        }

        bool VolumeViewer::onOpenImage(){
            if(!m_project->openImage(m_imagePath2Load)){
                return false;
            }
            initViewer();

            return true;
        }

        bool VolumeViewer::isLoaded(){
            return bProject_ImageLoaded;
        }

        VolumeViewer& VolumeViewer::getInstance(){
            static LychnisReader reader;
            static VolumeViewer instance(reader);

            return instance;
        }

        void VolumeViewer::setProjectPath(const std::string& path){
            if(!isLoaded()){
                m_projectPath2Load = path;
            }
        }

        void VolumeViewer::setImagePath(const std::string& path){
            if(!isLoaded()){
                m_imagePath2Load = path;
            }
        }
    }// namespace binding
}// namespace app