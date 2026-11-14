#include "WorkshopMaps.h"

#include <winsock2.h>
#include <ws2tcpip.h>
#include <iphlpapi.h>
#include <windows.h>
#include <shellapi.h>
#include <objbase.h>
#include <oleauto.h>
#include <natupnp.h>

#include <thread>

#include <algorithm>
#include <cctype>
#include <cwctype>
#include <fstream>

#include "bakkesmod/wrappers/http/HttpWrapper.h"
#include "bakkesmod/wrappers/includes.h"
#include "imgui/imgui.h"
#include "imgui/imgui_stdlib.h"

namespace fs = std::filesystem;

BAKKESMOD_PLUGIN(WorkshopMaps, "Workshop Maps 2.0", "1.1.1", PLUGINTYPE_FREEPLAY)

namespace
{
	const char* kSite = "https://bakkesplugins.com/maps";
	const char* kCdn = "https://cdn.bakkesplugins.com/";
	const int kPort = 7777;
	const char* kTexturesUrl = "https://videogamemods.com/vgm/rocketleague/mods/workshop-textures";
	const char* kRadminUrl = "https://www.radmin-vpn.com/";
	// The shared files most workshop maps need. Without them maps show up black or pink.
	const char* kTextureFiles[] = {
		"EditorLandscapeResources.upk", "EditorMaterials.upk", "EditorMeshes.upk", "EditorResources.upk",
		"Engine_MI_Shaders.upk", "EngineBuildings.upk", "EngineDebugMaterials.upk", "EngineMaterials.upk",
		"EngineResources.upk", "EngineVolumetrics.upk", "MapTemplateIndex.upk", "MapTemplates.upk",
		"mods.upk", "NodeBuddies.upk" };
	const char* kSupportUrl = "https://buymeacoffee.com/unicornbaby.txt";
#ifndef WM_BUILD
#define WM_BUILD 0
#endif
	// Built files are published to the "dist" branch of the repo by GitHub Actions.
	const char* kUpdateBase = "https://raw.githubusercontent.com/unicornbabytxt/workshop-maps-2/dist/";

	// ---- Look and feel, applied only to this plugin's window ----
	const ImVec4 kAccent(0.20f, 0.62f, 0.98f, 1.f);
	const ImVec4 kAccentHover(0.32f, 0.70f, 1.00f, 1.f);
	const ImVec4 kAccentDown(0.14f, 0.52f, 0.88f, 1.f);
	const ImVec4 kGood(0.45f, 0.88f, 0.58f, 1.f);
	const ImVec4 kBad(1.00f, 0.47f, 0.42f, 1.f);
	const ImVec4 kWarn(1.00f, 0.78f, 0.32f, 1.f);
	int gThemeColors = 0, gThemeVars = 0;

	void PushTheme()
	{
		auto color = [](ImGuiCol idx, float r, float g, float b, float a = 1.f) { ImGui::PushStyleColor(idx, ImVec4(r, g, b, a)); ++gThemeColors; };
		auto var1 = [](ImGuiStyleVar idx, float v) { ImGui::PushStyleVar(idx, v); ++gThemeVars; };
		auto var2 = [](ImGuiStyleVar idx, float x, float y) { ImGui::PushStyleVar(idx, ImVec2(x, y)); ++gThemeVars; };
		gThemeColors = gThemeVars = 0;

		var1(ImGuiStyleVar_WindowRounding, 10.f);
		var1(ImGuiStyleVar_ChildRounding, 8.f);
		var1(ImGuiStyleVar_FrameRounding, 6.f);
		var1(ImGuiStyleVar_PopupRounding, 8.f);
		var1(ImGuiStyleVar_GrabRounding, 6.f);
		var1(ImGuiStyleVar_TabRounding, 6.f);
		var1(ImGuiStyleVar_ScrollbarRounding, 8.f);
		var1(ImGuiStyleVar_WindowBorderSize, 0.f);
		var1(ImGuiStyleVar_FrameBorderSize, 0.f);
		var2(ImGuiStyleVar_WindowPadding, 18.f, 16.f);
		var2(ImGuiStyleVar_FramePadding, 10.f, 6.f);
		var2(ImGuiStyleVar_ItemSpacing, 10.f, 9.f);

		color(ImGuiCol_WindowBg, 0.07f, 0.08f, 0.10f, 0.98f);
		color(ImGuiCol_ChildBg, 0.10f, 0.11f, 0.14f);
		color(ImGuiCol_PopupBg, 0.10f, 0.11f, 0.14f);
		color(ImGuiCol_Border, 0.20f, 0.22f, 0.27f);
		color(ImGuiCol_Separator, 0.18f, 0.20f, 0.25f);
		color(ImGuiCol_Text, 0.93f, 0.94f, 0.96f);
		color(ImGuiCol_TextDisabled, 0.52f, 0.55f, 0.62f);
		color(ImGuiCol_TitleBg, 0.07f, 0.08f, 0.10f);
		color(ImGuiCol_TitleBgActive, 0.07f, 0.08f, 0.10f);
		color(ImGuiCol_TitleBgCollapsed, 0.07f, 0.08f, 0.10f);
		color(ImGuiCol_FrameBg, 0.14f, 0.16f, 0.20f);
		color(ImGuiCol_FrameBgHovered, 0.18f, 0.20f, 0.25f);
		color(ImGuiCol_FrameBgActive, 0.20f, 0.23f, 0.29f);
		color(ImGuiCol_Button, 0.16f, 0.18f, 0.23f);
		color(ImGuiCol_ButtonHovered, 0.22f, 0.25f, 0.31f);
		color(ImGuiCol_ButtonActive, 0.26f, 0.29f, 0.36f);
		color(ImGuiCol_Header, 0.14f, 0.16f, 0.20f);
		color(ImGuiCol_HeaderHovered, 0.18f, 0.20f, 0.25f);
		color(ImGuiCol_HeaderActive, 0.20f, 0.23f, 0.29f);
		color(ImGuiCol_Tab, 0.10f, 0.11f, 0.14f);
		color(ImGuiCol_TabHovered, 0.17f, 0.53f, 0.97f);
		color(ImGuiCol_TabActive, 0.10f, 0.44f, 0.90f);
		color(ImGuiCol_CheckMark, kAccent.x, kAccent.y, kAccent.z);
		color(ImGuiCol_SliderGrab, kAccent.x, kAccent.y, kAccent.z);
		color(ImGuiCol_SliderGrabActive, kAccentHover.x, kAccentHover.y, kAccentHover.z);
		color(ImGuiCol_PlotHistogram, kAccent.x, kAccent.y, kAccent.z);
		color(ImGuiCol_ScrollbarBg, 0.07f, 0.08f, 0.10f, 0.f);
		color(ImGuiCol_ScrollbarGrab, 0.22f, 0.25f, 0.31f);
	}

	void PopTheme()
	{
		ImGui::PopStyleColor(gThemeColors);
		ImGui::PopStyleVar(gThemeVars);
	}

