#include "application.h"

MainFrame::MainFrame(const wxString& title) : wxFrame(NULL, wxID_ANY, title, wxDefaultPosition, wxSize(800, 600)) {
	wxPanel* panel = new wxPanel(this, wxID_ANY);
	wxBoxSizer* vbox = new wxBoxSizer(wxVERTICAL);

	// --- Dòng 1: Nhập URL ---
	wxBoxSizer* hbox1 = new wxBoxSizer(wxHORIZONTAL);
	hbox1->Add(new wxStaticText(panel, wxID_ANY, wxT("URL Youtube:")), 0, wxRIGHT | wxALIGN_CENTER_VERTICAL, 10);

	// Thêm cờ wxTE_PROCESS_ENTER để có thể bắt sự kiện khi nhấn Enter[cite: 1]
	urlInput = new wxTextCtrl(panel, wxID_ANY, wxT(""), wxDefaultPosition, wxDefaultSize, wxTE_PROCESS_ENTER);
	urlInput->Bind(wxEVT_TEXT_ENTER, &MainFrame::OnUrlEnter, this);

	hbox1->Add(urlInput, 1, wxEXPAND);
	vbox->Add(hbox1, 0, wxEXPAND | wxALL, 10);

	// --- Dòng 3: Tùy chọn tải (Video/Audio) ---
	wxBoxSizer* hbox3 = new wxBoxSizer(wxHORIZONTAL);
	wxString choices[] = {
		wxT("Tự động chọn định dạng"),
		wxT("Tải theo định dạng người dùng chọn"),
		wxT("Chỉ tải Audio (Tự động chuyển sang MP3)") };
	typeRadio = new wxRadioBox(panel, wxID_ANY,
		wxT("Tùy chọn định dạng"),
		wxDefaultPosition, wxDefaultSize, 3, choices, 1, wxRA_SPECIFY_COLS);
	hbox3->Add(typeRadio, 1, wxEXPAND);
	vbox->Add(hbox3, 0, wxEXPAND | wxLEFT | wxRIGHT | wxBOTTOM, 10);
	typeRadio->Bind(wxEVT_RADIOBOX, &MainFrame::OnRadioBoxSelection, this);

	// --- Dòng 3: Tùy chọn tải 2 ---
	wxBoxSizer* dlo2 = new wxBoxSizer(wxHORIZONTAL);
	wxString dlo2choices[] = {
		wxT("Tải một video"),
		wxT("Tải toàn bộ danh sách phát chứa video") };
	typeRadio2 = new wxRadioBox(panel, wxID_ANY,
		wxT("Tùy chọn tải"),
		wxDefaultPosition, wxDefaultSize, 2, dlo2choices, 1, wxRA_SPECIFY_COLS);
	dlo2->Add(typeRadio2, 1, wxEXPAND);
	vbox->Add(dlo2, 0, wxEXPAND | wxLEFT | wxRIGHT | wxBOTTOM, 10);

	// --- Dòng 2: Nút lấy danh sách định dạng ---
	wxBoxSizer* hbox2 = new wxBoxSizer(wxHORIZONTAL);
	fetchBtn = new wxButton(panel, wxID_ANY, wxT("Lấy danh sách định dạng"));
	fetchBtn->Bind(wxEVT_BUTTON, &MainFrame::OnFetchFormats, this);
	hbox2->Add(fetchBtn, 0, wxRIGHT, 10);

	formatCombo = new wxComboBox(panel, wxID_ANY, wxT(""), wxDefaultPosition, wxDefaultSize, 0, NULL, wxCB_READONLY);
	hbox2->Add(formatCombo, 1, wxEXPAND);
	vbox->Add(hbox2, 0, wxEXPAND | wxLEFT | wxRIGHT | wxBOTTOM, 10);

	// --- Dòng 4: Nút Download và Nút Cancel (Mới) ---
	wxBoxSizer* hbox4 = new wxBoxSizer(wxHORIZONTAL); // Đặt chung hàng[cite: 1]
	downloadBtn = new wxButton(panel, wxID_ANY, wxT("Bắt đầu tải xuống"), wxDefaultPosition, wxSize(-1, 40));
	downloadBtn->Bind(wxEVT_BUTTON, &MainFrame::OnDownload, this);

	cancelBtn = new wxButton(panel, wxID_ANY, wxT("Hủy tải xuống"), wxDefaultPosition, wxSize(-1, 40));
	cancelBtn->Bind(wxEVT_BUTTON, &MainFrame::OnCancel, this);
	cancelBtn->Enable(false); // Khóa nút khi chưa tải

	hbox4->Add(downloadBtn, 1, wxEXPAND | wxRIGHT, 5);
	hbox4->Add(cancelBtn, 1, wxEXPAND | wxLEFT, 5);
	vbox->Add(hbox4, 0, wxEXPAND | wxLEFT | wxRIGHT | wxBOTTOM, 10);

	// --- Dòng 5: Cửa sổ Log ---
	logOutput = new wxTextCtrl(panel, wxID_ANY, wxT(""), wxDefaultPosition, wxDefaultSize, wxTE_MULTILINE | wxTE_READONLY | wxTE_RICH);
	wxFont font(10, wxFONTFAMILY_TELETYPE, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL);
	logOutput->SetFont(font);
	vbox->Add(logOutput, 1, wxEXPAND | wxLEFT | wxRIGHT | wxBOTTOM, 10);

	panel->SetSizer(vbox);

	isCancelled = false;

	// Khởi động luồng kiểm tra và cài đặt yt-dlp
	CheckAndInstallYtDlp();
	fetchBtn->Enable(GUIState.fetchbtn = false);
	formatCombo->Enable(GUIState.formatcombo = false);
}

