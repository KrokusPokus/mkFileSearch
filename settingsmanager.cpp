#include "settingsmanager.h"
#include <QFile>

SettingsManager::SettingsManager() {
	load(); // Lädt beim Erstellen automatisch
}

void SettingsManager::load() {
	QString settingsFilePath = getSettingsPath();
	QSettings s(settingsFilePath, QSettings::IniFormat);

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



	// Initiales Speichern, falls Datei nicht existiert (dein Default-Writer)
	if (!QFile::exists(settingsFilePath)) {
		save();
	}
}

void SettingsManager::save() {
	QString settingsFilePath = QFile::exists("settings.ini") ? "settings.ini" : getSettingsPath();
	QSettings s(settingsFilePath, QSettings::IniFormat);

	s.setValue("Core/UseSearchWorker", useSearchWorker);

	s.setValue("Extensions/FileExtAudio", DEFAULT_AUDIO);
	s.setValue("Extensions/FileExtImage", DEFAULT_IMAGE);
	s.setValue("Extensions/FileExtText", DEFAULT_TEXT);
	s.setValue("Extensions/FileExtVideo", DEFAULT_VIDEO);

    s.setValue("Handlers/AudioEditor", audioEditor);
    s.setValue("Handlers/ImageEditor", imageEditor);
    s.setValue("Handlers/TextEditor", textEditor);
    s.setValue("Handlers/VideoEditor", videoEditor);
    s.setValue("Handlers/FileManager", fileManager);
    s.setValue("Handlers/PropertiesDialog", propertiesDialog);

	s.setValue("Interface/AlternatingRowColors", alternatingRowColors);
	s.setValue("Interface/FontNameOverride", fontNameOverride);
	s.setValue("Interface/FontSizeOverride", fontSizeOverride);
	s.setValue("Interface/ShowGrid", showGrid);
	s.setValue("Interface/ShowPlaceholderText", showPlaceholderText);

	s.sync();
}

QSet<QString> SettingsManager::parseExtensions(const QString &input) {
	QSet<QString> set;
	const QStringList list = input.split(u',', Qt::SkipEmptyParts);
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
		// Sucht den Standard-Ordner für Konfigurationsdateien (AppConfigLocation)
		QString configDir = QStandardPaths::writableLocation(QStandardPaths::AppConfigLocation);

		// Sicherstellen, dass der Ordner existiert (erstellt ihn falls nötig)
		QDir().mkpath(configDir);

		settingsFilePath = configDir + "/settings.ini";
	}
	return settingsFilePath;
}