	// The main action button on a screen, in the accent color.
	bool PrimaryButton(const char* label)
	{
		ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.10f, 0.44f, 0.90f, 1.f));
		ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.17f, 0.53f, 0.97f, 1.f));
		ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.07f, 0.37f, 0.80f, 1.f));
		ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.f, 1.f, 1.f, 1.f));
		bool pressed = ImGui::Button(label);
		ImGui::PopStyleColor(4);
		return pressed;
	}

	// An on/off switch drawn as a button, so it's obvious what state it's in.
	bool ToggleButton(const char* name, bool on)
	{
		const std::string label = std::string(name) + (on ? ":  ON" : ":  OFF");
		if (on) return PrimaryButton(label.c_str());
		ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.75f, 0.78f, 0.84f, 1.f));
		bool pressed = ImGui::Button(label.c_str());
		ImGui::PopStyleColor();
		return pressed;
	}

	// Status text that wraps instead of running off the side of the window.
	void StatusLine(const std::string& text, bool isError)
	{
		if (text.empty()) return;
		ImGui::PushStyleColor(ImGuiCol_Text, isError ? kBad : kGood);
		ImGui::TextWrapped("%s", text.c_str());
		ImGui::PopStyleColor();
	}

	// Folder names like "Air_Dribble_Hoops" read better as "Air Dribble Hoops".
	std::string PrettyName(std::string s)
	{
		std::replace(s.begin(), s.end(), '_', ' ');
		for (size_t pos; (pos = s.find("  ")) != std::string::npos;) s.erase(pos, 1);
		while (!s.empty() && s.front() == ' ') s.erase(s.begin());
		while (!s.empty() && s.back() == ' ') s.pop_back();
		return s;
	}

	void SectionTitle(const char* text)
	{
		ImGui::Spacing();
		ImGui::TextColored(kAccent, "%s", text);
	}
	const char* kModeNames[] = { "Soccar", "Hoops", "Snow Day", "Rumble", "Dropshot", "Heatseeker" };
	const char* kModeClasses[] = { "TAGame.GameInfo_Soccar_TA", "TAGame.GameInfo_Basketball_TA", "TAGame.Gameinfo_Hockey_TA",
	                               "TAGame.GameInfo_Items_TA", "TAGame.GameInfo_Breakout_TA", "TAGame.GameInfo_GodBall_TA" };
	const char* kUserAgent = "Mozilla/5.0 (Windows NT 10.0; Win64; x64) WorkshopMapsPlugin/2.0";

	std::string Lower(std::string s)
	{
		std::transform(s.begin(), s.end(), s.begin(), [](unsigned char c) { return (char)std::tolower(c); });
		return s;
	}

	std::string Trim(const std::string& s)
	{
		size_t a = s.find_first_not_of(" \t\r\n");
		if (a == std::string::npos) return "";
		size_t b = s.find_last_not_of(" \t\r\n");
		return s.substr(a, b - a + 1);
	}

	void ReplaceAll(std::string& s, const std::string& from, const std::string& to)
	{
		for (size_t pos = 0; (pos = s.find(from, pos)) != std::string::npos; pos += to.size())
			s.replace(pos, from.size(), to);
	}

	// Makes a string safe to use as a Windows folder name.
	std::string SafeName(const std::string& s)
	{
		std::string out;
		for (unsigned char c : s)
		{
			if (c < 32 || c > 126) continue;
			if (std::string("<>:\"/\\|?*").find((char)c) != std::string::npos) continue;
			out += (char)c;
		}
		out = Trim(out);
		while (!out.empty() && (out.back() == '.' || out.back() == ' ')) out.pop_back();
		if (out.size() > 60) out = Trim(out.substr(0, 60));
		return out;
	}

	// Accepts "298", "/298", "maps/298", "https://www.bakkesplugins.com/maps/298?tab=versions", etc.
	std::string ParseMapId(const std::string& raw)
	{
		std::string s = Trim(raw);
		std::string lower = Lower(s);
		size_t start = 0;
		size_t marker = lower.find("maps/");
		if (marker != std::string::npos)
			start = marker + 5;
		else if (lower.find("http") != std::string::npos || lower.find(".com") != std::string::npos)
			return ""; // a link that isn't a map page
		while (start < s.size() && !std::isdigit((unsigned char)s[start])) ++start;

		std::string id;
		for (size_t i = start; i < s.size() && std::isdigit((unsigned char)s[i]); ++i) id += s[i];
		if (id.empty() || id.size() > 9) return "";
		return id;
	}

	// Pulls the first .zip link on the bakkesplugins CDN out of a page, whatever the page layout is.
	std::string FindZipUrl(std::string page)
	{
		ReplaceAll(page, "\\/", "/");
		ReplaceAll(page, "\\u0026", "&");
		ReplaceAll(page, "&amp;", "&");
		const std::string lower = Lower(page);
		for (size_t pos = 0; (pos = lower.find(kCdn, pos)) != std::string::npos; ++pos)
		{
			size_t end = page.find_first_of("\"'<> \\\r\n)", pos);
			if (end == std::string::npos) end = page.size();
			std::string url = page.substr(pos, end - pos);
			std::string path = Lower(url.substr(0, url.find_first_of("?#")));
			if (path.size() > 4 && path.compare(path.size() - 4, 4, ".zip") == 0)
				return url;
		}
		return "";
	}

	std::string FindTitle(const std::string& page)
	{
		std::string title;
		size_t og = page.find("og:title");
		if (og != std::string::npos)
		{
			size_t c = page.find("content=\"", og);
			size_t tagEnd = page.find('>', og);
			if (c != std::string::npos && c < tagEnd)
			{
				c += 9;
				size_t e = page.find('"', c);
				if (e != std::string::npos) title = page.substr(c, e - c);
			}
		}
		if (title.empty())
		{
			size_t a = page.find("<title>");
			size_t b = page.find("</title>");
			if (a != std::string::npos && b != std::string::npos && b > a) title = page.substr(a + 7, b - a - 7);
		}
		ReplaceAll(title, "&amp;", "&");
		ReplaceAll(title, "&#x27;", "'");
		ReplaceAll(title, "&#39;", "'");
		ReplaceAll(title, "&quot;", "");
		// Drop site suffixes like "Map Name - BakkesPlugins"
		for (const char* sep : { " - BakkesPlugins", " | BakkesPlugins" })
		{
			size_t p = title.find(sep);
			if (p != std::string::npos) title.erase(p);
		}
		return Trim(title);
	}

	bool IsMapFile(const fs::path& p)
	{
		std::string ext = Lower(p.extension().string());
		return ext == ".upk" || ext == ".udk";
	}

	// Finds the map file inside a folder. Renames .udk to .upk because that is what the game loads.
	fs::path FindMapFile(const fs::path& folder)
	{
		std::error_code ec;
		fs::path found;
		for (fs::recursive_directory_iterator it(folder, fs::directory_options::skip_permission_denied, ec), end; !ec && it != end; it.increment(ec))
		{
			if (!it->is_regular_file(ec) || !IsMapFile(it->path())) continue;
			if (Lower(it->path().extension().string()) == ".upk") return it->path();
			if (found.empty()) found = it->path();
		}
		if (!found.empty())
		{
			fs::path upk = found;
			upk.replace_extension(".upk");
			fs::rename(found, upk, ec);
			if (!ec) found = upk;
		}
		return found;
	}

	// Runs a program hidden and waits for it. Returns true if it exited with code 0.
	bool RunHidden(std::wstring commandLine)
	{
		STARTUPINFOW si{};
		si.cb = sizeof(si);
		PROCESS_INFORMATION pi{};
		if (!CreateProcessW(nullptr, commandLine.data(), nullptr, nullptr, FALSE, CREATE_NO_WINDOW, nullptr, nullptr, &si, &pi))
			return false;
		WaitForSingleObject(pi.hProcess, 10 * 60 * 1000);
		DWORD code = 1;
		GetExitCodeProcess(pi.hProcess, &code);
		CloseHandle(pi.hThread);
		CloseHandle(pi.hProcess);
		return code == 0;
	}

	fs::path DefaultMapsDir()
	{
		// RocketLeague.exe lives in <install>\Binaries\Win64
		wchar_t exe[MAX_PATH * 4] = {};
		GetModuleFileNameW(nullptr, exe, (DWORD)std::size(exe));
		return fs::path(exe).parent_path().parent_path().parent_path() / "TAGame" / "CookedPCConsole" / "mods";
	}

	// <install>\TAGame\CookedPCConsole, where the game keeps its own packages.
	fs::path CookedDir()
	{
		return DefaultMapsDir().parent_path();
	}

	bool IsTextureFile(const fs::path& p)
	{
		const std::string name = Lower(p.filename().string());
		for (const char* t : kTextureFiles)
			if (name == Lower(t)) return true;
		return false;
	}

}

