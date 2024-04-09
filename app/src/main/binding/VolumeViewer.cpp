#include "main/binding/VolumeViewer.hpp"

#include <commdlg.h>
#include <tchar.h>
#include <chrono>
#include <vector>

namespace app{
    namespace binding{
        struct ImportNodesInfo {
            std::vector<std::string> paths;
            bool bMerge{true};
            explicit ImportNodesInfo(const std::string &p) : paths{p} {}
            ImportNodesInfo() = default;
        };

        VolumeViewer::VolumeViewer() : viewer(new VolumeViewerCore(&reader)){
            m_thread = new std::thread(&VolumeViewer::mainLoop, this);
        }

        VolumeViewer::~VolumeViewer(){
            m_bRunning = false;
            m_thread->join();
            delete viewer;
        }

        void VolumeViewer::mainLoop(){
            // Periodic cycles.
            while(m_bRunning){
                auto start = std::chrono::high_resolution_clock::now();

                if(nullptr == reader.m_imageReader){
                    openImageFile();
                }

                if(nullptr != reader.m_imageReader){
                    importNodes();
                    operateNodes();
                    updateBlockSize();
                    updateCenter();
                    updateResolution();
                    reloadVolume();
                    updateChannel();
                    updateBlock();
                    exportVolume();
                    loadVolumeROI();
                    saveProject();
                }

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

        }

        void VolumeViewer::importNodes(){

        }

        void VolumeViewer::operateNodes(){

        }

        void VolumeViewer::updateBlockSize(){

        }

        void VolumeViewer::updateCenter(){

        }

        void VolumeViewer::updateResolution(){

        }

        void VolumeViewer::reloadVolume(){

        }

        void VolumeViewer::updateChannel(){

        }

        void VolumeViewer::updateBlock(){

        }

        void VolumeViewer::exportVolume(){

        }

        void VolumeViewer::loadVolumeROI(){

        }

        void VolumeViewer::saveProject(){
            
        }

        std::string VolumeViewer::getOpenFileName(){
            // Open file dialog and chose lychnis project file.
            OPENFILENAME ofn;
            // File path.
            TCHAR szFile[MAX_PATH] = _T("");
            ZeroMemory(&ofn, sizeof(OPENFILENAME));

            ofn.lStructSize = sizeof(OPENFILENAME);  
            ofn.hwndOwner = NULL;  
            ofn.lpstrFile = szFile;  
            ofn.nMaxFile = sizeof(szFile) / sizeof(szFile[0]);  
            ofn.lpstrFilter = _T("LYP files\0*.lyp;*.lyp2\0JSON files\0*.json\0");  
            ofn.nFilterIndex = 1;  
            ofn.lpstrFileTitle = NULL;  
            ofn.nMaxFileTitle = 0;  
            ofn.lpstrInitialDir = NULL;  
            ofn.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST;

            if(GetOpenFileName(&ofn) == FALSE) {
                return "";
            }

            // Get and Process file path. LPWSTR-->std::string
            int size_needed = WideCharToMultiByte(CP_UTF8, 0, ofn.lpstrFile, -1, NULL, 0, NULL, NULL);
            std::string filePath(size_needed, 0);
            WideCharToMultiByte(CP_UTF8, 0, ofn.lpstrFile, -1, &filePath[0], size_needed, NULL, NULL);
            std::replace(filePath.begin(), filePath.end(), '\\', '/');
            filePath.erase(std::find(filePath.begin(), filePath.end(), '\0'), filePath.end());

            return filePath;
        }

        bool VolumeViewer::onLoadProject(std::string& projectPath){
            // Only one project file is opened at a time.
            if (!reader.m_projectPath.empty() || !reader.m_imagePath.empty()){
                return false;
            }
            
            // If the projectPath is empty, app will open a file selection dialog.
            if (projectPath.empty()){
                projectPath = getOpenFileName();
                // Secondary testing.
                if (projectPath.empty()){
                    return false;
                }
            }

            // if(!reader.load(projectPath)){
            //     return false;
            // }

            // auto& params = reader.m_loadedProjectInfo;

            

            // if(reader.m_bAnnotation && reader.m_sliceThickness > 0){
            //     reader.m_bSplitSlices = true;
            //     if(reader.m_currentChannel < 0){
            //         reader.m_currentChannel = 0;
            //     }
            //     m_nextChannel = reader.m_currentChannel + 1;
            // }
            
            // delete m_imagePath2Load.exchange(new std::string(reader.m_imagePath));
            // if(reader.isBinary()){
            //     m_projectProto2Load.exchange(&reader);
            // }else if(params.contains("nodes")){
            //     delete m_nodesPath2Load.exchange(new ImportNodesInfo(projectPath));
            // }
            
            return true;
        }

        VolumeViewer& VolumeViewer::getInstance(){
            static VolumeViewer vvInstance;

            return vvInstance;
        }

        LychnisReader& VolumeViewer::getReader(std::string filePath){
            if(false == reader.isLoaded()){
                reader.load(filePath);
            }
        
            return reader;
        }
        
        VolumeViewerCore& VolumeViewer::getViewer(){
            if (viewer == nullptr){
                viewer = new VolumeViewerCore(&reader);
            }
            
            return *viewer;
        }

    }// namespace binding
}// namespace app