#include "settingsmanager.h"

#include <QCoreApplication>
#include <QFile>
#include <QProcessEnvironment>

SettingsManager::SettingsManager() {
	load(); // Lädt beim Erstellen automatisch
}

void SettingsManager::load() {
    getDefaults();

    QSettings s(getSettingsPath(), QSettings::IniFormat);

	// Extensions
	audioExts = parseExtensions(s.value("Extensions/FileExtAudio", DEFAULT_AUDIO).toString());
	imageExts = parseExtensions(s.value("Extensions/FileExtImage", DEFAULT_IMAGE).toString());
	textExts  = parseExtensions(s.value("Extensions/FileExtText",  DEFAULT_TEXT).toString());
	videoExts = parseExtensions(s.value("Extensions/FileExtVideo", DEFAULT_VIDEO).toString());

	// Handlers
    audioEditor      = QDir::toNativeSeparators(s.value("Handlers/AudioEditor", DEFAULT_AUDIO_EDITOR).toString());
    imageEditor      = QDir::toNativeSeparators(s.value("Handlers/ImageEditor", DEFAULT_IMAGE_EDITOR).toString());
    textEditor       = QDir::toNativeSeparators(s.value("Handlers/TextEditor", DEFAULT_TEXT_EDITOR).toString());
    videoEditor      = QDir::toNativeSeparators(s.value("Handlers/VideoEditor", DEFAULT_VIDEO_EDITOR).toString());
    fileManager      = QDir::toNativeSeparators(s.value("Handlers/FileManager", DEFAULT_FILE_MANAGER).toString());
    searchTool       = QDir::toNativeSeparators(s.value("Handlers/SearchTool", DEFAULT_SEARCH_TOOL).toString());
    renameTool       = QDir::toNativeSeparators(s.value("Handlers/RenameTool", DEFAULT_RENAME_TOOL).toString());

	// Interface
    alternatingRowColors = s.value("Interface/AlternatingRowColors", DEFAULT_ALTERNATING_ROW_COLORS).toBool();
    executableFilesRed   = s.value("Interface/ExecutableFilesRed", DEFAULT_EXECUTABLE_FILES_RED).toBool();
    fontNameOverride     = s.value("Interface/FontNameOverride", DEFAULT_FONT_NAME_OVERRIDE).toString();
    fontSizeOverride     = s.value("Interface/FontSizeOverride", DEFAULT_FONT_SIZE_OVERRIDE).toInt();
    menuOnMouseUp        = s.value("Interface/MenuOnMouseUp", DEFAULT_MENU_ON_MOUSE_UP).toBool();
    showFileExtensions   = s.value("Interface/ShowFileExtensions", DEFAULT_SHOW_FILE_EXTENSIONS).toBool();
    showGrid             = s.value("Interface/ShowGrid", DEFAULT_SHOW_GRID).toBool();
    showIconsInMenu      = s.value("Interface/ShowIconsInMenu", DEFAULT_SHOW_ICONS_IN_MENU).toBool();
    showPlaceholderText  = s.value("Interface/ShowPlaceholderText", DEFAULT_SHOW_PLACEHOLDER_TEXT).toBool();
    showShortcutsInMenu  = s.value("Interface/ShowShortcutsInMenu", DEFAULT_SHOW_SHORTCUTS_IN_MENU).toBool();

    saveSettings();
}

