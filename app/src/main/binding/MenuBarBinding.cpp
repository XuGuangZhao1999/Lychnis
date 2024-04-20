#include "main/binding/MenuBarBinding.hpp"
#include "main/net/RequestClient.hpp"
#include "shared/AppConfig.hpp"
#include "shared/util/Base64Encode.hpp"
#include "main/binding/VolumeViewer.hpp"

#include <algorithm>
#include <vector>
#include <Windows.h>
#include <commdlg.h>
#include <tchar.h>

#include <QDebug>
// #include <lychnisreader.h>
// #include <volumeviewercore.h>
#include <channelbarinfo.h>
#include <animationframeio.h>

#include <opencv2/shape/shape_transformer.hpp>

#include "include/cef_parser.h"
#include "include/cef_browser.h"

namespace app
{
namespace binding
{

class FileDialogCallback : public CefRunFileDialogCallback{
    private:
        CefRefPtr<CefFrame> m_frame;
        CefRefPtr<CefMessageRouterBrowserSide::Callback> m_callback;
    public:
        FileDialogCallback(CefRefPtr<CefFrame> frame, CefRefPtr<CefMessageRouterBrowserSide::Callback> callback):m_frame{frame}, m_callback{callback}{};

        void OnFileDialogDismissed (const std::vector< CefString > &file_paths){
            if(1 == file_paths.size()){
                VolumeViewer& vvInstance = VolumeViewer::getInstance(file_paths[0]);
                
                // CefRefPtr<CefProcessMessage> msg = CefProcessMessage::Create("Message");
                // msg->GetArgumentList()->SetString(0, "Message Testing");
                // m_frame->SendProcessMessage(PID_RENDERER, msg);
                vvInstance.onLoadProject();

                // LychnisReader& reader = vvInstance.getReader(file_paths[0]);
                // // LychnisReader reader(filePath);
                // if(reader.openImage() == false) {
                //     m_callback->Failure(404, "(未找到)应用找不到请求的工程文件");
                //     return;
                // }
                // VolumeViewerCore& viewer = vvInstance.getViewer();
                // // Test cpp call js.
                // // frame->ExecuteJavaScript("alert('加载成功')", frame->GetURL(), 0);
                // // VolumeViewerCore viewer(&reader);

                // // Set size
                // cv::Size viewerSize(1600, 1200);
                // viewer.resizeEvent(viewerSize, 1);

                // // Import nodes
                // double offset[] = {0, 0, 0};
                // QVariantMap projectInfo = reader.m_loadedProjectInfo;
                // viewer.importNodes(projectInfo, reader.m_voxelSize, offset);
                
                // // Get level
                // int levelSize = (reader.m_imageReader->getLevelIndexes()).size();
                // auto level = reader.m_imageReader->getLevel(levelSize / 2);

                // // Set bounds
                // double bounds[6];
                // for (int i = 0; i < 3; i++) {
                //     auto v1 = ((double *)&reader.m_origin)[i], v2 = v1 + reader.m_dims[i];
                //     bounds[2 * i] = v1;
                //     bounds[2 * i + 1] = v2;
                // }
                // viewer.setBounds(bounds);

                // // Set channel informations
                // QList<ChannelBarInfo *> channelInfos;
                // auto channelList = projectInfo["channels"].toList();
                // const auto numChannels = level->dataIds[0].size();
                // if (channelList.size() == numChannels) {
                //     for (auto &channel : channelList) { channelInfos.append(new ChannelBarInfo(channel.toMap())); }
                // } else { for (int i = 0; i < numChannels; i++) { channelInfos.append(nullptr); }}
                // viewer.setChannelInfos(channelInfos);

                // // Set data buffer
                // AutoBuffer buffer, singleChanBuffer, dstBuffer;
                // size_t numVoxels = level->dims[2] * level->dims[1] * level->dims[0];
                // auto singleChanBuf = (uint16_t *)singleChanBuffer.buffer(numVoxels * 2);
                // auto buf = (uint16_t *)buffer.buffer(singleChanBuffer.length() * numChannels);

                // // Read data from image to buffer
                // size_t start[] = {0, 0, 0};
                // QList<int> channels;
                // for (int i = 0; i < numChannels; i++) {
                //     reader.m_imageReader->readBlock(level, start, level->dims, singleChanBuf, 0, i);
                //     uint16_t *pBuffer = buf + i, *pBuffer2 = singleChanBuf;
                //     for (size_t j = 0; j < numVoxels; j++, pBuffer2++, pBuffer += numChannels) { *pBuffer = *pBuffer2; }
                //     channels.append(i);
                // }
                
                // // Set volume
                // size_t dims[3]; // x,y,z
                // double spacing[3], origin[3];
                // for (int i = 0; i < 3; i++) {
                //     dims[i] = level->dims[2 - i]; // zyx to xyz
                //     spacing[i] = level->spacing[2 - i] * ((double *)&reader.m_voxelSize)[i];
                //     origin[i] = (double)start[2 - i] * spacing[i] + ((double *)&reader.m_origin)[i];
                // }
                // viewer.setVolume(buf, dims, spacing, origin, channels, true, false);
                // viewer.setShowScaleBar(false);
                
                cv::Mat image = vvInstance.renderToImage(); // rgba
                
                // // Change to base64
                std::vector<uchar> tempBuf;
                cv::imencode(".jpg", image, tempBuf);
                // // std::string base64_image = CefBase64Encode(tempBuf.data(), tempBuf.size()*sizeof(uchar));
                // // std::string base64_image = shared::util::base64_encode(tempBuf.data(), tempBuf.size());
                
                // // Blob
                CefRefPtr<CefBinaryValue> imageBinary = CefBinaryValue::Create(tempBuf.data(), tempBuf.size()*sizeof(uchar));
                
                // // IPC
                CefRefPtr<CefProcessMessage> msg = CefProcessMessage::Create("Image");
                msg->GetArgumentList()->SetBinary(0, imageBinary);
                m_frame->SendProcessMessage(PID_RENDERER, msg);

                m_callback->Success("工程加载成功");
            }
        }