// Bắt sự kiện khi nhập xong URL (bằng cách ấn Enter) để kích hoạt tự động lấy định dạng[cite: 1]
void MainFrame::OnUrlEnter(wxCommandEvent& event) {
	wxCommandEvent dummyEvent;
	OnFetchFormats(dummyEvent);
}

// Hàm xử lý việc dừng tiến trình tải xuống[cite: 1]
void MainFrame::OnCancel(wxCommandEvent& event) {
	if (!cancelBtn->IsEnabled()) return;
	isCancelled = true;
	LogMessage(wxT("\n[HỆ THỐNG] Đang tiến hành hủy tiến trình...\n"));
#ifdef _WIN32
	system("taskkill /IM yt-dlp.exe /F >nul 2>&1");
#else
	system("killall -9 yt-dlp >/dev/null 2>&1");
#endif
}

// Hàm chạy tiến trình ngầm, đã cấu hình để Ẩn Console Window nếu chạy ở Windows[cite: 1]
void MainFrame::RunCommandAsync(const std::string& cmd, std::function<void(const wxString&)> onLine, std::function<void()> onFinished) {
	std::thread([this, cmd, onLine, onFinished]() {
#ifdef _WIN32
		HANDLE hRead, hWrite;
		SECURITY_ATTRIBUTES sa = { sizeof(SECURITY_ATTRIBUTES), NULL, TRUE };
		if (!CreatePipe(&hRead, &hWrite, &sa, 0)) return;

		STARTUPINFOA si = { sizeof(STARTUPINFOA) };
		si.dwFlags = STARTF_USESTDHANDLES | STARTF_USESHOWWINDOW;
		si.hStdOutput = hWrite;
		si.hStdError = hWrite;
		si.wShowWindow = SW_HIDE; // Thuộc tính quyết định ẩn cửa sổ CMD[cite: 1]

		PROCESS_INFORMATION pi;
		std::string fullCmd = "cmd.exe /c " + cmd;

		if (CreateProcessA(NULL, (LPSTR)fullCmd.c_str(), NULL, NULL, TRUE, CREATE_NO_WINDOW, NULL, NULL, &si, &pi)) {
			CloseHandle(hWrite);
			char buffer[256];
			DWORD bytesRead;
			std::string lineStr;
			while (ReadFile(hRead, buffer, sizeof(buffer) - 1, &bytesRead, NULL) && bytesRead > 0) {
				buffer[bytesRead] = '\0';
				lineStr += buffer;
				size_t pos;
				while ((pos = lineStr.find('\n')) != std::string::npos) {
					std::string line = lineStr.substr(0, pos + 1);
					lineStr.erase(0, pos + 1);
					wxString wxLine = wxString::FromUTF8(line.c_str());
					CallAfter([onLine, wxLine]() { onLine(wxLine); });
				}
			}
			CloseHandle(hRead);
			WaitForSingleObject(pi.hProcess, INFINITE);
			CloseHandle(pi.hProcess);
			CloseHandle(pi.hThread);
		}
		else {
			CloseHandle(hRead);
			CloseHandle(hWrite);
		}
#else
		FILE* pipe = POPEN((cmd + " 2>&1").c_str(), "r");
		if (pipe) {
			char buffer[256];
			while (fgets(buffer, sizeof(buffer), pipe) != nullptr) {
				wxString line = wxString::FromUTF8(buffer);
				CallAfter([onLine, line]() { onLine(line); });
			}
			PCLOSE(pipe);
		}
#endif
		if (onFinished) {
			CallAfter(onFinished);
		}
		}).detach();
}

