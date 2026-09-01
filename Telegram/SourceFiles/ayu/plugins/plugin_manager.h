// This is the source code of AyuGram for Desktop.
//
// We do not and cannot prevent the use of our code,
// but be respectful and credit the original author.
//
// exteraGram-compatible plugin system, ported with permission of the idea.
// Copyright @Radolyn, 2026 (base), plugin engine extensions 2026
#pragma once

#include <QList>
#include <QString>

#include <optional>

namespace Ui {
struct PluginMetadata;
} // namespace Ui

namespace AyuPlugins {

struct InstalledPlugin {
	QString id;
	QString name;
	QString version;
	QString author;
	QString description;
	QString pack; // icon sticker pack short name
	int iconIndex = -1;
	QString appVersion;
	QString sdkVersion;
	QStringList requirements;
	bool enabled = true;
	QString fileName; // file name inside the plugins dir
};

class PluginManager {
public:
	static PluginManager &instance();

	// <working dir>/tdata/plugins
	[[nodiscard]] QString pluginsDir() const;

	[[nodiscard]] const QList<InstalledPlugin> &plugins() const;
	[[nodiscard]] std::optional<InstalledPlugin> byId(
		const QString &id) const;
	[[nodiscard]] bool isInstalled(const QString &id) const;

	// Copies the .plugin file into the plugins dir and registers it.
	// If a plugin with the same id exists, it is replaced (update).
	bool install(const QString &sourcePath, const Ui::PluginMetadata &metadata);

	bool uninstall(const QString &id);
	void setEnabled(const QString &id, bool enabled);

private:
	PluginManager();

	void load();
	void save() const;

	QList<InstalledPlugin> _plugins;

};

} // namespace AyuPlugins