void WorkshopMaps::onLoad()
{
	cvarManager->registerCvar("wm_maps_dir", "", "Workshop maps folder. Empty = <Rocket League>/TAGame/CookedPCConsole/mods");

	// Console shortcuts: "wm_install 298" and "wm_open"
	cvarManager->registerNotifier("wm_install", [this](std::vector<std::string> args) {
		if (args.size() > 1) InstallFromInput(args[1]);
	}, "Install a workshop map by bakkesplugins map number or link", PERMISSION_ALL);
	cvarManager->registerNotifier("wm_open", [this](std::vector<std::string>) {
		cvarManager->executeCommand("togglemenu " + GetMenuName());
	}, "Open the Workshop Maps window", PERMISSION_ALL);

	cvarManager->registerCvar("wm_freeplay", "0", "Load workshop maps in free play mode");
	freeplayMode_ = cvarManager->getCvar("wm_freeplay").getBoolValue();

	// Clean up the leftover file from the last self-update, then see if a newer build exists.
	std::error_code ec;
	fs::remove(gameWrapper->GetBakkesModPath() / "plugins" / "WorkshopMaps2.dll.old", ec);
	CheckForUpdate();

	cvarManager->registerNotifier("wm_spawnball", [this](std::vector<std::string>) { SpawnBall(); },
		"Spawn a ball above your car (host or solo only)", PERMISSION_ALL);

	EnsureAutoload();
	RefreshMaps();
	CheckTextures();
}

// Makes the plugin load by itself every time the game starts, so nobody has to type "plugin load" again.
void WorkshopMaps::EnsureAutoload()
{
	const std::string wanted = "plugin load workshopmaps2";
	const fs::path cfg = gameWrapper->GetBakkesModPath() / "cfg" / "plugins.cfg";
	std::vector<std::string> lines;
	bool present = false, changed = false;
	{
		std::ifstream in(cfg);
		std::string line;
		while (std::getline(in, line))
		{
			std::string t = Lower(Trim(line));
			if (t == wanted) present = true;
			if (t == "plugin load workshopmaps") { changed = true; continue; } // the 1.0 name
			lines.push_back(line);
		}
	}
	if (!present) { lines.push_back(wanted); changed = true; }
	if (!changed) return;
	std::ofstream out(cfg, std::ios::trunc);
	if (!out) { cvarManager->log("[WorkshopMaps] Couldn't write plugins.cfg, autoload not set."); return; }
	for (const std::string& line : lines) out << line << "\n";
}

fs::path WorkshopMaps::BackupDir()
{
	return gameWrapper->GetDataFolder() / "workshopmaps" / "duplicates_backup";
}

// Moves a map folder or file out of the game folder instead of deleting it.
bool WorkshopMaps::MoveToBackup(const fs::path& what)
{
	std::error_code ec;
	fs::path dir = BackupDir();
	fs::create_directories(dir, ec);
	fs::path dest = dir / what.filename();
	for (int n = 2; fs::exists(dest, ec) && n < 500; ++n)
		dest = dir / (what.stem().string() + " (" + std::to_string(n) + ")" + what.extension().string());
	fs::rename(what, dest, ec);
	if (!ec) return true;
	// Different drive: copy, then remove the original.
	ec.clear();
	fs::copy(what, dest, fs::copy_options::recursive, ec);
	if (ec) return false;
	fs::remove_all(what, ec);
	return !ec;
}

// Keeps the newest copy of every map and moves the other copies to the backup folder.
int WorkshopMaps::CleanDuplicates()
{
	std::vector<MapEntry> maps;
	{
		std::lock_guard<std::mutex> lock(mutex_);
		maps = maps_;
	}
	const fs::path root = MapsDir();
	std::error_code ec;
	int moved = 0;
	std::vector<bool> done(maps.size(), false);
	for (size_t i = 0; i < maps.size(); ++i)
	{
		if (done[i] || !maps[i].duplicate) continue;
		const std::string key = Lower(maps[i].file.filename().string());
		size_t keep = i;
		std::vector<size_t> group;
		for (size_t j = i; j < maps.size(); ++j)
		{
			if (Lower(maps[j].file.filename().string()) != key) continue;
			group.push_back(j);
			done[j] = true;
			if (fs::last_write_time(maps[j].file, ec) > fs::last_write_time(maps[keep].file, ec)) keep = j;
		}
		for (size_t j : group)
		{
			if (j == keep) continue;
			const MapEntry& m = maps[j];
			bool ownFolder = !m.folder.empty() && m.folder != root && m.folder.parent_path() == root;
			if (MoveToBackup(ownFolder ? m.folder : m.file)) ++moved;
		}
	}
	RefreshMaps();
	return moved;
}

fs::path WorkshopMaps::MapsDir()
{
	CVarWrapper cvar = cvarManager->getCvar("wm_maps_dir");
	std::string custom = cvar.IsNull() ? "" : Trim(cvar.getStringValue());
	fs::path dir = custom.empty() ? DefaultMapsDir() : fs::path(custom);
	std::error_code ec;
	fs::create_directories(dir, ec);
	return dir;
}

void WorkshopMaps::SetStatus(const std::string& text, bool isError)
{
	std::lock_guard<std::mutex> lock(mutex_);
	status_ = text;
	statusIsError_ = isError;
	if (cvarManager) cvarManager->log("[WorkshopMaps] " + text);
}

void WorkshopMaps::RefreshMaps()
{
	std::vector<MapEntry> found;
	std::error_code ec;
	fs::path dir = MapsDir();
	for (fs::directory_iterator it(dir, ec), end; !ec && it != end; it.increment(ec))
	{
		if (it->is_directory(ec))
		{
			fs::path file = FindMapFile(it->path());
			if (file.empty()) continue;
			MapEntry entry;
			entry.name = PrettyName(it->path().filename().string());
			entry.file = file;
			entry.folder = it->path();
			std::ifstream nameFile(it->path() / "mapname.txt");
			std::string saved;
			if (nameFile && std::getline(nameFile, saved) && !Trim(saved).empty()) entry.name = Trim(saved);
			std::ifstream idFile(it->path() / "mapid.txt");
			std::string savedId;
			if (idFile && std::getline(idFile, savedId)) entry.id = Trim(savedId);
			found.push_back(entry);
		}
		else if (it->is_regular_file(ec) && IsMapFile(it->path()))
		{
			MapEntry entry;
			entry.name = PrettyName(it->path().stem().string());
			entry.file = it->path();
			found.push_back(entry);
		}
	}
	std::sort(found.begin(), found.end(), [](const MapEntry& a, const MapEntry& b) { return Lower(a.name) < Lower(b.name); });
	// The game finds maps by file name, so two copies with the same file name confuse it and can crash it.
	for (size_t i = 0; i < found.size(); ++i)
		for (size_t j = i + 1; j < found.size(); ++j)
			if (Lower(found[i].file.filename().string()) == Lower(found[j].file.filename().string()))
				found[i].duplicate = found[j].duplicate = true;

	std::lock_guard<std::mutex> lock(mutex_);
	maps_ = std::move(found);
}

void WorkshopMaps::RefreshZips()
{
	std::vector<std::pair<fs::file_time_type, ZipEntry>> found;
	wchar_t profile[MAX_PATH] = {};
	if (GetEnvironmentVariableW(L"USERPROFILE", profile, MAX_PATH))
	{
		std::error_code ec;
		for (fs::directory_iterator it(fs::path(profile) / "Downloads", ec), end; !ec && it != end; it.increment(ec))
		{
			if (!it->is_regular_file(ec) || Lower(it->path().extension().string()) != ".zip") continue;
			found.push_back({ it->last_write_time(ec), { it->path().filename().string(), it->path() } });
		}
	}
	std::sort(found.begin(), found.end(), [](const auto& a, const auto& b) { return a.first > b.first; });
	if (found.size() > 20) found.resize(20);

	std::lock_guard<std::mutex> lock(mutex_);
	zips_.clear();
	for (auto& item : found) zips_.push_back(item.second);
}