void SettingsManager::getDefaults() {
#ifdef Q_OS_LINUX
    DEFAULT_AUDIO_EDITOR = "org.kde.kwave.desktop";
    DEFAULT_IMAGE_EDITOR = "gimp.desktop";
    DEFAULT_TEXT_EDITOR = "org.kde.kate.desktop";
    DEFAULT_VIDEO_EDITOR = "org.kde.kdenlive.desktop";
    DEFAULT_FILE_MANAGER = "org.kde.dolphin.desktop";
    DEFAULT_SEARCH_TOOL = "org.kde.kfind.desktop";
    DEFAULT_RENAME_TOOL = "";

    DEFAULT_ALTERNATING_ROW_COLORS = true;
    DEFAULT_EXECUTABLE_FILES_RED = false;
    DEFAULT_FONT_NAME_OVERRIDE = "";
    DEFAULT_FONT_SIZE_OVERRIDE = 0;
    DEFAULT_MENU_ON_MOUSE_UP = false;
    DEFAULT_SHOW_FILE_EXTENSIONS = true;
    DEFAULT_SHOW_GRID = false;
    DEFAULT_SHOW_ICONS_IN_MENU = true;
    DEFAULT_SHOW_PLACEHOLDER_TEXT = true;
    DEFAULT_SHOW_SHORTCUTS_IN_MENU = true;
#elif defined(Q_OS_WIN)
    DEFAULT_AUDIO_EDITOR = "";
    DEFAULT_IMAGE_EDITOR = "";
    DEFAULT_TEXT_EDITOR = "";
    DEFAULT_VIDEO_EDITOR = "";
    DEFAULT_FILE_MANAGER = "mkFolderWidget";
    DEFAULT_SEARCH_TOOL = "";
    DEFAULT_RENAME_TOOL = "";

    DEFAULT_ALTERNATING_ROW_COLORS = false;
    DEFAULT_EXECUTABLE_FILES_RED = true;
    DEFAULT_FONT_NAME_OVERRIDE = "Tahoma";
    DEFAULT_FONT_SIZE_OVERRIDE = 8;
    DEFAULT_MENU_ON_MOUSE_UP = true;
    DEFAULT_SHOW_FILE_EXTENSIONS = false;
    DEFAULT_SHOW_GRID = false;
    DEFAULT_SHOW_ICONS_IN_MENU = false;
    DEFAULT_SHOW_PLACEHOLDER_TEXT = true;
    DEFAULT_SHOW_SHORTCUTS_IN_MENU = false;
#endif
}

void SettingsManager::saveSettings() {
    QSettings s(getSettingsPath(), QSettings::IniFormat);

    // Extensions
    safeSetValue(s, "Extensions/FileExtAudio", formatStringSet(audioExts));
    safeSetValue(s, "Extensions/FileExtImage", formatStringSet(imageExts));
    safeSetValue(s, "Extensions/FileExtText", formatStringSet(textExts));
    safeSetValue(s, "Extensions/FileExtVideo", formatStringSet(videoExts));

    // Handlers
    safeSetValue(s, "Handlers/AudioEditor", audioEditor);
    safeSetValue(s, "Handlers/ImageEditor", imageEditor);
    safeSetValue(s, "Handlers/TextEditor", textEditor);
    safeSetValue(s, "Handlers/VideoEditor", videoEditor);
    safeSetValue(s, "Handlers/FileManager", fileManager);
    safeSetValue(s, "Handlers/SearchTool", searchTool);
    safeSetValue(s, "Handlers/RenameTool", renameTool);

    // Interface
    safeSetValue(s, "Interface/AlternatingRowColors", alternatingRowColors);
    safeSetValue(s, "Interface/ExecutableFilesRed", executableFilesRed);
    safeSetValue(s, "Interface/FontNameOverride", fontNameOverride);
    safeSetValue(s, "Interface/FontSizeOverride", fontSizeOverride);
    safeSetValue(s, "Interface/MenuOnMouseUp", menuOnMouseUp);
    safeSetValue(s, "Interface/ShowGrid", showGrid);
    safeSetValue(s, "Interface/ShowPlaceholderText", showPlaceholderText);
    safeSetValue(s, "Interface/ShowIconsInMenu", showIconsInMenu);
    safeSetValue(s, "Interface/ShowShortcutsInMenu", showShortcutsInMenu);
    safeSetValue(s, "Interface/ShowFileExtensions", showFileExtensions);

    // Because we used safeSetValue(), the file will only get written if there were changes in values.
	s.sync();
}

void SettingsManager::safeSetValue(QSettings &settings, const QString &key, const QVariant &value) {
    if (settings.value(key) != value) {
        settings.setValue(key, value);
    }
}

QString SettingsManager::formatStringSet(const QSet<QString> &extensionSet) {
    QStringList stringList = extensionSet.values();
    stringList.sort(Qt::CaseInsensitive);
    return stringList.join(",");
    }

QSet<QString> SettingsManager::parseExtensions(const QString &input) {
    const QStringList list = input.split(u',', Qt::SkipEmptyParts);

	QSet<QString> set;
    set.reserve(list.size());

	for (const QString &s : list) {
		set.insert(s.trimmed().toLower());
	}

	return set;
}

QString SettingsManager::getSettingsPath() {
    QString iniFilePath = QDir(QCoreApplication::applicationDirPath()).filePath("/settings.ini");
    if (!QFile::exists(iniFilePath)) {
        QString toolsDirPath = QDir(QStandardPaths::writableLocation(QStandardPaths::GenericConfigLocation)).filePath("mkTools");
        if (!QDir(toolsDirPath).exists()) {
            QDir().mkpath(toolsDirPath);
        }
        QDir toolsDir(toolsDirPath);
        iniFilePath = toolsDir.filePath("settings.ini");
    }
    return iniFilePath;
}
