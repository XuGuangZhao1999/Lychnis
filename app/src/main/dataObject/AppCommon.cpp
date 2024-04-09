#include "main/dataObject/AppCommon.hpp"

namespace app{
    namespace dataObject{
        app = nullptr;

        AppCommon::AppCommon(){
            reader = nullptr;
            viewer = nullptr;
        }

        AppCommon::~AppCommon(){
            rlease();
        }

        AppCommon& AppCommon::getInstance(){
            static AppCommon app;

            return app;
        }

        LychnisReader& AppCommon::getReader(std::string filePath){
            if (reader == nullptr){
                reader = new LychnisReader(filePath);
            }
            
            return *reader;
        }
        
        VolumeViewerCore& AppCommon::getViewer(){
            if (viewer == nullptr && reader != nullptr){
                viewer = new VolumeViewerCore(reader);
            }
            
            return *viewer;
        }

        void AppCommon::rlease(){
            delete viewer;
            delete reader;
        }
    }// namespace dataObject
}// namespace app