void WorkshopMaps::InstallFromInput(std::string input)
{
	if (busy_) return;
	std::string id = ParseMapId(input);
	if (id.empty())
	{
		SetStatus("That doesn't look like a map number. Type just the number, like 298, or paste the map link.", true);
		return;
	}
	busy_ = true;
	progress_ = 0.f;
	SetStatus("Looking up map number " + id + "...");
	FetchPage(id, 0);
}

void WorkshopMaps::FetchPage(std::string id, int attempt)
{
	CurlRequest req;
	req.url = std::string(kSite) + "/" + id + (attempt == 0 ? "?tab=versions" : "");
	req.verb = "GET";
	req.headers["User-Agent"] = kUserAgent;
	req.headers["Accept"] = "text/html,*/*";

	HttpWrapper::SendCurlRequest(req, [this, id, attempt](int code, std::string data) {
		std::string zipUrl = code == 200 ? FindZipUrl(data) : "";
		if (zipUrl.empty())
		{
			if (attempt == 0) { FetchPage(id, 1); return; }
			busy_ = false;
			if (code == 404)
				SetStatus("Map " + id + " doesn't exist on bakkesplugins.com. Double check the number.", true);
			else if (code != 200)
				SetStatus("Couldn't reach bakkesplugins.com (error " + std::to_string(code) + "). Download the zip in your browser and use 'Install from a zip' below.", true);
			else
				SetStatus("Found map " + id + " but no download link on the page. Download the zip in your browser and use 'Install from a zip' below.", true);
			return;
		}
		DownloadZip(id, FindTitle(data), zipUrl);
	});
}

void WorkshopMaps::DownloadZip(std::string id, std::string title, std::string url)
{
	std::string folderName = SafeName(title);
	if (folderName.empty()) folderName = "Map " + id;
	SetStatus("Downloading " + folderName + "...");

	std::error_code ec;
	fs::path tmpDir = gameWrapper->GetDataFolder() / "workshopmaps";
	fs::create_directories(tmpDir, ec);
	fs::path zipPath = tmpDir / ("map_" + id + ".zip");
	fs::remove(zipPath, ec);

	CurlRequest req;
	req.url = url;
	req.verb = "GET";
	req.headers["User-Agent"] = kUserAgent;
	req.progress_function = [this](double total, double now, double, double) {
		if (total > 0) progress_ = (float)(now / total);
	};

	HttpWrapper::SendCurlRequest(req, zipPath.wstring(), [this, id, folderName, zipPath](int code, std::wstring) {
		std::error_code ec;
		std::string error;
		if (code != 200)
			error = "Download failed (error " + std::to_string(code) + ").";
		else if (!fs::exists(zipPath, ec) || fs::file_size(zipPath, ec) == 0)
			error = "Download came back empty.";
		else
		{
			SetStatus("Unpacking " + folderName + "...");
			if (InstallZip(zipPath, folderName, error))
				std::ofstream(MapsDir() / SafeName(folderName) / "mapid.txt") << id;
		}
		fs::remove(zipPath, ec);
		busy_ = false;
		if (!error.empty()) { SetStatus(error, true); return; }
		RefreshMaps();
		SetStatus("Installed " + folderName + " (map " + id + "). Hit Play next to it in the list.");
	});
}

bool WorkshopMaps::InstallZip(const fs::path& zip, const std::string& folderNameRaw, std::string& error)
{
	std::string folderName = SafeName(folderNameRaw);
	if (folderName.empty()) { error = "Bad map name."; return false; }

	std::error_code ec;
	fs::path target = MapsDir() / folderName;
	fs::remove_all(target, ec); // reinstalling replaces the old copy
	fs::create_directories(target, ec);
	if (ec) { error = "Couldn't create " + target.string() + ". If Rocket League is in Program Files, try running it once as admin or pick another maps folder in Settings."; return false; }

	// Windows 10 and 11 ship tar.exe which unpacks zips. PowerShell is the fallback.
	wchar_t sysDir[MAX_PATH] = {};
	GetSystemDirectoryW(sysDir, MAX_PATH);
	bool ok = RunHidden(L"\"" + std::wstring(sysDir) + L"\\tar.exe\" -xf \"" + zip.wstring() + L"\" -C \"" + target.wstring() + L"\"");
	if (!ok)
		ok = RunHidden(L"powershell.exe -NoProfile -NonInteractive -Command \"Expand-Archive -Force -LiteralPath '" + zip.wstring() + L"' -DestinationPath '" + target.wstring() + L"'\"");

	fs::path mapFile = ok ? FindMapFile(target) : fs::path();
	if (mapFile.empty())
	{
		fs::remove_all(target, ec);
		error = ok ? "That zip doesn't have a map file (.upk or .udk) in it." : "Couldn't unpack the zip.";
		return false;
	}

	std::ofstream(target / "mapname.txt") << folderNameRaw;

	// If an older copy of this same map is sitting in another folder, move it out so the game only sees one.
	RefreshMaps();
	std::vector<MapEntry> maps;
	{
		std::lock_guard<std::mutex> lock(mutex_);
		maps = maps_;
	}
	const fs::path root = MapsDir();
	const std::string key = Lower(mapFile.filename().string());
	for (const MapEntry& m : maps)
	{
		if (Lower(m.file.filename().string()) != key || m.folder == target) continue;
		bool ownFolder = !m.folder.empty() && m.folder != root && m.folder.parent_path() == root;
		MoveToBackup(ownFolder ? m.folder : m.file);
	}
	return true;
}

void WorkshopMaps::LoadMap(const MapEntry& map)
{
	if (freeplayMode_)
	{
		// Same game mode the real Free Play uses, so the ball and your own free play bindings come with it.
		std::string arena = map.file.string();
		std::replace(arena.begin(), arena.end(), '/', '\\');
		std::string command = "start " + arena + "?Game=TAGame.GameInfo_Tutorial_TA?GameTags=Freeplay";
		gameWrapper->Execute([this, command](GameWrapper*) { gameWrapper->ExecuteUnrealCommand(command); });
		return;
	}
	std::string path = map.file.generic_string();
	gameWrapper->Execute([this, path](GameWrapper*) {
		cvarManager->executeCommand("load_workshop \"" + path + "\"");
	});
}

void WorkshopMaps::CheckForUpdate()
{
	if (WM_BUILD <= 0) return; // local test build
	// Runs at game start and every time the window opens, but no more than once a minute.
	const auto now = std::chrono::steady_clock::now();
	if (lastUpdateCheck_.time_since_epoch().count() != 0 && now - lastUpdateCheck_ < std::chrono::seconds(60)) return;
	lastUpdateCheck_ = now;
	CurlRequest req;
	req.url = std::string(kUpdateBase) + "version.txt";
	req.verb = "GET";
	HttpWrapper::SendCurlRequest(req, [this](int code, std::string data) {
		std::string text = Trim(data);
		if (code != 200 || text.empty() || text.size() > 9) return;
		if (!std::all_of(text.begin(), text.end(), [](unsigned char c) { return std::isdigit(c); })) return;
		latestBuild_ = std::stoi(text);
	});
}

// Downloads the newest build next to the running plugin and swaps the files. Windows lets a loaded dll be renamed,
// so the old one is moved aside and removed on the next start.
void WorkshopMaps::InstallUpdate()
{
	if (updating_.exchange(true)) return;
	SetStatus("Downloading the update...");
	const fs::path plugins = gameWrapper->GetBakkesModPath() / "plugins";
	const fs::path current = plugins / "WorkshopMaps2.dll";
	const fs::path incoming = plugins / "WorkshopMaps2.dll.new";
	const fs::path old = plugins / "WorkshopMaps2.dll.old";
	std::error_code ec;
	fs::remove(incoming, ec);

	CurlRequest req;
	req.url = std::string(kUpdateBase) + "WorkshopMaps2.dll";
	req.verb = "GET";
	HttpWrapper::SendCurlRequest(req, incoming.wstring(), [this, current, incoming, old](int code, std::wstring) {
		std::error_code ec;
		bool valid = code == 200 && fs::exists(incoming, ec) && fs::file_size(incoming, ec) > 100 * 1024;
		if (valid)
		{
			char magic[2] = {};
			std::ifstream(incoming, std::ios::binary).read(magic, 2);
			valid = magic[0] == 'M' && magic[1] == 'Z';
		}
		if (!valid)
		{
			fs::remove(incoming, ec);
			updating_ = false;
			SetStatus("Update download failed. Try again in a few minutes.", true);
			return;
		}
		fs::remove(old, ec);
		ec.clear();
		fs::rename(current, old, ec);
		if (!ec) fs::rename(incoming, current, ec);
		updating_ = false;
		if (ec)
		{
			SetStatus("Couldn't swap in the update: " + ec.message(), true);
			return;
		}
		updated_ = true;
		SetStatus("Updated. Restart Rocket League to finish.");
	});
}

