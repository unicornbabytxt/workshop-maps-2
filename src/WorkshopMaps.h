#pragma once

#include "bakkesmod/plugin/bakkesmodplugin.h"
#include "bakkesmod/plugin/pluginwindow.h"
#include "bakkesmod/plugin/PluginSettingsWindow.h"

#include <atomic>
#include <filesystem>
#include <memory>
#include <mutex>
#include <string>
#include <vector>

struct MapEntry
{
	std::string name;               // shown in the list
	std::filesystem::path file;     // the .upk that gets loaded
	std::filesystem::path folder;   // folder to delete when removing (empty for loose files)
	std::string id;                 // bakkesplugins map number, if installed through the plugin
	bool duplicate = false;         // another installed map uses the same file name
};

struct ZipEntry
{
	std::string name;
	std::filesystem::path file;
};

class WorkshopMaps : public BakkesMod::Plugin::BakkesModPlugin,
                     public BakkesMod::Plugin::PluginWindow,
                     public BakkesMod::Plugin::PluginSettingsWindow
{
public:
	void onLoad() override;

	// Plugin window (F2 > Plugins > Workshop Maps > Open, or bind "togglemenu workshopmaps2")
	void Render() override;
	std::string GetMenuName() override { return "workshopmaps2"; }
	std::string GetMenuTitle() override { return "Workshop Maps 2.0"; }
	void SetImGuiContext(uintptr_t ctx) override;
	bool ShouldBlockInput() override;
	bool IsActiveOverlay() override { return true; }
	void OnOpen() override;
	void OnClose() override { windowOpen_ = false; }

	// Tab in the F2 settings menu
	void RenderSettings() override;
	std::string GetPluginName() override { return "Workshop Maps 2.0"; }

private:
	std::filesystem::path MapsDir();
	void RefreshMaps();
	void RefreshZips();
	void InstallFromInput(std::string input);
	void FetchPage(std::string id, int attempt);
	void DownloadZip(std::string id, std::string title, std::string url);
	bool InstallZip(const std::filesystem::path& zip, const std::string& folderName, std::string& error);
	void LoadMap(const MapEntry& map);
	void EnsureAutoload();
	std::filesystem::path BackupDir();
	bool MoveToBackup(const std::filesystem::path& what);
	int CleanDuplicates();
	void RenderFriendsTab(const std::vector<MapEntry>& maps, bool busy);
	void HostGame(const MapEntry& map);
	void JoinGame(const std::string& code);
	void FindPublicIp();
	void TryOpenPort();
	void SpawnBall();
	void RenderBody();
	void CheckForUpdate();
	void InstallUpdate();
	void SetStatus(const std::string& text, bool isError = false);

	bool windowOpen_ = false;
	std::string inputBuf_;
	std::string dirBuf_;
	std::string filterBuf_;
	int confirmDelete_ = -1;
	int hostMap_ = 0;
	int hostMode_ = 0;
	int hostPlayers_ = 8;
	bool freeplayMode_ = false;
	std::atomic<int> latestBuild_{ 0 };
	std::atomic<bool> updating_{ false };
	std::atomic<bool> updated_{ false };
	std::string joinBuf_;
	std::string shareIp_;              // guarded by mutex_ when written off the UI thread

	std::mutex mutex_;                 // guards everything below
	std::vector<MapEntry> maps_;
	std::vector<ZipEntry> zips_;
	std::string status_;
	bool statusIsError_ = false;

	std::atomic<bool> busy_{ false };
	std::atomic<float> progress_{ 0.f };
};