        FileDialogCallback(const FileDialogCallback&) = delete;
        FileDialogCallback& operator=(const FileDialogCallback&) = delete;

        IMPLEMENT_REFCOUNTING(FileDialogCallback);
};

MenuBarBinding::MenuBarBinding(){
    // tasks["loadProject"] = &MenuBarBinding::onTaskLoadProject;
    tasks.RegisterFunction("loadProject", &MenuBarBinding::onTaskLoadProject);
}

bool MenuBarBinding::OnQuery(CefRefPtr<CefBrowser> browser, CefRefPtr<CefFrame> frame, int64_t queryId, const CefString &request, bool persistent, CefRefPtr<Callback> callback)
{
    // only handle messages from application url
    const std::string &url = frame->GetURL();

    if (!(url.rfind(APP_ORIGIN, 0) == 0))
    {
        return false;
    }

    // auto func = tasks[request];
    auto func = tasks.GetFunction("loadProject");

    if(nullptr != func){
        return (this->*func)(browser, frame, queryId, request, persistent, callback);
    }

    return false;
}

bool MenuBarBinding::onTaskNetworkRequest(CefRefPtr<CefBrowser> browser, CefRefPtr<CefFrame> frame, int64_t queryId, const CefString &request, bool persistent, CefRefPtr<Callback> callback, const std::string &messageName, const std::string &requestMessage)
{
    // create a CefRequest object
    CefRefPtr<CefRequest> httpRequest = CefRequest::Create();

    // set the request URL
    httpRequest->SetURL(requestMessage.substr(messageName.size() + 1));

    // set the request method (supported methods include GET, POST, HEAD, DELETE and PUT)
    httpRequest->SetMethod("POST");

    // optionally specify custom headers
    CefRequest::HeaderMap headerMap;
    headerMap.insert(std::make_pair("X-My-Header", "My Header Value"));
    httpRequest->SetHeaderMap(headerMap);

    // 1. optionally specify upload content
    // 2. the default "Content-Type" header value is "application/x-www-form-urlencoded"
    // 3. set "Content-Type" via the HeaderMap if a different value is desired
    const std::string &uploadData = "arg1=val1&arg2=val2";

    CefRefPtr<CefPostData> postData = CefPostData::Create();
    CefRefPtr<CefPostDataElement> element = CefPostDataElement::Create();

    element->SetToBytes(uploadData.size(), uploadData.c_str());
    postData->AddElement(element);
    httpRequest->SetPostData(postData);

    // optionally set flags
    httpRequest->SetFlags(UR_FLAG_SKIP_CACHE);

    // create client and load
    CefRefPtr<CefURLRequest> urlRequest = frame->CreateURLRequest(httpRequest, new app::net::RequestClient(callback, base::BindOnce(&MenuBarBinding::onRequestComplete, base::Unretained(this))));

    return true;
}

bool MenuBarBinding::onTaskReverseData(CefRefPtr<CefBrowser> browser, CefRefPtr<CefFrame> frame, int64_t queryId, const CefString &request, bool persistent, CefRefPtr<Callback> callback, const std::string &messageName, const std::string &requestMessage)
{
    std::string result = requestMessage.substr(messageName.size() + 1);
    std::reverse(result.begin(), result.end());
    callback->Success(result);

    return true;
}

void MenuBarBinding::onRequestComplete(CefRefPtr<Callback> callback, CefURLRequest::ErrorCode errorCode, const std::string &downloadData)
{
    CEF_REQUIRE_UI_THREAD();

    if (errorCode == ERR_NONE)
    {
        callback->Success(downloadData);
    }
    else
    {
        callback->Failure(errorCode, "Request failed!");
    }

    callback = nullptr;
}

bool MenuBarBinding::onTaskLoadProject(CefRefPtr<CefBrowser> browser, CefRefPtr<CefFrame> frame, int64_t queryId, const CefString &request, bool persistent, CefRefPtr<Callback> callback){
    if(!VolumeViewer::isLoaded()){
        std::vector<CefString> filter={".lyp", ".lyp2", ".json"};
        CefRefPtr<CefRunFileDialogCallback> cbk = new FileDialogCallback(frame, callback);
        browser->GetHost()->RunFileDialog(FILE_DIALOG_OPEN, "Please select a project file", "", filter, cbk);
    }

    return true;
}

bool MenuBarBinding::onTaskWheelEvent(CefRefPtr<CefBrowser> browser, CefRefPtr<CefFrame> frame, int64_t queryId, const CefString &request, bool persistent, CefRefPtr<Callback> callback, const std::string &messageName, const std::string &requestMessage){
    
    return false;
}

void MenuBarBinding::init(CefRefPtr<CefMessageRouterBrowserSide> router)
{
    router->AddHandler(new MenuBarBinding(), false);
}

} // namespace binding
} // namespace app
