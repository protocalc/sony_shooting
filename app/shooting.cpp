#include <cstdlib>
#if defined(USE_EXPERIMENTAL_FS)
#include <experimental/filesystem>
namespace fs = std::experimental::filesystem;
#else
#include <filesystem>
namespace fs = std::filesystem;
#if defined(__APPLE__)
#include <unistd.h>
#endif
#endif
#include <signal.h>
#include <cstdint>
#include <chrono>
#include <thread>
#include <iomanip>
#include <iostream>
#include <fstream>
#include "CRSDK/CameraRemote_SDK.h"
#include "CameraDevice.h"
#include "Text.h"

//#define LIVEVIEW_ENB

namespace SDK = SCRSDK;
using namespace std::chrono_literals;

// Global dll object
// cli::CRLibInterface* cr_lib = nullptr;

static volatile bool keepShooting=true;

void signal_handler(int s)
{
    keepShooting=false;
}

int main(int argc, char* argv[])
{   

    if (strcmp(argv[1], "--help")==0)
    {
        cli::tout << "Help for the RX0mII controll \n";
        cli::tout << "Currentely the app just set the DateTime of the camera\n";
        cli::tout << "using the PC Unix time (second accuracy) and then start an interval shooting.\n";
        cli::tout << "The app needs to be run with the following syntax\n";
        cli::tout << "./shooting fps Number_of_Photo\n";
        cli::tout << "-- where fps is the number of frame per second to be taken\n";
        cli::tout << "-- and the Number_of_Photo is the number of photo to be taken (if -1 continues indefinetly)\n";
        cli::tout << "The software generates also a log with the timestamp with the info when the trigger is sent\n";
    }

    else
    {
        // Number of frame per second
        double fps = atof(argv[1]);

        // Number of photos to take
        int Nphoto = atof(argv[2]);

        // Define the logFile

        std::ofstream LogFile;
        LogFile.open("ShootingLog.log", std::ios::app);

        LogFile << "Unix Time (ms)" << "\t \t \t" << "\n";

        signal(SIGINT, signal_handler);

        // Change global locale to native locale
        std::locale::global(std::locale(""));

        // Make the stream's locale the same as the current global locale
        cli::tin.imbue(std::locale());
        cli::tout.imbue(std::locale());

        cli::tout << "RemoteSampleApp v1.05.00 running...\n\n";

        CrInt32u version = SDK::GetSDKVersion();
        int major = (version & 0xFF000000) >> 24;
        int minor = (version & 0x00FF0000) >> 16;
        int patch = (version & 0x0000FF00) >> 8;
        // int reserved = (version & 0x000000FF);

        cli::tout << "Remote SDK version: ";
        cli::tout << major << "." << minor << "." << std::setfill(TEXT('0')) << std::setw(2) << patch << "\n";

        // Load the library dynamically
        // cr_lib = cli::load_cr_lib();

        cli::tout << "Initialize Remote SDK...\n";
        
    #if defined(__APPLE__)
            char path[255]; /*MAX_PATH*/
            getcwd(path, sizeof(path) -1);
            cli::tout << "Working directory: " << path << '\n';
    #else
            cli::tout << "Working directory: " << fs::current_path() << '\n';
    #endif
        // auto init_success = cr_lib->Init(0);
        auto init_success = SDK::Init();
        if (!init_success) {
            cli::tout << "Failed to initialize Remote SDK. Terminating.\n";
            // cr_lib->Release();
            SDK::Release();
            std::exit(EXIT_FAILURE);
        }
        cli::tout << "Remote SDK successfully initialized.\n\n";

        cli::tout << "Enumerate connected camera devices...\n";
        SDK::ICrEnumCameraObjectInfo* camera_list = nullptr;
        // auto enum_status = cr_lib->EnumCameraObjects(&camera_list, 3);
        auto enum_status = SDK::EnumCameraObjects(&camera_list);
        if (CR_FAILED(enum_status) || camera_list == nullptr) {
            cli::tout << "No cameras detected. Connect a camera and retry.\n";
            // cr_lib->Release();
            SDK::Release();
            std::exit(EXIT_FAILURE);
        }
        auto ncams = camera_list->GetCount();

        cli::tsmatch smatch;
        CrInt32u no = 1;

        typedef std::shared_ptr<cli::CameraDevice> CameraDevicePtr;
        typedef std::vector<CameraDevicePtr> CameraDeviceList;
        CameraDeviceList cameraList; // all
        std::int32_t cameraNumUniq = 1;
        std::int32_t selectCamera = 1;

        cli::tout << "Connect to selected camera...\n";
        auto* camera_info = camera_list->GetCameraObjectInfo(no - 1);

        cli::tout << "Create camera SDK camera callback object.\n";
        CameraDevicePtr camera = CameraDevicePtr(new cli::CameraDevice(cameraNumUniq, nullptr, camera_info));
        cameraList.push_back(camera); // add 1st

        camera_list->Release();

        if (camera->is_connected()) {
            cli::tout << "Please disconnect\n";
        }
        else {
            camera->connect(SDK::CrSdkControlMode_Remote);
        }

        cli::tout << std::endl;

        std::this_thread::sleep_for(1000ms);

        cli::tout << "Set DateTime\n";

        camera->set_datetime();

        std::this_thread::sleep_for(1000ms);

        if (fps != 0)
        {
            double timer = 1/fps*1000;
            int count = 0;
            while (true && keepShooting)
            {   
                auto t1 = std::chrono::high_resolution_clock::now();
                camera->capture_image();
                

                LogFile << std::chrono::duration_cast<std::chrono::milliseconds>(t1.time_since_epoch()).count() << "\tShoot #\t" << count << "\n";

                ++count;

                if (Nphoto != -1 && count > Nphoto)
                {
                    break;
                }

                auto t2 = std::chrono::high_resolution_clock::now();
                
                std::chrono::duration<double, std::milli> elapsed = t2-t1;
                double diff = timer-elapsed.count();

                if (diff > 0)
                {
                    std::this_thread::sleep_for(std::chrono::duration<double, std::milli>(diff));
                }
            }   
        }

        if (camera->is_connected()) {
            camera->disconnect();
        }

        LogFile.close();

        cli::tout << "Release SDK resources.\n";
        // cr_lib->Release();
        SDK::Release();

        // cli::free_cr_lib(&cr_lib);

        cli::tout << "Exiting application.\n";
        std::exit(EXIT_SUCCESS);

        return 0;
    }
}