// Counts missing workshop texture files and finds copies sitting in the wrong place.
void WorkshopMaps::CheckTextures()
{
	const fs::path root = CookedDir();
	std::error_code ec;
	int missing = 0;
	for (const char* name : kTextureFiles)
		if (!fs::exists(root / name, ec)) ++missing;

	// The game finds packages by file name, so a second copy anywhere under this folder causes
	// "Ambiguous package name" popups. Some map downloads bundle their own copies.
	std::vector<fs::path> dupes;
	for (fs::recursive_directory_iterator it(root, fs::directory_options::skip_permission_denied, ec), end; !ec && it != end; it.increment(ec))
	{
		if (!it->is_regular_file(ec) || it->path().parent_path() == root) continue;
		if (IsTextureFile(it->path())) dupes.push_back(it->path());
	}

	texturesMissing_ = missing;
	std::lock_guard<std::mutex> lock(mutex_);
	textureDupes_ = std::move(dupes);
}

// Unpacks a downloaded textures zip and copies the files the game is missing into place.
void WorkshopMaps::InstallTexturesFromZip(const fs::path& zip)
{
	if (texturesBusy_.exchange(true)) return;
	SetStatus("Installing workshop textures...");
	const fs::path tmp = gameWrapper->GetDataFolder() / "workshopmaps" / "textures_tmp";
	std::thread([this, zip, tmp]() {
		std::error_code ec;
		fs::remove_all(tmp, ec);
		fs::create_directories(tmp, ec);

		wchar_t sysDir[MAX_PATH] = {};
		GetSystemDirectoryW(sysDir, MAX_PATH);
		bool ok = RunHidden(L"\"" + std::wstring(sysDir) + L"\\tar.exe\" -xf \"" + zip.wstring() + L"\" -C \"" + tmp.wstring() + L"\"");
		if (!ok)
			ok = RunHidden(L"powershell.exe -NoProfile -NonInteractive -Command \"Expand-Archive -Force -LiteralPath '" + zip.wstring() + L"' -DestinationPath '" + tmp.wstring() + L"'\"");

		const fs::path root = CookedDir();
		int found = 0, installed = 0, failed = 0;
		if (ok)
		{
			for (fs::recursive_directory_iterator it(tmp, fs::directory_options::skip_permission_denied, ec), end; !ec && it != end; it.increment(ec))
			{
				if (!it->is_regular_file(ec) || !IsTextureFile(it->path())) continue;
				++found;
				const fs::path dest = root / it->path().filename();
				std::error_code copyEc;
				if (fs::exists(dest, copyEc)) continue;
				fs::copy_file(it->path(), dest, copyEc);
				if (copyEc) ++failed; else ++installed;
			}
		}
		fs::remove_all(tmp, ec);
		CheckTextures();
		texturesBusy_ = false;

		if (!ok)
			SetStatus("Couldn't unpack " + zip.filename().string() + ".", true);
		else if (found == 0)
			SetStatus(zip.filename().string() + " doesn't have the workshop textures in it.", true);
		else if (failed > 0)
			SetStatus("Couldn't copy textures into the game folder. Start Rocket League as administrator once and try again.", true);
		else
			SetStatus("Workshop textures installed (" + std::to_string(installed) + " files). Restart Rocket League so maps pick them up.");
	}).detach();
}

// While textures are missing, watch the Downloads folder for a textures zip and install it by itself.
void WorkshopMaps::PollTextureZip()
{
	if (texturesMissing_ == 0 || texturesBusy_) return;
	const double now = ImGui::GetTime();
	if (now - lastTexturePoll_ < 3.0) return;
	lastTexturePoll_ = now;

	wchar_t profile[MAX_PATH] = {};
	if (!GetEnvironmentVariableW(L"USERPROFILE", profile, MAX_PATH)) return;
	std::error_code ec;
	fs::path best;
	fs::file_time_type bestTime{};
	for (fs::directory_iterator it(fs::path(profile) / "Downloads", ec), end; !ec && it != end; it.increment(ec))
	{
		if (!it->is_regular_file(ec) || Lower(it->path().extension().string()) != ".zip") continue;
		if (Lower(it->path().filename().string()).find("texture") == std::string::npos) continue;
		if (std::find(triedTextureZips_.begin(), triedTextureZips_.end(), it->path()) != triedTextureZips_.end()) continue;
		const auto t = it->last_write_time(ec);
		if (best.empty() || t > bestTime) { best = it->path(); bestTime = t; }
	}
	if (best.empty()) return;
	triedTextureZips_.push_back(best);
	InstallTexturesFromZip(best);
}

// Puts each texture file in the one place the game expects and moves extra copies to the backup folder.
int WorkshopMaps::FixTextureDuplicates()
{
	std::vector<fs::path> dupes;
	{
		std::lock_guard<std::mutex> lock(mutex_);
		dupes = textureDupes_;
	}
	const fs::path root = CookedDir();
	int fixed = 0;
	for (const fs::path& dupe : dupes)
	{
		std::error_code ec;
		const fs::path home = root / dupe.filename();
		if (!fs::exists(home, ec))
		{
			fs::rename(dupe, home, ec); // the game was missing this one, so this copy becomes the real one
			if (!ec) ++fixed;
		}
		else if (MoveToBackup(dupe))
			++fixed;
	}
	CheckTextures();
	RefreshMaps();
	return fixed;
}

// Looks for a virtual LAN app (Radmin VPN, ZeroTier, Tailscale, Hamachi) and this PC's address inside it.
// Friends on the same virtual network can join with that address and nobody has to touch a router.
void WorkshopMaps::RefreshVlans()
{
	std::vector<std::pair<std::string, std::string>> found;
	ULONG size = 16 * 1024;
	std::vector<unsigned char> buffer(size);
	const ULONG flags = GAA_FLAG_SKIP_ANYCAST | GAA_FLAG_SKIP_MULTICAST | GAA_FLAG_SKIP_DNS_SERVER;
	ULONG result = GetAdaptersAddresses(AF_INET, flags, nullptr, reinterpret_cast<IP_ADAPTER_ADDRESSES*>(buffer.data()), &size);
	if (result == ERROR_BUFFER_OVERFLOW)
	{
		buffer.resize(size);
		result = GetAdaptersAddresses(AF_INET, flags, nullptr, reinterpret_cast<IP_ADAPTER_ADDRESSES*>(buffer.data()), &size);
	}
	if (result == NO_ERROR)
	{
		for (IP_ADAPTER_ADDRESSES* a = reinterpret_cast<IP_ADAPTER_ADDRESSES*>(buffer.data()); a; a = a->Next)
		{
			if (a->OperStatus != IfOperStatusUp) continue;
			std::wstring text = std::wstring(a->Description ? a->Description : L"") + L" " + (a->FriendlyName ? a->FriendlyName : L"");
			std::transform(text.begin(), text.end(), text.begin(), [](wchar_t c) { return (wchar_t)std::towlower(c); });
			const char* label = nullptr;
			if (text.find(L"radmin") != std::wstring::npos) label = "Radmin VPN";
			else if (text.find(L"zerotier") != std::wstring::npos) label = "ZeroTier";
			else if (text.find(L"tailscale") != std::wstring::npos) label = "Tailscale";
			else if (text.find(L"hamachi") != std::wstring::npos) label = "Hamachi";
			if (!label) continue;
			for (IP_ADAPTER_UNICAST_ADDRESS* u = a->FirstUnicastAddress; u; u = u->Next)
			{
				sockaddr* sa = u->Address.lpSockaddr;
				if (!sa || sa->sa_family != AF_INET) continue;
				char ip[64] = {};
				if (inet_ntop(AF_INET, &reinterpret_cast<sockaddr_in*>(sa)->sin_addr, ip, sizeof(ip)))
					found.push_back({ label, ip });
				break;
			}
		}
	}
	std::lock_guard<std::mutex> lock(mutex_);
	vlans_ = std::move(found);
}