void MainFrame::CheckAndInstallYtDlp() {
	LogMessage(wxT("Đang kiểm tra cài đặt yt-dlp...\n"));
	SetUIState(GUI_INACTIVE_OVERRIDE);

	std::thread([this]() {
		FILE* pipe = POPEN("yt-dlp --version", "r");
		bool installed = false;
		if (pipe) {
			char buffer[128];
			if (fgets(buffer, sizeof(buffer), pipe) != nullptr) {
				installed = true;
			}
			PCLOSE(pipe);
		}

		if (!installed) {
			CallAfter([this]() { LogMessage(wxT("[HỆ THỐNG] Không tìm thấy yt-dlp. Đang tự động cài đặt qua winget...\n")); });
			std::string installCmd = "winget install --id yt-dlp.yt-dlp -e --accept-package-agreements --accept-source-agreements";

			// Việc ẩn cài đặt winget vẫn có thể sử dụng POPEN hoặc bạn có thể chuyển thành logic CreateProcess tương tự như trên
			FILE* pInstall = POPEN((installCmd + " 2>&1").c_str(), "r");
			if (pInstall) {
				char buffer[256];
				while (fgets(buffer, sizeof(buffer), pInstall) != nullptr) {
					wxString line = wxString::FromUTF8(buffer);
					CallAfter([this, line]() { LogMessage(line); });
				}
				PCLOSE(pInstall);
			}
			CallAfter([this]() { LogMessage(wxT("[HỆ THỐNG] Quá trình cài đặt hoàn tất.\n")); SetUIState(GUI_ACTIVE_OVERRIDE_OFF); });
		}
		else {
			CallAfter([this]() { LogMessage(wxT("[HỆ THỐNG] yt-dlp đã sẵn sàng!\n")); SetUIState(GUI_ACTIVE_OVERRIDE_OFF); });
		}
		}).detach();
}

void MainFrame::OnRadioBoxSelection(wxCommandEvent& event) {
	int selection = typeRadio->GetSelection();
	formatCombo->Clear();
	switch (selection) {
	case DLO_AUTO: {
		formatCombo->Append(wxT("Định dạng sẽ được chọn tự động bởi YT-DLP."));
		fetchBtn->Enable(false);
		formatCombo->Enable(false);
		break;
	}
	case DLO_USERSELECTED: {
		fetchBtn->Enable(true);
		formatCombo->Enable(true);
		break;
	}
	case DLO_MP3: {
		formatCombo->Append(wxT("Định dạng âm thanh sẽ được chọn là MP3."));
		fetchBtn->Enable(false);
		formatCombo->Enable(false);
		break;
	}
	default:
		break;
	}
}

