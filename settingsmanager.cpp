#include "settingsmanager.h"
#include <QFile>

SettingsManager::SettingsManager() {
	load(); // Lädt beim Erstellen automatisch
}

void SettingsManager::load() {
    QSettings s(getSettingsPath(), QSettings::IniFormat);

	// Core
	useSearchWorker = s.value("Core/UseSearchWorker", true).toBool();

	// Extensions
	audioExts = parseExtensions(s.value("Extensions/FileExtAudio", DEFAULT_AUDIO).toString());
	imageExts = parseExtensions(s.value("Extensions/FileExtImage", DEFAULT_IMAGE).toString());
	textExts  = parseExtensions(s.value("Extensions/FileExtText",  DEFAULT_TEXT).toString());
	videoExts = parseExtensions(s.value("Extensions/FileExtVideo", DEFAULT_VIDEO).toString());

	// Handlers
    audioEditor      = QDir::toNativeSeparators(s.value("Handlers/AudioEditor", "kwave").toString());
    imageEditor      = QDir::toNativeSeparators(s.value("Handlers/ImageEditor", "gimp").toString());
    textEditor       = QDir::toNativeSeparators(s.value("Handlers/TextEditor", "kate").toString());
    videoEditor      = QDir::toNativeSeparators(s.value("Handlers/VideoEditor", "kdenlive").toString());
    fileManager      = QDir::toNativeSeparators(s.value("Handlers/FileManager", "dolphin").toString());
	propertiesDialog = QDir::toNativeSeparators(s.value("Handlers/PropertiesDialog").toString());

	// Interface
	alternatingRowColors = s.value("Interface/AlternatingRowColors", true).toBool();
	fontNameOverride     = s.value("Interface/FontNameOverride", "").toString();
	fontSizeOverride     = s.value("Interface/FontSizeOverride", 0).toInt();
	showGrid             = s.value("Interface/ShowGrid", false).toBool();
	showPlaceholderText  = s.value("Interface/ShowPlaceholderText", true).toBool();
    showIconsInMenu      = s.value("Interface/ShowIconsInMenu", true).toBool();
    showShortcutsInMenu  = s.value("Interface/ShowShortcutsInMenu", true).toBool();

    save();
}

void SettingsManager::save() {
    QSettings s(getSettingsPath(), QSettings::IniFormat);

    // Core
    safeSetValue(s, "Core/UseSearchWorker", useSearchWorker);

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
    safeSetValue(s, "Handlers/PropertiesDialog", propertiesDialog);

    // Interface
    safeSetValue(s, "Interface/AlternatingRowColors", alternatingRowColors);
    safeSetValue(s, "Interface/FontNameOverride", fontNameOverride);
    safeSetValue(s, "Interface/FontSizeOverride", fontSizeOverride);
    safeSetValue(s, "Interface/ShowGrid", showGrid);
    safeSetValue(s, "Interface/ShowPlaceholderText", showPlaceholderText);
    safeSetValue(s, "Interface/ShowIconsInMenu", showIconsInMenu);
    safeSetValue(s, "Interface/ShowShortcutsInMenu", showShortcutsInMenu);

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
	QString settingsFilePath;
	if (QFile::exists("settings.ini")) {
		settingsFilePath = "settings.ini";
	} else {
        // Looks up the default folder path for config files (AppConfigLocation)
		QString configDir = QStandardPaths::writableLocation(QStandardPaths::AppConfigLocation);

        // Make sure that path exists
		QDir().mkpath(configDir);

		settingsFilePath = configDir + "/settings.ini";
	}
	return settingsFilePath;
}