// Starts a real match on the map that friends can connect to. Same approach Rocket Plugin uses.
void WorkshopMaps::HostGame(const MapEntry& map)
{
	std::string arena = map.file.string();
	std::replace(arena.begin(), arena.end(), '/', '\\');
	int players = std::clamp(hostPlayers_, 2, 8) & ~1;
	std::string command = "open " + arena + "?Playtest?game=" + kModeClasses[hostMode_] +
		"?GameTags=BotsNone,PlayerCount" + std::to_string(players) +
		"?NumPublicConnections=10?NumOpenPublicConnections=10?Lan?Listen";
	gameWrapper->Execute([this, command](GameWrapper*) { gameWrapper->ExecuteUnrealCommand(command); });
	SetStatus("Hosting " + map.name + ". Send your friends the join code.");
}

// Join code looks like "73.12.4.9:7777/298". The part after the slash is the map number so the map can be installed first.
void WorkshopMaps::JoinGame(const std::string& codeRaw)
{
	std::string code = Trim(codeRaw);
	std::string mapId;
	size_t slash = code.find('/');
	if (slash != std::string::npos)
	{
		mapId = ParseMapId(code.substr(slash + 1));
		code = code.substr(0, slash);
	}
	bool valid = !code.empty() && std::all_of(code.begin(), code.end(), [](unsigned char c) {
		return std::isalnum(c) || c == '.' || c == ':' || c == '-';
	});
	if (!valid)
	{
		SetStatus("That join code doesn't look right. It should look like 73.12.4.9:7777/298", true);
		return;
	}
	if (code.find(':') == std::string::npos) code += ":" + std::to_string(kPort);

	if (!mapId.empty())
	{
		bool have = false;
		{
			std::lock_guard<std::mutex> lock(mutex_);
			for (const MapEntry& m : maps_) if (m.id == mapId) have = true;
		}
		if (!have)
		{
			if (!busy_) InstallFromInput(mapId);
			SetStatus("You don't have this map yet, installing it now. Hit Join again when it says Installed.");
			return;
		}
	}
	std::string command = "start " + code + "/?Lan?Password=";
	gameWrapper->Execute([this, command](GameWrapper*) { gameWrapper->ExecuteUnrealCommand(command); });
	SetStatus("Joining " + code + "...");
}

void WorkshopMaps::FindPublicIp()
{
	CurlRequest req;
	req.url = "https://api.ipify.org";
	req.verb = "GET";
	HttpWrapper::SendCurlRequest(req, [this](int code, std::string data) {
		std::string ip = Trim(data);
		bool ok = code == 200 && !ip.empty() && ip.size() < 46 && std::all_of(ip.begin(), ip.end(), [](unsigned char c) {
			return std::isxdigit(c) || c == '.' || c == ':';
		});
		if (!ok) { SetStatus("Couldn't look up your IP. Search 'what is my ip' in a browser and type it in.", true); return; }
		std::lock_guard<std::mutex> lock(mutex_);
		shareIp_ = ip;
	});
}

// Asks the router (UPnP) to send game traffic on port 7777 to this PC, so friends can connect without manual port forwarding.
void WorkshopMaps::TryOpenPort()
{
	SetStatus("Asking your router to open port 7777...");
	std::thread([this]() {
		// Find this PC's address on the home network.
		std::wstring localIp;
		WSADATA wsa{};
		if (WSAStartup(MAKEWORD(2, 2), &wsa) == 0)
		{
			SOCKET sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
			if (sock != INVALID_SOCKET)
			{
				sockaddr_in remote{};
				remote.sin_family = AF_INET;
				remote.sin_port = htons(53);
				inet_pton(AF_INET, "8.8.8.8", &remote.sin_addr);
				sockaddr_in local{};
				int len = sizeof(local);
				if (connect(sock, (sockaddr*)&remote, sizeof(remote)) == 0 && getsockname(sock, (sockaddr*)&local, &len) == 0)
				{
					wchar_t buf[64] = {};
					if (InetNtopW(AF_INET, &local.sin_addr, buf, 64)) localIp = buf;
				}
				closesocket(sock);
			}
			WSACleanup();
		}
		if (localIp.empty()) { SetStatus("Couldn't find this PC's network address.", true); return; }

		const char* failText = "Your router wouldn't open the port automatically. You'll need to forward UDP port 7777 by hand, or use a virtual LAN app.";
		HRESULT init = CoInitializeEx(nullptr, COINIT_MULTITHREADED);
		bool ok = false;
		IUPnPNAT* nat = nullptr;
		if (SUCCEEDED(CoCreateInstance(__uuidof(UPnPNAT), nullptr, CLSCTX_ALL, __uuidof(IUPnPNAT), (void**)&nat)) && nat)
		{
			IStaticPortMappingCollection* mappings = nullptr;
			if (SUCCEEDED(nat->get_StaticPortMappingCollection(&mappings)) && mappings)
			{
				BSTR proto = SysAllocString(L"UDP");
				BSTR client = SysAllocString(localIp.c_str());
				BSTR desc = SysAllocString(L"Workshop Maps 2.0");
				IStaticPortMapping* mapping = nullptr;
				mappings->Remove(kPort, proto); // clear a stale mapping pointing at another PC
				if (SUCCEEDED(mappings->Add(kPort, proto, kPort, client, VARIANT_TRUE, desc, &mapping)) && mapping)
				{
					ok = true;
					mapping->Release();
				}
				SysFreeString(proto);
				SysFreeString(client);
				SysFreeString(desc);
				mappings->Release();
			}
			nat->Release();
		}
		if (SUCCEEDED(init)) CoUninitialize();
		if (ok) SetStatus("Router port 7777 is open. Friends should be able to join with your code.");
		else SetStatus(failText, true);
	}).detach();
}

void WorkshopMaps::SpawnBall()
{
	gameWrapper->Execute([this](GameWrapper* gw) {
		ServerWrapper server = gw->GetGameEventAsServer();
		if (server.IsNull())
		{
			SetStatus("Load a map first. Only the host (or solo play) can spawn a ball.", true);
			return;
		}
		CarWrapper car = gw->GetLocalCar();
		Vector where = car.IsNull() ? Vector(0, 0, 300) : car.GetLocation();
		where.Z += 250;
		server.SpawnBall(where, true, false);
		SetStatus("Ball spawned above your car.");
	});
}