void MainFrame::OnFetchFormats(wxCommandEvent& event) {
	formatCombo->Clear();

	if (typeRadio->GetSelection() == DLO_AUTO || typeRadio->GetSelection() == DLO_MP3) {
		return;
	}

	std::string url = urlInput->GetValue().ToStdString();
	if (url.empty()) {
		wxMessageBox(wxT("Vui lòng nhập URL!"), wxT("Lỗi"), wxICON_ERROR);
		return;
	}

	LogMessage(wxT("\nĐang lấy danh sách định dạng...\n"));
	SetUIState(GUI_INACTIVE_OVERRIDE);

	std::string cmd = "yt-dlp -F ";

	if (typeRadio2->GetSelection() == DLO2_NOPLAYLIST) {
		cmd += "--no-playlist ";
	}

	cmd += "\"" + url + "\"";

	RunCommandAsync(cmd,
		[this](const wxString& line) {
			LogMessage(line);
			std::string stdLine = line.ToStdString();
			if (stdLine.find("[") == std::string::npos &&
				stdLine.find("ID") == std::string::npos &&
				stdLine.find("---") == std::string::npos &&
				stdLine.find("storyboard") == std::string::npos &&
				stdLine.length() > 5) {

				wxString userReadableLine = "";
				std::istringstream lineStream(stdLine);
				std::string id, ext, res, fps, filesize;
				lineStream >> id >> ext >> res;
				while (lineStream >> filesize) {
					if (filesize == "|") break;
				}

				lineStream >> filesize;
				userReadableLine = id + "\t -------- VIDEO: " + ext + " " + res + ", AUDIO: BEST | SIZE: " + filesize;

				formatCombo->Append(userReadableLine);
			}
		},
		[this]() {
			LogMessage(wxT("=> Lấy danh sách định dạng thành công.\n"));
			if (formatCombo->GetCount() > 0) formatCombo->SetSelection(formatCombo->GetCount() - 1);
			SetUIState(GUI_ACTIVE_OVERRIDE_OFF);
		}
	);
}

void MainFrame::OnDownload(wxCommandEvent& event) {
	downloadFolderMedic();

	std::string url = urlInput->GetValue().ToStdString();
	if (url.empty()) {
		wxMessageBox(wxT("Vui lòng nhập URL!"), wxT("Lỗi"), wxICON_ERROR);
		return;
	}

	// Đổi vị trí lưu mặc định sang thư mục temp[cite: 1]
	std::string cmd = "yt-dlp -P " + (std::filesystem::current_path().string()) + "\\temp ";

	if (typeRadio2->GetSelection() == DLO2_NOPLAYLIST) {
		cmd += " --no-playlist ";
	}

	if (typeRadio->GetSelection() == DLO_AUTO) {
		cmd += "\"" + url + "\"";
	}
	else if (typeRadio->GetSelection() == DLO_MP3) {
		cmd += "-x --audio-format mp3 \"" + url + "\"";
	}
	else if (typeRadio->GetSelection() == DLO_USERSELECTED) {
		int sel = formatCombo->GetSelection();
		if (sel == wxNOT_FOUND) {
			wxMessageBox(wxT("Vui lòng lấy danh sách và chọn một định dạng."), wxT("Lỗi"), wxICON_ERROR);
			return;
		}
		std::string selectedLine = formatCombo->GetString(sel).ToStdString();
		std::string formatCode = selectedLine.substr(0, selectedLine.find_first_of(" \t"));

		cmd += "-f \"" + formatCode + "+bestaudio/best\" \"" + url + "\"";
	}

	LogMessage(wxT("\nBẮT ĐẦU TẢI XUỐNG:\n> ") + wxString::FromUTF8(cmd.c_str()) + wxT("\n"));
	SetUIState(GUI_INACTIVE_OVERRIDE);

	isCancelled = false;
	cancelBtn->Enable(true); // Kích hoạt nút Hủy trong quá trình tải xuống[cite: 1]

	RunCommandAsync(cmd,
		[this](const wxString& line) { LogMessage(line); },
		[this]() {
			cancelBtn->Enable(false); // Vô hiệu hóa nút hủy khi kết thúc
			if (!isCancelled) {
				// Di chuyển file từ temp sang downloads nếu quá trình thành công[cite: 1]
				try {
					std::filesystem::path tempPath = std::filesystem::current_path() / "temp";
					std::filesystem::path dlPath = std::filesystem::current_path() / "downloads";

					for (const auto& entry : std::filesystem::directory_iterator(tempPath)) {
						auto dest = dlPath / entry.path().filename();
						if (std::filesystem::exists(dest)) std::filesystem::remove(dest); // Ngừa file bị trùng
						std::filesystem::rename(entry.path(), dest);
					}
					LogMessage(wxT("\n=> HOÀN TẤT TẢI XUỐNG!\n"));
				}
				catch (const std::exception& e) {
					LogMessage(wxT("\n[LỖI] Không thể di chuyển tệp: ") + wxString::FromUTF8(e.what()) + wxT("\n"));
				}
			}
			else {
				// Nếu bị hủy, đưa ra thông báo phù hợp[cite: 1]
				LogMessage(wxT("\n=> ĐÃ HỦY TẢI XUỐNG!\n"));
			}
			SetUIState(GUI_ACTIVE_OVERRIDE_OFF);
		}
	);
}

