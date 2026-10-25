#pragma once

#include <wx/wx.h>
#include <thread>
#include <string>
#include <vector>
#include <cstdio>
#include <sstream>
#include <filesystem>
#include <atomic> // Thêm thư viện atomic để quản lý trạng thái luồng

#ifdef _WIN32
#include <windows.h> // Thêm windows.h để dùng CreateProcess (Ẩn CMD)
#define POPEN _popen
#define PCLOSE _pclose
#else
#define POPEN popen
#define PCLOSE pclose
#endif

enum dloption {
    DLO_AUTO = 0,
    DLO_USERSELECTED = 1,
    DLO_MP3 = 2
};

enum dloption2 {
    DLO2_NOPLAYLIST = 0,
    DLO2_ALLOWPLAYLIST = 1
};

enum GUIACTIVESTATE {
    GUI_ACTIVE_OVERRIDE_OFF = INT_MIN,
    GUI_INACTIVE_OVERRIDE = 0,
    GUI_URLINPUT = 1,
    GUI_TYPERADIO = (1 << 1),
    GUI_FETCHBTN = (1 << 2),
    GUI_FORMATCOMBO = (1 << 3),
    GUI_DLBTN = (1 << 4),
    GUI_LOGOUTPUT = (1 << 5),
    GUI_TYPERADIO2 = (1 << 6),

    GUI_ACTIVE_OVERRIDE = INT_MAX,
};

struct GUIActiveState {
    bool url_input = true;

    bool fetchbtn = true;
    bool formatcombo = true;

    bool typeradio = true;
    bool typeradio2 = true;

    bool dlbtn = true;
    bool logoutput = true;
    bool overrideState = true;
};

class MainFrame : public wxFrame {
public:
    MainFrame(const wxString& title);

private:
    GUIActiveState GUIState;
    wxTextCtrl* urlInput;

    wxRadioBox* typeRadio;
    wxRadioBox* typeRadio2;

    wxButton* fetchBtn;
    wxComboBox* formatCombo;

    wxButton* downloadBtn;
    wxButton* cancelBtn; // Thêm nút hủy tải xuống[cite: 2]
    wxTextCtrl* logOutput;

    std::atomic<bool> isCancelled; // Biến nhận diện quá trình có bị hủy hay không[cite: 2]

    // Hàm tiện ích để chạy command line trong thread riêng và ghi log ra UI
    void RunCommandAsync(const std::string& cmd, std::function<void(const wxString&)> onLine, std::function<void()> onFinished);

    void CheckAndInstallYtDlp();

    void OnRadioBoxSelection(wxCommandEvent& event);

    void OnFetchFormats(wxCommandEvent& event);

    void OnDownload(wxCommandEvent& event);

    void OnCancel(wxCommandEvent& event); // Sự kiện nhấn nút Hủy[cite: 2]
    void OnUrlEnter(wxCommandEvent& event); // Sự kiện nhấn Enter sau khi nhập URL[cite: 2]

    // Đổi tham số từ std::string thành wxString để hỗ trợ chuỗi có dấu được bọc bằng wxT()
    void LogMessage(const wxString& msg);

    void SetUIState(int state);
};

class YtDlpApp : public wxApp {
public:
    virtual bool OnInit();
};

bool checkDownloadFolder();
void downloadFolderMedic();