void WorkshopMaps::RenderFriendsTab(const std::vector<MapEntry>& maps, bool busy)
{
	ImGui::TextWrapped("One person hosts, everyone else joins with a code. Everybody needs this plugin and has to launch with anti-cheat disabled.");
	ImGui::Separator();

	// ---- Host ----
	SectionTitle("HOST A GAME");
	if (maps.empty())
		ImGui::TextDisabled("Install a map on the Workshop Maps tab first.");
	else
	{
		if (hostMap_ < 0 || hostMap_ >= (int)maps.size()) hostMap_ = 0;
		ImGui::PushItemWidth(300);
		if (ImGui::BeginCombo("Map", maps[hostMap_].name.c_str()))
		{
			for (int i = 0; i < (int)maps.size(); ++i)
			{
				ImGui::PushID(i);
				if (ImGui::Selectable(maps[i].name.c_str(), i == hostMap_)) hostMap_ = i;
				ImGui::PopID();
			}
			ImGui::EndCombo();
		}
		ImGui::Combo("Game mode", &hostMode_, kModeNames, IM_ARRAYSIZE(kModeNames));
		ImGui::SliderInt("Max players", &hostPlayers_, 2, 8);
		ImGui::PopItemWidth();
		if (PrimaryButton("Host this map")) HostGame(maps[hostMap_]);
		ImGui::SameLine();
		ImGui::TextDisabled("You pick teams in game like a normal private match.");

		ImGui::Spacing();
		ImGui::TextUnformatted("Your join code");
		std::string ip;
		{
			std::lock_guard<std::mutex> lock(mutex_);
			ip = shareIp_;
		}
		ImGui::PushItemWidth(200);
		if (ImGui::InputTextWithHint("##ip", "Your IP address", &ip))
		{
			std::lock_guard<std::mutex> lock(mutex_);
			shareIp_ = ip;
		}
		ImGui::PopItemWidth();
		ImGui::SameLine();
		if (ImGui::Button("Find my IP")) FindPublicIp();
		if (!Trim(ip).empty())
		{
			std::string code = Trim(ip) + ":" + std::to_string(kPort);
			if (!maps[hostMap_].id.empty()) code += "/" + maps[hostMap_].id;
			ImGui::Text("Code: %s", code.c_str());
			ImGui::SameLine();
			if (ImGui::Button("Copy code")) ImGui::SetClipboardText(code.c_str());
		}
		// ---- Easiest path: a virtual LAN app, no router work ----
		SectionTitle("NO ROUTER SETUP (EASIEST)");
		std::vector<std::pair<std::string, std::string>> vlans;
		{
			std::lock_guard<std::mutex> lock(mutex_);
			vlans = vlans_;
		}
		if (vlans.empty())
		{
			ImGui::TextWrapped("Everybody installs a free virtual LAN app like Radmin VPN and joins the same network in it. "
				"Come back here after and the plugin picks up your address by itself.");
			if (ImGui::Button("Get Radmin VPN (opens browser)"))
				ShellExecuteA(nullptr, "open", kRadminUrl, nullptr, nullptr, SW_SHOWNORMAL);
			ImGui::SameLine();
			if (ImGui::Button("Check again")) RefreshVlans();
		}
		else
		{
			for (int i = 0; i < (int)vlans.size(); ++i)
			{
				ImGui::PushID(2000 + i);
				ImGui::Text("%s found: %s", vlans[i].first.c_str(), vlans[i].second.c_str());
				ImGui::SameLine();
				if (ImGui::SmallButton("Use this address"))
				{
					std::lock_guard<std::mutex> lock(mutex_);
					shareIp_ = vlans[i].second;
				}
				ImGui::PopID();
			}
			ImGui::TextWrapped("Friends have to be in the same network in that app. Nobody needs to touch a router.");
		}

		SectionTitle("OR OPEN YOUR ROUTER");
		if (ImGui::Button("Open my router port automatically")) TryOpenPort();
		ImGui::TextWrapped("If the router refuses, forward UDP port 7777 to this PC in your router settings.");
	}

	ImGui::Separator();

	// ---- Join ----
	SectionTitle("JOIN A GAME");
	ImGui::PushItemWidth(300);
	bool enter = ImGui::InputTextWithHint("##join", "Paste the host's join code", &joinBuf_, ImGuiInputTextFlags_EnterReturnsTrue);
	ImGui::PopItemWidth();
	ImGui::SameLine();
	if (PrimaryButton("Join") || enter) JoinGame(joinBuf_);
	ImGui::TextWrapped("If you don't have the map, the plugin installs it first. Then hit Join again. If the join fails right after an install, restart Rocket League once and try again.");
	if (busy) ImGui::ProgressBar(progress_.load(), ImVec2(-1, 0));
}

void WorkshopMaps::SetImGuiContext(uintptr_t ctx)
{
	ImGui::SetCurrentContext(reinterpret_cast<ImGuiContext*>(ctx));
}

bool WorkshopMaps::ShouldBlockInput()
{
	return ImGui::GetIO().WantCaptureMouse || ImGui::GetIO().WantCaptureKeyboard;
}

void WorkshopMaps::OnOpen()
{
	windowOpen_ = true;
	dirBuf_ = MapsDir().string();
	RefreshMaps();
	RefreshZips();
	CheckTextures();
	RefreshVlans();
	triedTextureZips_.clear();
	CheckForUpdate();
}

void WorkshopMaps::RenderSettings()
{
	ImGui::TextUnformatted("Install and play workshop maps without touching the console.");
	if (ImGui::Button("Open Workshop Maps"))
	{
		gameWrapper->Execute([this](GameWrapper*) { cvarManager->executeCommand("togglemenu " + GetMenuName()); });
	}
	ImGui::TextUnformatted("Tip: in F2 > Bindings, bind a key to:  togglemenu workshopmaps2");
}

void WorkshopMaps::Render()
{
	PushTheme();
	ImGui::SetNextWindowSize(ImVec2(640, 760), ImGuiCond_FirstUseEver);
	if (ImGui::Begin(GetMenuTitle().c_str(), &windowOpen_, ImGuiWindowFlags_NoCollapse))
		RenderBody();
	ImGui::End();
	PopTheme();

	if (!windowOpen_)
		cvarManager->executeCommand("togglemenu " + GetMenuName());
}