void MainFrame::LogMessage(const wxString& msg) {
	logOutput->AppendText(msg);
}

void MainFrame::SetUIState(int state) {
	if (state == GUI_ACTIVE_OVERRIDE) {
		GUIState.overrideState = true;

		fetchBtn->Enable(GUIState.overrideState);
		downloadBtn->Enable(GUIState.overrideState);
		urlInput->Enable(GUIState.overrideState);
		typeRadio->Enable(GUIState.overrideState);
		formatCombo->Enable(GUIState.overrideState);
		typeRadio2->Enable(GUIState.overrideState);
		return;
	}
	if (state == GUI_INACTIVE_OVERRIDE) {
		GUIState.overrideState = false;

		fetchBtn->Enable(GUIState.overrideState);
		downloadBtn->Enable(GUIState.overrideState);
		urlInput->Enable(GUIState.overrideState);
		typeRadio->Enable(GUIState.overrideState);
		formatCombo->Enable(GUIState.overrideState);
		typeRadio2->Enable(GUIState.overrideState);
		return;
	}
	if (state == GUI_ACTIVE_OVERRIDE_OFF) {
		fetchBtn->Enable(GUIState.fetchbtn);
		downloadBtn->Enable(GUIState.dlbtn);
		urlInput->Enable(GUIState.url_input);
		typeRadio->Enable(GUIState.typeradio);
		formatCombo->Enable(GUIState.formatcombo);
		typeRadio2->Enable(GUIState.typeradio2);
		return;
	}

	fetchBtn->Enable(GUIState.fetchbtn = ((state & GUI_FETCHBTN) != 0));
	downloadBtn->Enable(GUIState.dlbtn = ((state & GUI_DLBTN) != 0));
	urlInput->Enable(GUIState.url_input = ((state & GUI_URLINPUT) != 0));
	typeRadio->Enable(GUIState.typeradio = ((state & GUI_TYPERADIO) != 0));
	formatCombo->Enable(GUIState.formatcombo = ((state & GUI_FORMATCOMBO) != 0));
	typeRadio2->Enable(GUIState.typeradio2 = ((state & GUI_TYPERADIO2) != 0));
}

bool YtDlpApp::OnInit() {
	MainFrame* frame = new MainFrame(wxT("Trình tải Youtube (yt-dlp GUI)"));
	frame->Show(true);
	return true;
}

bool checkDownloadFolder() {
	return std::filesystem::exists(std::filesystem::current_path() / "downloads");
}

void downloadFolderMedic() {
	if (!checkDownloadFolder()) {
		std::filesystem::create_directory(std::filesystem::current_path() / "downloads");
	}
	// Đảm bảo thư mục temp cũng luôn được tạo ra[cite: 1]
	if (!std::filesystem::exists(std::filesystem::current_path() / "temp")) {
		std::filesystem::create_directory(std::filesystem::current_path() / "temp");
	}
}