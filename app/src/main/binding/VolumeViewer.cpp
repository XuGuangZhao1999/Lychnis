#include "main/binding/VolumeViewer.hpp"
#include "shared/util/BindingUtil.hpp"

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
            for(auto it: m_blockSize){
                it = 256;
            }
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

            m_totalResolution = (m_project->m_imageReader->getLevelIndexes()).size();
            m_currentResolution = m_totalResolution - 1;
            auto level = m_project->m_imageReader->getLevel(m_currentResolution);

            double bounds[6];
            for (int i = 0; i < 3; i++) {
                auto v1 = ((double *)&m_project->m_origin)[i], v2 = v1 + m_project->m_dims[i];
                bounds[2 * i] = v1;
                bounds[2 * i + 1] = v2;
            }
            setBounds(bounds);

            auto channelList = projectInfo["channels"].toList();
            const auto numChannels = level->dataIds[0].size();
            if (channelList.size() == numChannels) {
                for (auto &channel : channelList) { m_channelInfos.append(new ChannelBarInfo(channel.toMap())); }
            } else { for (int i = 0; i < numChannels; i++) { m_channelInfos.append(nullptr); }}
            setChannelInfos(m_channelInfos);
            setShowScaleBar(false);

            updateBlock();
        }

        void VolumeViewer::updateBlock(){
            auto level = m_project->m_imageReader->getLevel(m_currentResolution);
            const auto numChannels = level->dataIds[0].size();
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

            m_center.x = m_volumeInfo->center[0];
            m_center.y = m_volumeInfo->center[1];
            m_center.z = m_volumeInfo->center[2];
        }

        void VolumeViewer::updateScreen(){
            shared::util::BindingUtil::paintEvent(m_frame, *this);
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

        void VolumeViewer::setFrame(CefRefPtr<CefFrame> frame){
            if(m_frame == nullptr){
                m_frame = frame;
            }
        }
    }// namespace binding
}// namespace app