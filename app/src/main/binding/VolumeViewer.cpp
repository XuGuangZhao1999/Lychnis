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
        struct VolumeReloadInfo {
            cv::Point3d origin, size;
            double pixelSize{};
            bool b3d, bChanging;
            
            VolumeReloadInfo() : b3d(true), bChanging(false) {}
        };
        struct ImportNodesInfo {
            QStringList paths;
            bool bMerge{true};
            explicit ImportNodesInfo(const QString &p) { paths.append(p); }
            ImportNodesInfo() = default;
        };


        std::atomic_bool VolumeViewer::bProject_ImageLoaded = false;

        VolumeViewer::VolumeViewer(LychnisReader& reader, CefRefPtr<CefFrame> frame) : VolumeViewerCore(&reader), m_frame(frame){
            m_project = &reader;
            // Start the main loop.
            m_thread = new std::thread(&VolumeViewer::mainLoop, this);
        }

        VolumeViewer::~VolumeViewer(){
            // Stop the main loop.
            m_bRunning = false;
            m_thread->join();
        }

        void VolumeViewer::mainLoop(){
            // Periodic cycles.
            while(m_bRunning){
                auto start = std::chrono::high_resolution_clock::now();

                if(nullptr == m_project->m_imageReader){
                    openImageFile();
                }

                if(nullptr != m_project->m_imageReader){
                    importNodes();
                    operateNodes();
                    updateBlockSize();
                    updateCenter();
                    updateResolution();
                    reloadVolume();
                    updateChannel();
                    updateBlock();
                    saveProject();
                }
                qDebug()<<"main loop ......\n";

                auto end = std::chrono::high_resolution_clock::now();
                std::chrono::duration<double, std::milli> elapsed = end - start;
                
                // The rest of the time is spent sleeping.
                int64_t timeLeft = 100 - elapsed.count();
                if(timeLeft > 0){
                    std::this_thread::sleep_for(std::chrono::milliseconds(timeLeft));
                }
            }
        }

        void VolumeViewer::openImageFile(){
            auto pathPtr = m_imagePath2Load.exchange(nullptr);
            if (nullptr == pathPtr) { return; }
            auto filePath = *pathPtr;
            delete pathPtr;
            auto c = Common::i();

            filePath = Util::convertPlatformPath(filePath).toStdString();

            showMessage("Loading image ...");

            #if SHOW_VERBOSE
            LychnisReaderImpl::setVerbose(true);
            #endif
            auto reader = LychnisReaderFactory::createReader(filePath);
            if (nullptr == reader || !reader->isOpen()) {
                showMessage("Unable to open image file");
                return;
            }

            cv::Size viewerSize(1600, 1200);
            resizeEvent(viewerSize, 1);

            bool bProjectValid = !m_project->m_projectPath.empty();

            auto rawLevel = reader->getLevel(0);

            c->p_bRemoteVolume = reader->getReaderType() == LychnisReaderImpl::REMOTE;
            
            // Slice thickness
            int sliceThickness = reader->getSliceThickness();

            if (!bProjectValid && sliceThickness >= 0 && sliceThickness != m_project->m_sliceThickness) {
                m_project->m_sliceThickness = sliceThickness;
                emit c->sliceThicknessLoaded(sliceThickness);
            }

            m_project->m_imageReader = std::move(reader);
            m_project->m_imagePath = filePath;
            
            // Voxel size
            double voxelSize[3];
            if (m_project->m_imageReader->getVoxelSize(voxelSize)) {
                if (!bProjectValid) {
                m_project->m_voxelSize = *(cv::Point3d *)voxelSize;
                emit c->voxelSizeLoaded(voxelSize[0], voxelSize[1], voxelSize[2]);
                }
            }

            // Origin point
            if (!bProjectValid) {
                auto path = QString::fromStdString(filePath);
                if (!path.startsWith("{")) { emit c->setProjectFileName(QFileInfo(path).fileName()); }
                int origin[3];
                if (m_project->m_imageReader->getOrigin(origin)) {
                m_project->m_origin = cv::Point3d(origin[0], origin[1], origin[2]);
                emit c->originChangedViewer();
                }
            }

            // Volume geometry
            auto onUpdateVolumeGeometry = [this, rawLevel, c](bool bProjectValid, bool bUpdateCenter) {
                auto pVoxelSize = (double *)&m_project->m_voxelSize;
                for (int i = 0; i < 3; i++) { m_project->m_dims[i] = double(rawLevel->dims[2 - i]) * pVoxelSize[i]; }

                QPoint rx(m_project->m_origin.x, m_project->m_origin.x + m_project->m_dims[0]),
                    ry(m_project->m_origin.y, m_project->m_origin.y + m_project->m_dims[1]),
                    rz(m_project->m_origin.z, m_project->m_origin.z + m_project->m_dims[2]);
                if (!bProjectValid) {
                m_project->m_center[0] = (rx.y()) / 2.;
                m_project->m_center[1] = (ry.x() + ry.y()) / 2.;
                m_project->m_center[2] = (rz.x() + rz.y()) / 2.;
                }

                double center[] = {rx.x() - 1., -1, -1};
                if (bUpdateCenter) { memcpy(center, m_project->m_center, sizeof(m_project->m_center)); }

                // emit c->centerLoaded(rx, ry, rz, center[0], center[1], center[2]);
                // double bounds[] = {(double)rx.x(), (double)rx.y(), (double)ry.x(), (double)ry.y(), (double)rz.x(), (double)rz.y()};
	            // setBounds(bounds);
                double bounds[6];
                for (int i = 0; i < 3; i++) {
                    auto v1 = ((double *)&m_project->m_origin)[i], v2 = v1 + m_project->m_dims[i];
                    bounds[2 * i] = v1;
                    bounds[2 * i + 1] = v2;
                }
                setBounds(bounds);
            };
            onUpdateVolumeGeometry(bProjectValid, true);

            // Resolution
            int resId = m_project->m_currentRes.load(), firstRes = -1;
            for (int index : m_project->m_imageReader->getLevelIndexes()) {
                auto info = m_project->m_imageReader->getLevel(index);
                firstRes = info->resId;
                bool bOK = true;
                for (int i = 0; i < 3; i++) {
                if (info->dims[i] > m_blockSize[2 - i]) {
                    bOK = false;
                    break;
                }
                }
                if (bOK) {
                m_project->m_initResId = info->resId;
                break;
                }
            }
            if (resId < 0) { resId = firstRes; }

            if (!m_project->m_bSplitSlices) {
                m_project->m_channelNumber = (int)rawLevel->dataIds[0].size();
                setChannels(m_project->m_imageReader->getChannelNames(), m_project->m_imageReader->getChannelColors());
            } else {
                QList<cv::Point3d> colors = {cv::Point3d(0.5, 1, 1), cv::Point3d(1, 1, 0.5)};
                if (m_project->m_sliceThickness == 0) { colors = {cv::Point3d(1, 1, 1), cv::Point3d(0, 0, 0)}; }
                m_project->m_channelNumber = 2;

                if (m_project->m_currentChannel >= 0 && m_project->m_currentChannel < m_project->m_channelInfos.length()) {
                auto v = m_project->m_channelInfos[m_project->m_currentChannel].toMap();
                auto contrast = v["contrast"].toString();
                m_project->m_channelInfos.clear();

                if (!contrast.isEmpty()) {
                    for (int i = 0; i < 2; i++) {
                    v["color"] = Util::point2String(colors[i]);
                    v["index"] = i;
                    m_project->m_channelInfos.append(v);
                    }
                }
                } else { m_project->m_channelInfos.clear(); }

                setChannels({"C1", "C2"}, colors);
            }

            // Todo: remove these magic numbers
            if (m_project->m_dims[2] == 1) {
                setCameraDirection({0, 0, -1}, {0, -1, 0}, true);
                resId = 0;
                m_project->m_initResId = resId;
                m_project->m_currentRes = resId;
                m_blockSize[0] = 4096;
                m_blockSize[1] = 4096;
                m_blockSize[2] = 16;
                emit c->blockSizeLoaded(4096, 16);
            } else if (std::max(m_project->m_dims[0], std::max(m_project->m_dims[1], m_project->m_dims[2])) <= 512) {
                resId = 0;
                m_project->m_initResId = resId;
                m_project->m_currentRes = resId;
                for (int &i : m_blockSize) { i = 512; }
                emit c->blockSizeLoaded(512, 512);
            }

            m_nextResolution = resId;
            updateResolution();
            updateBlock();

            showMessage("Image loaded");
        }

        void VolumeViewer::importNodes(){
            auto protoPtr = m_projectProto2Load.exchange(nullptr);
            if (nullptr != protoPtr) {
                auto dp = protoPtr->m_origin - m_project->m_origin;
                importNodesProto(protoPtr->m_loadedProjectInfo, protoPtr->m_voxelSize, dp, protoPtr->m_loadedProto);
            }

            auto pathPtr = m_nodesPath2Load.exchange(nullptr);
            if (nullptr == pathPtr) { return; }
            auto paths = pathPtr->paths;//QString::fromStdString(*pathPtr).split("\n");
            g_mergeRadius = pathPtr->bMerge ? 1E-6 : -1;
            delete pathPtr;

            auto c = Common::i();
            int index = 0;
            for (auto &path : paths) {
                auto progressStr = QString(" (%1/%2) ").arg(QString::number(++index), QString::number(paths.size()));
                emit c->showMessage(tr("Importing nodes") + progressStr + "...");

                QVariantMap params;
                bool bLoaded{false};
                void *ptr{nullptr};
                if (path.endsWith(".lypb")) {
                ptr = LychnisProjectReader::loadNodesProto(path.toStdString(), params);
                bLoaded = nullptr != ptr;
                } else if (path.endsWith(".lyp") || path.endsWith(".json")) {
                bLoaded = Util::loadJson(path, params);
                } else if (path.endsWith(".swc")) { if (!importNodesSWC(path)) { return; } else { continue; }}

                if (!bLoaded) {
                emit c->showMessage(tr("Unable to load file ") + path, 3000);
                return;
                }

                bool bOK;
                cv::Point3d voxelSize;
                if (!LychnisProjectReader::loadVoxelSize(params, voxelSize)) {
                emit c->showMessage("", 100);
                return;
                }

                cv::Point3d origin, dp;
                bOK = Util::string2Point(params["origin"].toString(), origin);
                if (bOK) { dp = origin - cv::Point3d(m_project->m_origin); }

                if (nullptr == ptr) { VolumeViewerCore::importNodes(params, voxelSize, (double *)&dp); }
                else { importNodesProto(params, voxelSize, dp, ptr); }
            }

            emit nodeImported();
            emit c->showMessage(tr("Nodes imported"), 500);
        }

        void VolumeViewer::operateNodes(){
            int code = m_nextNodeOperation.exchange(-1);
            if (code < 0) { return; }

            if (4 == code) { buildNodeGroups(); }
        }

        void VolumeViewer::updateBlockSize(){
            cv::Point3i *ptr = m_nextBlockSize.exchange(nullptr);
            if (nullptr == ptr) { return; }
            m_blockSize[0] = ptr->x;
            m_blockSize[1] = ptr->y;
            m_blockSize[2] = ptr->z;
            delete ptr;
            markModified();
        }

        void VolumeViewer::updateCenter(){
            auto ptr = m_nextCenter.exchange(nullptr);
            if (nullptr != ptr) {
                m_project->m_center[0] = ptr->x;
                m_project->m_center[1] = ptr->y;
                m_project->m_center[2] = ptr->z;
                delete ptr;
                markModified();
                return;
            }
        }

        void VolumeViewer::updateResolution(){
            int resId = m_nextResolution.exchange(-1);
            if (resId < 0) { return; }
            m_project->m_currentRes = resId;
            markModified();

            if (nullptr != m_project->m_imageReader) {
                auto p = m_project->m_imageReader->getLevel(resId);
                if (nullptr == p) {
                    showMessage("Unable to get level");
                    return;
                }
                emit Common::i()->spacingChanged(p->spacing[2], p->spacing[1], p->spacing[0]);
            } else { emit Common::i()->spacingChanged(1, 1, 1); }
        }

        void VolumeViewer::reloadVolume(){
            auto info = m_volumeReloadInfo.exchange(nullptr);
            if (nullptr == info) { return; }

            if (info->bChanging) {
                if (info->b3d) {
                memcpy(m_blockSize, m_blockSize3D, sizeof(m_blockSize));
                m_project->m_currentRes = m_currentRes3D.load();
                } else {
                memcpy(m_blockSize3D, m_blockSize, sizeof(m_blockSize));
                m_currentRes3D = m_project->m_currentRes.load();
                }
            }
            if (!info->b3d) {
                cv::Rect r1(info->origin.x, info->origin.y, ceil(info->size.x), ceil(info->size.y)),
                    r2(0, 0, m_project->m_dims[0], m_project->m_dims[1]);
                if (!Util::computeInterSection(r1, r2)) {
                    showMessage("Invalid ROI when reload volume");
                    delete info;
                    
                    return;
                }

                info->origin.x = r2.x;
                info->origin.y = r2.y;
                info->size.x = r2.width;
                info->size.y = r2.height;
                cv::Point3d center = info->origin + info->size * 0.5;
                m_project->m_center[0] = center.x;
                m_project->m_center[1] = center.y;
                emit Common::i()->centerChangedViewer(m_project->m_center[0], m_project->m_center[1], m_project->m_center[2]);

                LevelInfo *level = nullptr;
                for (int index : m_project->m_imageReader->getLevelIndexes()) {
                auto p = m_project->m_imageReader->getLevel(index);
                if (!p->b2dAvail) { continue; }
                if (nullptr == level || p->spacing[1] <= info->pixelSize) { level = p; } else { break; }
                }
                m_project->m_currentRes = level->resId;
                info->size /= level->spacing[1];
                m_blockSize[0] = abs(info->size.x);
                m_blockSize[1] = abs(info->size.y);
                m_blockSize[2] = 1;
            }
            delete info;
            markModified();
        }

        void VolumeViewer::updateChannel(){
            int channel = m_nextChannel.exchange(-1);
            bool bUpdated = false;
            if (channel >= 0) {
                m_project->m_currentChannel = channel - 1;
                bUpdated = true;
            }
            cv::Point2i *pRange = m_nextWorldRange.exchange(nullptr);
            if (nullptr != pRange) {
                if (nullptr != m_project->m_imageReader) {
                if (m_project->m_currentChannel >= 0 && pRange->y > pRange->x && pRange->x >= 0) {
                    m_project->m_imageReader->setChannelDisplayRange(-m_project->m_currentChannel, pRange->x, pRange->y);
                }
                bUpdated = true;
                }
                delete pRange;
            }
        }

        void VolumeViewer::updateBlock(){
            if (m_bUpdated) { m_bUpdated = false; } else { return; }

            auto p = m_project->m_imageReader->getLevel(m_project->m_currentRes);
            if (nullptr == p) { return; }

            showMessage("Resolution level " + std::to_string(p->resId + 1) + " loading");

            bool bOK = true;
            void *buffer = nullptr;
            QList<int> channels;
            size_t start[3], block[3];
            double spacing[] = {1, 1, 1};

            double minVoxelSize = std::min(m_project->m_voxelSize.x, std::min(m_project->m_voxelSize.y, m_project->m_voxelSize.z)),
                *p_voxelSize = (double *)&(m_project->m_voxelSize), *p_origin = (double *)&(m_project->m_origin);
            int blockSize[3];
            for (int i = 0; i < 3; i++) { blockSize[i] = m_blockSize[i] * minVoxelSize / p_voxelSize[i]; }

            for (int i = 0; i < 3; i++) {
                double v = (m_project->m_center[2 - i] - p_origin[2 - i]) / p_voxelSize[2 - i] / p->spacing[i];
                int w1 = (int)p->dims[i];
                size_t w2 = blockSize[2 - i];
                size_t v1 = std::min(w1 - 1, std::max(0, int(v - w2 / 2.0))), v2 = std::min(w2, w1 - v1);
                start[i] = v1;
                block[i] = v2;
                spacing[i] = p->spacing[i];
            }

            size_t num = block[0] * block[1] * block[2], len = num * 2;

            QList<ChannelBarInfo*> channelBar;
            getChannelInfos(channelBar);

            if (m_project->m_bSplitSlices) { channels.append(m_project->m_currentChannel); }
            else {
                for (int i = 0; i < m_project->m_channelNumber; i++) {
                    if(i<channelBar.length() && channelBar[i]->bVisible){
                        channels.append(i);
                    }
                }
            }

            if (channels.empty()) {
                showMessage("No visible channels");
                return;
            }
            int channelNumber = channels.length();

            bool bImage2D = (1 == block[0]);//2d image rendering
            size_t area = len * channelNumber;
            buffer = malloc(area * (bImage2D ? 2 : 1));
            if (nullptr == buffer) {
                showMessage("Unable to allocate memory in updateBlock");

                return;
            }

            uint16_t *buffer2 = (1 == m_project->m_channelNumber ? (uint16_t *)buffer : (uint16_t *)malloc(len));
            bOK &= loadBlock(p, start, block, spacing, channels, buffer, buffer2);
            if (buffer2 != buffer) { free(buffer2); }

            if (bImage2D) { memcpy((char *)buffer + area, buffer, area); }

            if (!bOK) {
                showMessage("Failed to load data");
                free(buffer);
                return;
            }

            double origin[3];
            for (int i = 0; i < 3; i++) {
                spacing[i] *= p_voxelSize[2 - i];
                origin[i] = double(start[i]) * spacing[i] + p_origin[2 - i];
            }

            sendBlock2Viewer(buffer, origin, block, spacing, channels);
            showMessage("Image loaded");
        }

        bool VolumeViewer::loadBlock(LevelInfo *p, size_t start[], size_t block[], double spacing[], const QList<int> &channels, void *buffer, uint16_t *buffer2) {
            static uint16_t *buffer3 = nullptr;
            static size_t buffer3Length = 0;
            
            bool bOK = true;
            size_t num = block[0] * block[1] * block[2], len = num * 2;
            int channelNumber = channels.length();

            for (int i = 0; i < channels.length(); i++) {
                int index = channels[i], offset[3] = {0, 0, 0};

                if (buffer2 != buffer && (offset[0] != 0 || offset[1] != 0 || offset[2] != 0)) {//channel shifting
                size_t ls[3] = {0}, start3[3], block3[3];
                bool bSameSize = true;
                for (int i = 0; i < 3; i++) {
                    int v1 = int(start[i]) - offset[i], v2 = v1 + int(block[i]), w2 = int(p->dims[i]);
                    if (v1 < 0) {
                    ls[i] = -v1;
                    start3[i] = 0;
                    } else { start3[i] = v1; }
                    block3[i] = std::min(v2, w2) - start3[i];
                    bSameSize &= (block3[i] == block[i]);
                }
                if (bSameSize) {
                    bOK &= m_project->m_imageReader->readBlock(p, start3, block, buffer2, m_project->m_currentTimePoint, index);
                } else {
                    if (len > buffer3Length) {
                    free(buffer3);
                    buffer3Length = len;
                    buffer3 = (uint16_t *)malloc(len);
                    }

                    bOK &= m_project->m_imageReader->readBlock(p, start3, block3, buffer3, m_project->m_currentTimePoint, index);

                    cv::Size size1((int)block3[2], (int)block3[1]), size((int)block[2], (int)block[1]);
                    memset(buffer2, 0, len);
                    uint16_t *pBuffer1 = buffer3, *pBuffer = buffer2 + ls[0];

                    for (int z = 0; z < block3[0]; z++, pBuffer1 += size1.area(), pBuffer += size.area()) {
                    cv::Mat image1(size1, CV_16UC1, pBuffer1), image(size, CV_16UC1, pBuffer);
                    image1.copyTo(image(cv::Rect((int)ls[2], (int)ls[1], image1.cols, image1.rows)));
                    }
                }

                } else { bOK &= m_project->m_imageReader->readBlock(p, start, block, buffer2, m_project->m_currentTimePoint, index); }
                if (!bOK || buffer2 == buffer) { break; }//break if only one channel

                //	memcpy((uint16_t*)buffer+i*num,buffer2,num*2);

                //  color composition
                uint16_t *pBuffer = (uint16_t *)buffer + i, *pBuffer2 = buffer2;
                for (size_t j = 0; j < num; j++, pBuffer2++, pBuffer += channelNumber) { *pBuffer = *pBuffer2; }

            }
            return bOK;
        }

        void VolumeViewer::saveProject(){
            QList<LychnisProjectReader::ProjectInfo *> infos;
            auto projectInfo = m_projectInfo2Save.exchange(nullptr);
            if (nullptr != projectInfo) {
                infos.append(projectInfo);
                projectInfo->bAutoSaved = false;
            }

            static auto start = std::chrono::system_clock::now();
            auto end = std::chrono::system_clock::now();
            std::chrono::duration<double> elapsed_seconds = end - start;
            if (elapsed_seconds.count() > 60) {
                // auto saving for every 60 seconds
                projectInfo = m_projectInfo2AutoSave.exchange(nullptr);
                if (nullptr != projectInfo) {
                infos.append(projectInfo);
                projectInfo->bAutoSaved = true;
                }
                start = end;
            }

            if (infos.empty()) { return; }

            QVariantList channelParams;
            QList<ChannelBarInfo*> channelBarInfo;
            getChannelInfos(channelBarInfo);
            for (auto p : channelBarInfo) { 
                channelParams.append(p->toMap());
            }
            
            auto c = Common::i();

            for (auto &info : infos) {
                showMessage("Saving...");

                if (info->mode > 0) {
                bool bOK = false;
                switch (info->mode) {
                    case 1: { bOK = m_project->exportNodes2ImsFile(info->filePath); }
                    break;
                    case 2: { bOK = m_project->exportNodes2Spots(info->filePath); }
                    break;
                    case 3: { bOK = m_project->exportNodesSWC(info->filePath); }
                    break;
                }
                showMessage(bOK ? "Done" : "Failed");
                continue;
                }

                auto params = info->allInfo;
                info->channelParams = channelParams;
                auto bOK = m_project->dump(params, info);

                if (bOK) {
                if (!info->bAutoSaved) {
                    showMessage("Saved");
                    emit c->setProjectFileName(QFileInfo(info->filePath).fileName());
                } else { showMessage("Saved"); }
                } else { emit c->showMessage(tr("Unable to save project to ") + info->filePath, 3000); }
            }
        }

        void VolumeViewer::setChannels(const QStringList &names, const QList<cv::Point3d> &colors){
            auto channelInfos = new ChannelBarInfo(-1), lastChannel = channelInfos;
            QList<ChannelBarInfo *> infos;
            QStringList channelNames;

            // m_channelBar->syncAllChannels(m_project->m_bAnnotation);

            bool bOK1 = m_project->m_channelInfos.length() == m_project->m_channelNumber,
                bOK2 = names.length() == m_project->m_channelNumber, bOK3 = colors.length() == m_project->m_channelNumber;
            for (int i = 0; i < m_project->m_channelNumber; i++) {
                auto p = bOK1 ? new ChannelBarInfo(m_project->m_channelInfos[i].toMap()) : new ChannelBarInfo(i);
                lastChannel->next = p;
                lastChannel = p;
                infos.append(p);
                if (bOK2) {
                p->name = names[i];
                channelNames.append(p->name);
                } else {
                QString s = QString::number(p->index + 1);
                p->name = "C" + s;
                channelNames.append("Channel " + s);
                }
                if (bOK3 && !bOK1) {
                cv::Point3d p1 = colors[i];
                memcpy(p->color, &p1, sizeof(p1));
                }
            }
            setChannelInfos(infos);
            // m_channelBar->setChannels(channelInfos);

            emit Common::i()->channelLoaded(channelNames.join("\n"), m_project->m_bSplitSlices ? -1 : m_project->m_currentChannel);
        }

        void VolumeViewer::sendBlock2Viewer(void *buffer, double origin[], size_t block[], double spacing[], QList<int> &channels){
            size_t num = block[0] * block[1] * block[2], len = num * 2;
            bool bImage2D = (1 == block[0]);

            if (m_project->m_bSplitSlices) {
                auto buffer3 = (uint16_t *)malloc(len * 2);
                memset(buffer3, 0, len * 2);
            
                channels = QList<int>() << 0 << 1;

                if (m_project->m_sliceThickness <= 0) {
                size_t num = block[0] * block[1] * block[2];
                uint16_t *pSrc = (uint16_t *)buffer, *pDst = buffer3;
                for (int i = 0; i < num; i++, pSrc++, pDst += 2) {
                    uint16_t v = *pSrc;
                    *pDst = v;
                    *(pDst + 1) = 0;
                }
                } else {
                size_t imageArea = block[1] * block[2];
                for (int i = 0; i < block[0]; i++) {
                    int slice = (i * spacing[0] + origin[0]) / m_project->m_sliceThickness;
                    bool bC1 = (slice % 2 == 0);
                    uint16_t *pSrc = (uint16_t *)buffer + i * imageArea, *pDst = buffer3 + 2 * (i * imageArea);
                    for (int i = 0; i < imageArea; i++, pSrc++, pDst += 2) {
                    *(pDst + int(!bC1)) = *pSrc;
                    }
                }
                }
                free(buffer);
                buffer = buffer3;
            }

            double spacing1[] = {spacing[2], spacing[1], spacing[0]};
            double origin1[3] = {origin[2], origin[1], origin[0]};//x,y,z
            size_t block1[] = {block[2], block[1], block[0]};//x,y,z
            if (bImage2D) { block1[2] = 2; }

            auto vInfo = new VolumeInfo;
            vInfo->buffer = buffer;
            vInfo->channels = channels;
            memcpy(vInfo->dims, block1, sizeof(vInfo->dims));
            memcpy(vInfo->spacing, spacing1, sizeof(vInfo->spacing));
            memcpy(vInfo->origin, origin1, sizeof(vInfo->origin));

            auto info = vInfo;
            setVolume(info->buffer, info->dims, info->spacing, info->origin, info->channels, true, true);
            info->buffer = nullptr;
            delete info;
        }


        void VolumeViewer::onLoadProject(std::string projectPath){
            // Only one project file is opened at a time.
            if (isLoaded()){
                return ;
            }
            
            // If the projectPath is empty, app will return.
            if (projectPath.empty()){
                return ;
            }

            // Load the project file.
            if(!m_project->load(projectPath)){
                showMessage("Failed to load project file.");
                return ;
            }

            bProject_ImageLoaded = true;
            auto& params = m_project->m_loadedProjectInfo;

            if(m_project->m_bAnnotation && m_project->m_sliceThickness > 0){
                m_project->m_bSplitSlices = true;
                if(m_project->m_currentChannel < 0){
                    m_project->m_currentChannel = 0;
                }
                m_nextChannel = m_project->m_currentChannel + 1;
            }
            
            delete m_imagePath2Load.exchange(new std::string(m_project->m_imagePath));
            if(m_project->isBinary()){
                m_projectProto2Load.exchange(m_project);
            }else if(params.contains("nodes")){
                delete m_nodesPath2Load.exchange(new ImportNodesInfo(QString::fromStdString(projectPath)));
            }
        }

        void VolumeViewer::onSaveProject(bool bSaveAs, std::string& projectPath){
            if (m_project->m_imagePath.empty()) { return; }

            // Save as or projectPath is empty.
            if (bSaveAs || m_project->m_projectPath.empty()) {
                if (!projectPath.empty()) {
                     m_project->m_projectPath = projectPath;
                }else { return; }
            }

            auto info = new LychnisProjectReader::ProjectInfo;
            info->filePath = QString::fromStdString(m_project->m_projectPath);

            info->displayInfo = exportDisplayParams();
            delete m_projectInfo2Save.exchange(info);
        }

        bool VolumeViewer::isLoaded(){
            return bProject_ImageLoaded;
        }

        QVariantMap VolumeViewer::exportDisplayParams(){
            DisplayParameters params;
            getDisplayParameters(&params);
            params.windowSize.bAuto = m_fixedViewerSize.empty();
            auto size = params.windowSize.bAuto ? m_autoViewerSize : m_fixedViewerSize;
            params.windowSize.size[0] = size.width;
            params.windowSize.size[1] = size.height;
            auto data = params.toMap();
            AnimationFrame frame;
            requestCameraParams(&frame);
            data["camera"] = frame.toMap();
            return data;
        }

        VolumeViewer& VolumeViewer::getInstance(CefRefPtr<CefFrame> frame){
            static LychnisReader reader;
            static VolumeViewer instance(reader, frame);

            return instance;
        }

        void VolumeViewer::showMessage(const std::string& message){
            CefRefPtr<CefProcessMessage> msg = CefProcessMessage::Create("Message");
            msg->GetArgumentList()->SetString(0, message);
            m_frame->SendProcessMessage(PID_RENDERER, msg);
        }

        void VolumeViewer::markModified(){
            m_bUpdated = true;
        }

        void VolumeViewer::paintEvent(){
            // Render the image to a cv::Mat object.
            cv::Mat image = renderToImage();
            if (image.empty()) { return; }

            // Convert the cv::Mat object to a binary buffer.
            std::vector<uchar> tempBuf;
            cv::imencode(".jpg", image, tempBuf);

            CefRefPtr<CefBinaryValue> imageBinary = CefBinaryValue::Create(tempBuf.data(), tempBuf.size()*sizeof(uchar));
                
            // IPC: Send the binary buffer to the renderer process.
            CefRefPtr<CefProcessMessage> msg = CefProcessMessage::Create("Image");
            msg->GetArgumentList()->SetBinary(0, imageBinary);
            m_frame->SendProcessMessage(PID_RENDERER, msg);
            qDebug() << "Paint Event ......";
        }

        void VolumeViewer::updateScreen(){
            paintEvent();
        }

    }// namespace binding
}// namespace app