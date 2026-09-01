// This is the source code of AyuGram for Desktop.
//
// We do not and cannot prevent the use of our code,
// but be respectful and credit the original author.
//
// exteraGram-compatible plugin system, ported with permission of the idea.
// Copyright @Radolyn, 2026 (base), plugin engine extensions 2026
#include "ayu/plugins/plugin_manager.h"

#include "ayu/libs/json.hpp"
#include "ayu/ui/boxes/plugin_info_box.h"
#include "logs.h"
#include "settings.h"

#include <QDir>
#include <QFile>
#include <QFileInfo>

using json = nlohmann::json;

namespace AyuPlugins {
namespace {

QString registryPath() {
	return cWorkingDir() + u"tdata/plugins/registry.json"_q;
}

InstalledPlugin fromJson(const json &j) {
	auto result = InstalledPlugin();
	result.id = QString::fromStdString(j.value("id", ""));
	result.name = QString::fromStdString(j.value("name", ""));
	result.version = QString::fromStdString(j.value("version", "1.0"));
	result.author = QString::fromStdString(j.value("author", ""));
	result.description = QString::fromStdString(j.value("description", ""));
	result.pack = QString::fromStdString(j.value("pack", ""));
	result.iconIndex = j.value("iconIndex", -1);
	result.appVersion = QString::fromStdString(j.value("appVersion", ""));
	result.sdkVersion = QString::fromStdString(j.value("sdkVersion", ""));
	result.enabled = j.value("enabled", true);
	result.fileName = QString::fromStdString(j.value("fileName", ""));
	if (const auto it = j.find("requirements"); it != j.end() && it->is_array()) {
		for (const auto &req : *it) {
			result.requirements.push_back(
				QString::fromStdString(req.get<std::string>()));
		}
	}
	return result;
}

json toJson(const InstalledPlugin &plugin) {
	auto j = json::object();
	j["id"] = plugin.id.toStdString();
	j["name"] = plugin.name.toStdString();
	j["version"] = plugin.version.toStdString();
	j["author"] = plugin.author.toStdString();
	j["description"] = plugin.description.toStdString();
	j["pack"] = plugin.pack.toStdString();
	j["iconIndex"] = plugin.iconIndex;
	j["appVersion"] = plugin.appVersion.toStdString();
	j["sdkVersion"] = plugin.sdkVersion.toStdString();
	j["enabled"] = plugin.enabled;
	j["fileName"] = plugin.fileName.toStdString();
	auto requirements = json::array();
	for (const auto &req : plugin.requirements) {
		requirements.push_back(req.toStdString());
	}
	j["requirements"] = std::move(requirements);
	return j;
}

} // namespace

PluginManager &PluginManager::instance() {
	static PluginManager result;
	return result;
}

PluginManager::PluginManager() {
	load();
}

QString PluginManager::pluginsDir() const {
	return cWorkingDir() + u"tdata/plugins"_q;
}

const QList<InstalledPlugin> &PluginManager::plugins() const {
	return _plugins;
}

std::optional<InstalledPlugin> PluginManager::byId(
		const QString &id) const {
	for (const auto &plugin : _plugins) {
		if (plugin.id == id) {
			return plugin;
		}
	}
	return std::nullopt;
}

bool PluginManager::isInstalled(const QString &id) const {
	return byId(id).has_value();
}

bool PluginManager::install(
		const QString &sourcePath,
		const Ui::PluginMetadata &metadata) {
	if (metadata.id.isEmpty() || metadata.name.isEmpty()) {
		return false;
	}

	const auto dir = QDir(pluginsDir());
	if (!dir.exists() && !dir.mkpath(u"."_q)) {
		return false;
	}

	const auto previous = byId(metadata.id);
	const auto fileName = metadata.id + u".plugin"_q;
	const auto destination = dir.filePath(fileName);

	if (QFileInfo(sourcePath).absoluteFilePath()
		!= QFileInfo(destination).absoluteFilePath()) {
		QFile::remove(destination);
		if (!QFile::copy(sourcePath, destination)) {
			return false;
		}
	}

	auto plugin = InstalledPlugin();
	plugin.id = metadata.id;
	plugin.name = metadata.name;
	plugin.version = metadata.version;
	plugin.author = metadata.author;
	plugin.description = metadata.description;
	plugin.pack = metadata.icon.split('/').value(0);
	plugin.iconIndex = metadata.icon.split('/').value(1).toInt();
	plugin.appVersion = metadata.minVersion;
	plugin.requirements = metadata.requirements;
	plugin.fileName = fileName;
	plugin.enabled = previous ? previous->enabled : true;

	if (previous) {
		for (auto i = 0; i < _plugins.size(); ++i) {
			if (_plugins[i].id == plugin.id) {
				_plugins.replace(i, plugin);
				break;
			}
		}
	} else {
		_plugins.push_back(std::move(plugin));
	}
	save();
	return true;
}

bool PluginManager::uninstall(const QString &id) {
	for (auto i = 0; i < _plugins.size(); ++i) {
		if (_plugins[i].id == id) {
			const auto fileName = _plugins[i].fileName;
			_plugins.removeAt(i);
			if (!fileName.isEmpty()) {
				QFile::remove(QDir(pluginsDir()).filePath(fileName));
			}
			save();
			return true;
		}
	}
	return false;
}

void PluginManager::setEnabled(const QString &id, bool enabled) {
	for (auto &plugin : _plugins) {
		if (plugin.id == id) {
			plugin.enabled = enabled;
			save();
			return;
		}
	}
}

void PluginManager::load() {
	_plugins.clear();

	QFile file(registryPath());
	if (!file.exists()) {
		return;
	}
	if (!file.open(QIODevice::ReadOnly)) {
		return;
	}
	auto error = QString();
	const auto content = file.readAll();
	file.close();

	try {
		const auto j = json::parse(content.toStdString());
		if (!j.is_array()) {
			return;
		}
		for (const auto &entry : j) {
			auto plugin = fromJson(entry);
			if (plugin.id.isEmpty() || plugin.fileName.isEmpty()) {
				continue;
			}
			// Drop registry entries whose files have vanished.
			if (!QFile::exists(QDir(pluginsDir()).filePath(plugin.fileName))) {
				continue;
			}
			_plugins.push_back(std::move(plugin));
		}
	} catch (const std::exception &e) {
		LOG(("Plugin registry parse error: %1")
			.arg(QString::fromStdString(e.what())));
	}
}

void PluginManager::save() const {
	QDir(pluginsDir()).mkpath(u"."_q);

	auto j = json::array();
	for (const auto &plugin : _plugins) {
		j.push_back(toJson(plugin));
	}

	QFile file(registryPath());
	if (file.open(QIODevice::WriteOnly)) {
		file.write(QByteArray::fromStdString(j.dump(4)));
		file.close();
	} else {
		LOG(("Could not write plugin registry: %1").arg(registryPath()));
	}
}

} // namespace AyuPlugins