void WorkshopMaps::RenderBody()
{
	// Copy shared state once per frame so the UI never holds the lock.
	std::vector<MapEntry> maps;
	std::vector<ZipEntry> zips;
	std::string status;
	bool statusIsError = false;
	{
		std::lock_guard<std::mutex> lock(mutex_);
		maps = maps_;
		zips = zips_;
		status = status_;
		statusIsError = statusIsError_;
	}
	const bool busy = busy_;

	// ---- Update banner ----
	if (updated_)
		ImGui::TextColored(kGood, "Update installed. Restart Rocket League to finish.");
	else if (latestBuild_ > WM_BUILD && WM_BUILD > 0)
	{
		ImGui::TextColored(kWarn, "A newer version is out.");
		ImGui::SameLine();
		if (updating_) ImGui::TextDisabled("Updating...");
		else if (PrimaryButton("Update now")) InstallUpdate();
	}

	// ---- Workshop textures ----
	const int texturesMissing = texturesMissing_;
	if (texturesMissing > 0)
	{
		ImGui::TextColored(kWarn, "Workshop textures missing (%d of %d). Maps can look black or pink without them.",
			texturesMissing, (int)IM_ARRAYSIZE(kTextureFiles));
		if (texturesBusy_)
			ImGui::TextDisabled("Installing textures...");
		else
		{
			if (PrimaryButton("Get textures"))
				ShellExecuteA(nullptr, "open", kTexturesUrl, nullptr, nullptr, SW_SHOWNORMAL);
			ImGui::SameLine();
			ImGui::TextDisabled("Download the zip. The plugin installs it by itself.");
		}
		PollTextureZip();
	}
	size_t textureDupeCount = 0;
	{
		std::lock_guard<std::mutex> lock(mutex_);
		textureDupeCount = textureDupes_.size();
	}
	if (textureDupeCount > 0)
	{
		ImGui::TextColored(kWarn, "%d texture files are in the wrong place. That causes 'Ambiguous package name' popups.", (int)textureDupeCount);
		ImGui::SameLine();
		if (ImGui::SmallButton("Fix it"))
		{
			int fixed = FixTextureDuplicates();
			SetStatus("Fixed " + std::to_string(fixed) + " texture files. Restart Rocket League.");
		}
	}

	if (!ImGui::BeginTabBar("##tabs")) return;
	if (ImGui::BeginTabItem("Workshop Maps"))
	{
	// ---- Install by number ----
	SectionTitle("INSTALL A MAP");
	ImGui::PushItemWidth(260);
	bool enter = ImGui::InputTextWithHint("##mapid", "Map number or link (ex: 298)", &inputBuf_, ImGuiInputTextFlags_EnterReturnsTrue);
	ImGui::PopItemWidth();
	ImGui::SameLine();
	if (busy)
		ImGui::TextUnformatted("Working...");
	else if (PrimaryButton("Install") || enter)
	{
		InstallFromInput(inputBuf_);
		inputBuf_.clear();
	}
	ImGui::SameLine();
	if (ImGui::Button("Find maps (opens browser)"))
		ShellExecuteA(nullptr, "open", kSite, nullptr, nullptr, SW_SHOWNORMAL);
	ImGui::TextDisabled("The number is the end of the map's page address: bakkesplugins.com/maps/298");

	if (busy) ImGui::ProgressBar(progress_.load(), ImVec2(-1, 0));
	StatusLine(status, statusIsError);

	// ---- My maps ----
	SectionTitle("MY MAPS");
	ImGui::SameLine();
	ImGui::TextDisabled("%d", (int)maps.size());
	ImGui::SameLine();
	ImGui::PushItemWidth(180);
	ImGui::InputTextWithHint("##filter", "Search", &filterBuf_);
	ImGui::PopItemWidth();
	ImGui::SameLine();
	if (ImGui::Button("Refresh")) { RefreshMaps(); RefreshZips(); CheckTextures(); RefreshVlans(); }
	ImGui::SameLine();
	if (ImGui::Button("Open folder"))
		ShellExecuteW(nullptr, L"open", MapsDir().wstring().c_str(), nullptr, nullptr, SW_SHOWNORMAL);

	int duplicateCount = 0;
	for (const MapEntry& m : maps) if (m.duplicate) ++duplicateCount;
	if (duplicateCount > 0)
	{
		ImGui::TextColored(kWarn, "%d maps have duplicate copies. Duplicates can crash the game on load.", duplicateCount);
		if (ImGui::Button("Clean up duplicates (keeps the newest copy)"))
		{
			int moved = CleanDuplicates();
			SetStatus("Moved " + std::to_string(moved) + (moved == 1 ? " duplicate copy" : " duplicate copies") + " to the backup folder. Restart Rocket League before playing.");
		}
	}

	const std::string filter = Lower(filterBuf_);
	if (ToggleButton("Free play mode", freeplayMode_))
	{
		freeplayMode_ = !freeplayMode_;
		cvarManager->getCvar("wm_freeplay").setValue(freeplayMode_ ? 1 : 0);
		cvarManager->executeCommand("writeconfig", false);
	}
	ImGui::SameLine();
	ImGui::TextDisabled(freeplayMode_ ? "Maps load as Free Play, with a ball and your free play bindings."
	                                  : "Click to load maps as Free Play, with a ball and your free play bindings.");

	// The list takes whatever room is left, so the sections and footer below it stay on screen.
	const float listHeight = (std::max)(180.f, ImGui::GetContentRegionAvail().y - 185.f);
	ImGui::BeginChild("##maps", ImVec2(0, listHeight), true);
	if (maps.empty())
		ImGui::TextWrapped("No maps yet. Type a map number above and hit Install.");
	for (int i = 0; i < (int)maps.size(); ++i)
	{
		const MapEntry& map = maps[i];
		if (!filter.empty() && Lower(map.name).find(filter) == std::string::npos) continue;
		ImGui::PushID(i);
		if (PrimaryButton("Play")) LoadMap(map);
		ImGui::SameLine();
		ImGui::TextUnformatted(map.name.c_str());
		if (map.duplicate)
		{
			ImGui::SameLine();
			ImGui::TextColored(kWarn, "(duplicate)");
		}
		ImGui::SameLine(ImGui::GetWindowContentRegionWidth() - 110);
		if (confirmDelete_ == i)
		{
			if (ImGui::SmallButton("Yes, delete"))
			{
				std::error_code ec;
				const fs::path root = MapsDir();
				if (!map.folder.empty() && map.folder != root && map.folder.parent_path() == root)
					fs::remove_all(map.folder, ec);
				else
					fs::remove(map.file, ec);
				confirmDelete_ = -1;
				RefreshMaps();
			}
			ImGui::SameLine();
			if (ImGui::SmallButton("No")) confirmDelete_ = -1;
		}
		else if (ImGui::SmallButton("Delete"))
			confirmDelete_ = i;
		ImGui::PopID();
	}
	ImGui::EndChild();

	// ---- Extras ----
	if (ImGui::CollapsingHeader("Extras"))
	{
		ImGui::TextWrapped("True Ball Height Indicator fixes the ball's height ring on maps where the floor isn't at ground level, "
			"like the Dribble Challenge maps. It's a separate plugin from the BakkesMod team.");
		std::error_code extrasEc;
		if (fs::exists(gameWrapper->GetBakkesModPath() / "plugins" / "TrueBallHeightIndicator.dll", extrasEc))
			ImGui::TextColored(kGood, "True Ball Height Indicator is installed. Turn it on or off under F2 > Plugins.");
		else if (ImGui::Button("Install True Ball Height Indicator"))
		{
			gameWrapper->Execute([this](GameWrapper*) { cvarManager->executeCommand("bpm_install 169"); });
			SetStatus("Asked BakkesMod to install True Ball Height Indicator. Look for it under F2 > Plugins.");
		}
	}

	// ---- Backup: install from a zip already downloaded ----
	if (ImGui::CollapsingHeader("Install from a zip in my Downloads folder"))
	{
		ImGui::TextWrapped("Backup plan if the number install can't reach the site. Download the map zip in your browser, then pick it here.");
		if (zips.empty()) ImGui::TextDisabled("No .zip files found in Downloads.");
		for (int i = 0; i < (int)zips.size(); ++i)
		{
			ImGui::PushID(1000 + i);
			if (!busy && ImGui::SmallButton("Install"))
			{
				std::string error;
				std::string name = zips[i].file.stem().string();
				if (InstallZip(zips[i].file, name, error))
				{
					RefreshMaps();
					SetStatus("Installed " + name + ". Hit Play next to it in the list.");
				}
				else
					SetStatus(error, true);
			}
			ImGui::SameLine();
			ImGui::TextUnformatted(zips[i].name.c_str());
			ImGui::PopID();
		}
	}

	// ---- Settings ----
	if (ImGui::CollapsingHeader("Settings"))
	{
		ImGui::TextUnformatted("Maps folder");
		ImGui::PushItemWidth(-1);
		ImGui::InputText("##dir", &dirBuf_);
		ImGui::PopItemWidth();
		if (ImGui::Button("Save folder"))
		{
			cvarManager->getCvar("wm_maps_dir").setValue(Trim(dirBuf_));
			cvarManager->executeCommand("writeconfig", false);
			dirBuf_ = MapsDir().string();
			RefreshMaps();
		}
		ImGui::SameLine();
		if (ImGui::Button("Use default (Rocket League mods folder)"))
		{
			cvarManager->getCvar("wm_maps_dir").setValue(std::string(""));
			cvarManager->executeCommand("writeconfig", false);
			dirBuf_ = MapsDir().string();
			RefreshMaps();
		}
	}

		ImGui::EndTabItem();
	}
	if (ImGui::BeginTabItem("Play with Friends"))
	{
		RenderFriendsTab(maps, busy);
		StatusLine(status, statusIsError);
		ImGui::EndTabItem();
	}
	ImGui::EndTabBar();

	ImGui::Spacing();
	ImGui::TextDisabled("Workshop Maps 2.0  build %d", WM_BUILD);
	ImGui::SameLine(ImGui::GetWindowContentRegionWidth() - 150);
	if (ImGui::SmallButton("Support the plugin"))
		ShellExecuteA(nullptr, "open", kSupportUrl, nullptr, nullptr, SW_SHOWNORMAL);
	if (ImGui::IsItemHovered())
		ImGui::SetTooltip("Free forever. If it made your life easier, tips are appreciated.");
}
