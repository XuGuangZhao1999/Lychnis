#include "main/binding/VolumeViewer.hpp"
#include "shared/util/BindingUtil.hpp"
#include "include/cef_parser.h"

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
            //  int minGray, maxGray;
            VolumeReloadInfo() : b3d(true), bChanging(false) {}
        };
        struct ImportNodesInfo {
            QStringList paths;
            bool bMerge{true};
            explicit ImportNodesInfo(const QString &p) { paths.append(p); }
            ImportNodesInfo() = default;
        };

        bool VolumeViewer::bProject_ImageLoaded = false;
        std::string VolumeViewer::m_imagePath2Load = "";
        std::string VolumeViewer::m_projectPath2Load = "";

        VolumeViewer::VolumeViewer(LychnisReader& reader) : VolumeViewerCore(&reader) {
            m_project = &reader;
            for(auto &it: m_blockSize){
                it = 256;
            }
        }

        VolumeViewer::~VolumeViewer(){
            bProject_ImageLoaded = false;
            (void)m_frame.release();
        }

        bool VolumeViewer::initViewer(){
            cv::Size viewerSize(1600, 1200);
            resizeEvent(viewerSize, 1);
            setShowScaleBar(false);

            bProject_ImageLoaded = openImageFile();

            return bProject_ImageLoaded;
        }

        bool VolumeViewer::openImageFile(){
            auto filePath = m_imagePath2Load;
            auto c = Common::i();

            filePath = Util::convertPlatformPath(filePath).toStdString();

            #if SHOW_VERBOSE
            LychnisReaderImpl::setVerbose(true);
            #endif
            auto reader = LychnisReaderFactory::createReader(filePath);
            if (nullptr == reader || !reader->isOpen()) {
                QString errMsg = (nullptr == reader ? "" : reader->getErrorMessage());
                if (!errMsg.isEmpty()) { errMsg = ", " + errMsg; }
                return false;
            }

            bool bProjectValid = !m_project->m_projectPath.empty();
            importNodes();

            auto rawLevel = reader->getLevel(0);

            c->p_bRemoteVolume = reader->getReaderType() == LychnisReaderImpl::REMOTE;

            int sliceThickness = reader->getSliceThickness();

            if (!bProjectValid && sliceThickness >= 0 && sliceThickness != m_project->m_sliceThickness) {
                m_project->m_sliceThickness = sliceThickness;
            }

            m_project->m_imageReader = std::move(reader);
            m_project->m_imagePath = filePath;

            double voxelSize[3];
            if (m_project->m_imageReader->getVoxelSize(voxelSize)) {
                if (!bProjectValid) {
                m_project->m_voxelSize = *(cv::Point3d *)voxelSize;
                fristChangeCurrentNode(m_projectPtr->m_nodesManager.m_currentNode);
                }
            }

            if (!bProjectValid) {
                auto path = QString::fromStdString(filePath);
                if (!path.startsWith("{")) { emit c->setProjectFileName(QFileInfo(path).fileName()); }
                int origin[3];
                if (m_project->m_imageReader->getOrigin(origin)) {
                m_project->m_origin = cv::Point3d(origin[0], origin[1], origin[2]);
                }
            }

            auto onUpdateVolumeGeometry = [this, rawLevel](bool bProjectValid, bool bUpdateCenter) {
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
                if(bUpdateCenter){memcpy(center, m_project->m_center, sizeof(m_project->m_center));}
                m_center.x = m_project->m_center[0];
                m_center.y = m_project->m_center[1];
                m_center.z = m_project->m_center[2];

                double bounds[] = {(double)rx.x(), (double)rx.y(), (double)ry.x(), (double)ry.y(), (double)rz.x(), (double)rz.y()};
	            setBounds(bounds);
            };
            onUpdateVolumeGeometry(bProjectValid, true);

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
                m_project->m_channelNumber = rawLevel->dataIds[0].size();
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

            m_totalResolutions = m_project->m_imageReader->getLevelIndexes().length();

            // Todo: remove these magic numbers
            if (m_project->m_dims[2] == 1) {
                setCameraDirection({0, 0, -1}, {0, -1, 0}, true);
                resId = 0;
                m_project->m_initResId = resId;
                m_project->m_currentRes = resId;
                m_blockSize[0] = 4096;
                m_blockSize[1] = 4096;
                m_blockSize[2] = 16;
            } else if (std::max(m_project->m_dims[0], std::max(m_project->m_dims[1], m_project->m_dims[2])) <= 512) {
                resId = 0;
                m_project->m_initResId = resId;
                m_project->m_currentRes = resId;
                for (int &i : m_blockSize) { i = 512; }
            }

            m_currentResolution = resId;
            updateResolution(resId);
            updateBlock();

            sendVisualInfo();
            visableChannelCount = m_channelInfos.size();

            return true;
        }

        void VolumeViewer::updateBlock(){
            auto p = m_project->m_imageReader->getLevel(m_project->m_currentRes);
            if (nullptr == p) { return; }

            auto c = Common::i();
            emit c->showMessage(tr("(Resolution level ") + QString::number(p->resId + 1) + tr(") loading ..."));

            bool bOK = true;
            void *buffer = nullptr;
            QList<int> channels;
            size_t start[3], block[3];
            double spacing[] = {1, 1, 1};

            double minVoxelSize = std::min(m_project->m_voxelSize.x, std::min(m_project->m_voxelSize.y, m_project->m_voxelSize.z)),
                *p_voxelSize = (double *)&m_project->m_voxelSize, *p_origin = (double *)&m_project->m_origin;
            int blockSize[3];
            for (int i = 0; i < 3; i++) { blockSize[i] = m_blockSize[i] * minVoxelSize / p_voxelSize[i]; }

            for (int i = 0; i < 3; i++) {
                double v = (m_project->m_center[2 - i] - p_origin[2 - i]) / p_voxelSize[2 - i] / p->spacing[i];
                int w1 = p->dims[i];
                size_t w2 = blockSize[2 - i];
                size_t v1 = std::min(w1 - 1, std::max(0, int(v - w2 / 2.0))), v2 = std::min(w2, w1 - v1);
                start[i] = v1;
                block[i] = v2;
                spacing[i] = p->spacing[i];
            }

            size_t num = block[0] * block[1] * block[2], len = num * 2;

            if (m_project->m_bSplitSlices) { channels.append(m_project->m_currentChannel); }
            else {
                m_channelInfos.clear();
                getChannelInfos(m_channelInfos);
                for (int i = 0; i < m_project->m_channelNumber; i++) {
                if (m_channelInfos[i]->bVisible) { channels.append(i); }
                }
            }

            if (channels.empty()) {
                emit c->showMessage(tr("No visible channels"), 2000);
                return;
            }
            int channelNumber = channels.length();

            bool bImage2D = (1 == block[0]);//2d image rendering
            size_t area = len * channelNumber;
            buffer = malloc(area * (bImage2D ? 2 : 1));
            if (nullptr == buffer) {
                qWarning() << "Unable to allocate memory in updateBlock";
                return;
            }

            uint16_t *buffer2 = (1 == m_project->m_channelNumber ? (uint16_t *)buffer : (uint16_t *)malloc(len));
            bOK &= loadBlock(p, start, block, spacing, channels, buffer, buffer2);
            if (buffer2 != buffer) { free(buffer2); }

            if (bImage2D) { memcpy((char *)buffer + area, buffer, area); }

            if (!bOK) {
                emit c->showMessage(tr("Failed to load data"), 3000);
                free(buffer);
                return;
            }

            // enhance data here ...
            //  cv::Mat src(cv::Size(block[0], block[1] * block[2]), CV_16UC1, buffer), dst;
            //  Util::enhanceImage(src, dst, true, false);
            //  if (dst.size() != src.size() || dst.type() != CV_16UC1) { return; }
            //  memcpy(buffer, dst.data, src.total() * 2);

            double origin[3];
            for (int i = 0; i < 3; i++) {
                spacing[i] *= p_voxelSize[2 - i];
                origin[i] = double(start[i]) * spacing[i] + p_origin[2 - i];
            }

            sendBlock2Viewer(buffer, origin, block, spacing, channels);
            emit c->showMessage(tr("Image loaded"), 500);
        }

        void VolumeViewer::setChannels(const QStringList &names, const QList<cv::Point3d> &colors){
            auto channelInfos = new ChannelBarInfo(-1), lastChannel = channelInfos;
            QList<ChannelBarInfo *> infos;
            QStringList channelNames;

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
            getChannelInfos(m_channelInfos);
        }

        bool VolumeViewer::loadBlock(LevelInfo *p, size_t start[], size_t block[], double spacing[], const QList<int> &channels, void *buffer, uint16_t *buffer2){
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
                    int v1 = int(start[i]) - offset[i], v2 = v1 + block[i], w2 = p->dims[i];
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

                    cv::Size size1(block3[2], block3[1]), size(block[2], block[1]);
                    memset(buffer2, 0, len);
                    uint16_t *pBuffer1 = buffer3, *pBuffer = buffer2 + ls[0];

                    for (int z = 0; z < block3[0]; z++, pBuffer1 += size1.area(), pBuffer += size.area()) {
                    cv::Mat image1(size1, CV_16UC1, pBuffer1), image(size, CV_16UC1, pBuffer);
                    image1.copyTo(image(cv::Rect(ls[2], ls[1], image1.cols, image1.rows)));
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

        void VolumeViewer::sendBlock2Viewer(void *buffer, double origin[], size_t block[], double spacing[], QList<int> &channels){
            size_t num = block[0] * block[1] * block[2], len = num * 2;
            bool bImage2D = (1 == block[0]);

            if (m_project->m_bSplitSlices) {
                auto buffer3 = (uint16_t *)malloc(len * 2);
                memset(buffer3, 0, len * 2);
            //	int sliceThick = m_project.m_sliceThickness;
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
            setVolume(info->buffer, info->dims, info->spacing, info->origin, info->channels, true, false);
            info->buffer = nullptr;
            delete info;
        }

        void VolumeViewer::updateScreen(){
            shared::util::BindingUtil::paintEvent(m_frame, *this);
        }

        void VolumeViewer::fristChangeCurrentNode(LychnisNode *p){
            if (nullptr != m_projectPtr->m_nodesManager.m_currentNode) { removeActor(m_currentNodeActor); }

            bool bSwitchNode = (p != nullptr && m_projectPtr->m_nodesManager.m_currentNode != nullptr
                && p != m_projectPtr->m_nodesManager.m_currentNode);
            m_projectPtr->m_nodesManager.m_currentNode = p;
            auto c = Common::i();

            cv::Point3d sectionfocalPoint(-1, -1, -1);
            if (nullptr != p) {
                auto group =
                    m_projectPtr->m_nodesManager.m_nodeGroups.value(p->groupId, m_projectPtr->m_nodesManager.m_defaultGroup);
                for (int i = 0; i < 3; i++) { *((double *)&s_colorLine + i) = group->color[i]; }

                m_prevSelectedGroupId = group->groupId;
                m_currentNodeActor = getNodeActor(p, (double *)&s_colorLine);

                sectionfocalPoint = cv::Point3d(p->pos[0], p->pos[1], p->pos[2]);

                QString posStr = p->toString(1);
                if (m_projectPtr->m_sliceThickness > 0) { posStr += ", " + tr("slice") + " " + QString::number(nodeSliceIndex(p)); }

            //	auto creatorMsg = p->creatorName;

                emit
                c->selectedNodeChanged(p->id,
                                    p->msg,
                                    p->creatorName,
                                    posStr,
                                    group->color[0],
                                    group->color[1],
                                    group->color[2],
                                    true,
                                    p->bReadOnly);

                if (bSwitchNode && m_bAutoLoading) {
                bool bNeedReload = true;
                if (bNeedReload) { emit centerUpdated(p->pos[0], p->pos[1], p->pos[2]); }
                } else if (c->p_bRemoteVolume && m_projectPtr->m_bAnnotation && !m_movingInfo.bMoving) {
                emit roiCenterUpdated(p->pos[0], p->pos[1], p->pos[2], m_roiMode);
                } else if (m_bAutoStitching) { moveSliceByNode(true); }

            //	m_reviewDialog->selectNode(p->id);
                postSelectNode(p->id);
            } else {
                emit
                c->selectedNodeChanged(0, "", "", "", -1, -1, -1, false, true);
            }
            //  Todo
            //  for (auto &m_sectionViewer : m_sectionViewers) {
            //	auto p = m_sectionViewer;
            //	p->setFocalPoint(sectionfocalPoint);
            //	if (p->isVisible()) { m_sectionViewer->showFocalPoint(); }
            //  }

            // updateScreen();
        }

        void VolumeViewer::sendVisualInfo(){
            auto resInfo = CefDictionaryValue::Create();
            resInfo->SetInt("totalResolutions", m_totalResolutions);
            resInfo->SetInt("currentResolution", m_currentResolution);

            auto centerInfo = CefDictionaryValue::Create();
            centerInfo->SetInt("x", m_center.x);
            centerInfo->SetInt("y", m_center.y);
            centerInfo->SetInt("z", m_center.z);

            auto blockSizeInfo = CefDictionaryValue::Create();
            blockSizeInfo->SetInt("width", m_blockSize[0]);
            blockSizeInfo->SetInt("height", m_blockSize[2]);

            auto channelInfos = CefListValue::Create();
            for(auto it : m_channelInfos){
                QVariantMap channelInfo = it->toMap();
                auto temp = CefDictionaryValue::Create();
                temp->SetString("position", channelInfo["position"].toString().toStdString());
                temp->SetString("color", channelInfo["color"].toString().toStdString());
                temp->SetString("contrast", channelInfo["contrast"].toString().toStdString());
                temp->SetInt("index", channelInfo["index"].toInt());
                temp->SetDouble("gamma", channelInfo["gamma"].toDouble());
                temp->SetBool("visible", channelInfo["visible"].toBool());
                channelInfos->SetDictionary(temp->GetInt("index"), temp);                
            }

            auto infos = CefDictionaryValue::Create();
            infos->SetDictionary("Resolution", resInfo);
            infos->SetDictionary("Center", centerInfo);
            infos->SetDictionary("BlockSize", blockSizeInfo);
            infos->SetList("Channels", channelInfos);
            
            auto temp = CefValue::Create();
            temp->SetDictionary(infos);
            auto msgData = CefWriteJSON(temp, JSON_WRITER_DEFAULT);

            shared::util::BindingUtil::setInfos(m_frame, msgData);
        }

        void VolumeViewer::keyPressKernel(int key){
            Common *c = Common::i();
            if (nullptr == m_volumeInfo) { return; }

            //  MutexTryLocker locker(&m_nodesMutex);
            //  if (!locker.tryLock()) { return; }

            switch (key) {
                case Qt::Key_Z: { g_bVolumePicking = true; }
                break;
                case Qt::Key_X: { g_bPropPicking = true; }
                break;
                case Qt::Key_V: { toggleShowNodes(true); }
                break;
                case Qt::Key_F: { findNextNode(); }
                break;
                case Qt::Key_B: { moveSliceByNode(); }
                break;
            //    case KeyEvent::Key::P:{generateSurfaceNodes();}break;
            //    case KeyEvent::Key::G:{m_viewer->grabFramebuffer().save("d:/lufeng/tmp/res.png");}break;

                case Qt::Key_Home:
                case Qt::Key_End:
                case Qt::Key_Up:
                case Qt::Key_Down: {
                traverseNode(key, false);
                break;
                }

                case Qt::Key_Delete: { deleteCurrentNode(); }
                break;
                case Qt::Key_Escape: { if (nullptr != m_projectPtr->m_nodesManager.m_currentNode) { changeCurrentNode(nullptr); }}
                break;
                case Qt::Key_Space: {
                if (nullptr != m_projectPtr->m_nodesManager.m_currentNode) {
                    auto p = m_projectPtr->m_nodesManager.m_currentNode;
                    emit centerUpdated(p->pos[0], p->pos[1], p->pos[2]);
                    updateCenter(p->pos[0], p->pos[1], p->pos[2]);
                } else {
                    emit
                    c->volumeReload();
                }
                }
                break;
            }
        }

        bool VolumeViewer::onLoadProject(){
            if(!m_project->load(m_projectPath2Load)){
                return false;
            }

            auto &params = m_project->m_loadedProjectInfo;

            if (m_project->m_bAnnotation && m_project->m_sliceThickness > 0) {
                m_project->m_bSplitSlices = true;
                if (m_project->m_currentChannel < 0) { m_project->m_currentChannel = 0; }
            }

            m_imagePath2Load = m_project->m_imagePath;
            if (m_project->isBinary()) { m_projectProto2Load = m_project; }
            else if (params.contains("nodes")) { m_nodesPath2Load = new ImportNodesInfo(QString::fromStdString(m_projectPath2Load)); }

            return initViewer();
        }

        bool VolumeViewer::onSaveProject(std::string& projectPath, bool bSaveAs){
            return true;
        }

        void VolumeViewer::importNodes(){
            auto protoPtr = m_projectProto2Load;
            if (nullptr != protoPtr) {
                auto dp = protoPtr->m_origin - m_project->m_origin;
                importNodesProto(protoPtr->m_loadedProjectInfo, protoPtr->m_voxelSize, dp, protoPtr->m_loadedProto);
            }

            auto pathPtr = m_nodesPath2Load;
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

        void VolumeViewer::updateResolution(int resId){
            if(resId < 0){return;}
            m_currentResolution = resId;
            m_project->m_currentRes = m_currentResolution;

            if (nullptr != m_project->m_imageReader) {
                auto p = m_project->m_imageReader->getLevel(resId);
                if (nullptr == p) {
                    return;
                }
            }

            bResUpdated = true;
            updateBlock();
        }

        void VolumeViewer::updateChannel(int channelId, int lower, int upper){
            if (channelId >= 0) {
                m_currentChannel = channelId;
                m_project->m_currentChannel = m_currentChannel - 1;
            }
            cv::Point2i *pRange = new cv::Point2i(lower, upper);
            if (nullptr != pRange) {
                if (nullptr != m_project->m_imageReader) {
                if (m_project->m_currentChannel >= 0 && pRange->y > pRange->x && pRange->x >= 0) {
                    m_project->m_imageReader->setChannelDisplayRange(-m_project->m_currentChannel, pRange->x, pRange->y);
                    m_worldRange = *pRange;
                }
                }
                delete pRange;
            }

            updateBlock();
        }

        void VolumeViewer::updateBlockSize(int x, int y, int z){
            m_blockSize[0] = x;
            m_blockSize[1] = y;
            m_blockSize[2] = z;

            updateBlock();
        }

        void VolumeViewer::updateCenter(int x, int y, int z){
            m_project->m_center[0] = x;
            m_project->m_center[1] = y;
            m_project->m_center[2] = z;

            m_center.x = x;
            m_center.y = y;
            m_center.z = z;

            updateBlock();
        }

        void VolumeViewer::updateChannelColor(int index, double r, double g, double b, double gamma){
            setColor(index, r, g, b, gamma);
            onRender(false);
        }

        void VolumeViewer::updateChannelVisibility(int index, bool bVisible){
            showHideChannel(index, bVisible);
            onRender(false);
            auto channelSize = m_channelInfos.size();
            if(bVisible){
                if(bResUpdated && visableChannelCount<channelSize){
                    updateBlock();
                }else if(visableChannelCount == channelSize){
                    bResUpdated = false;
                }
                ++visableChannelCount;
            }else{
                --visableChannelCount;
            }
        }

        void VolumeViewer::updateChannelContrast(int lower, int upper){
            for(int i=0; i<m_channelInfos.size(); ++i){
                onContrastChanged(i, lower, upper, m_project->m_bAnnotation);
            }
            onRender(false);
        }

        bool VolumeViewer::pickVolume(int x, int y){
             double pos[3];
            if (pickPos(x, y, pos)) {
                // VolumeViewerCore::pickVolume(pos[0], pos[1], pos[2]);
                updateCenter(pos[0], pos[1], pos[2]);
                return true;
            } else { return false; }
        }

        bool VolumeViewer::onOpenImage(){
            return initViewer();
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