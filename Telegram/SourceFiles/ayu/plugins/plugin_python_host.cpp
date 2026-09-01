// This is the source code of AyuGram for Desktop.
//
// We do not and cannot prevent the use of our code,
// but be respectful and credit the original author.
//
// exteraGram-compatible plugin system, ported with permission of the idea.
// Copyright @Radolyn, 2026 (base), plugin engine extensions 2026
//
// _ayu_host: the C-implemented Python module exposed to plugins. Keeps the
// surface tiny on purpose — everything else is layered in Python
// (see plugin_python_modules.cpp).
#ifdef AYU_WITH_PYTHON

#include <Python.h>

#include "ayu/plugins/plugin_manager.h"
#include "ayu/libs/json.hpp"
#include "logs.h"
#include "settings.h"

#include <QDir>
#include <QFile>

using json = nlohmann::json;

namespace AyuPlugins {
namespace {

bool validId(const QString &id) {
	if (id.isEmpty() || id.size() > 64) {
		return false;
	}
	for (const auto ch : id) {
		const auto c = ch.unicode();
		const auto ok = (c >= 'a' && c <= 'z')
			|| (c >= 'A' && c <= 'Z')
			|| (c >= '0' && c <= '9')
			|| c == '_' || c == '-';
		if (!ok) {
			return false;
		}
	}
	return true;
}

QString settingsPath(const QString &id) {
	return QDir(cWorkingDir() + u"tdata/plugins"_q).filePath(
		id + u"/settings.json"_q);
}

json readSettings(const QString &id) {
	QFile file(settingsPath(id));
	if (!file.open(QIODevice::ReadOnly)) {
		return json::object();
	}
	try {
		const auto parsed = json::parse(file.readAll().toStdString());
		file.close();
		return parsed.is_object() ? parsed : json::object();
	} catch (...) {
		return json::object();
	}
}

void writeSettings(const QString &id, const json &data) {
	const auto dir = QFileInfo(settingsPath(id)).dir();
	if (!dir.exists() && !dir.mkpath(u"."_q)) {
		return;
	}
	QFile file(settingsPath(id));
	if (file.open(QIODevice::WriteOnly)) {
		file.write(QByteArray::fromStdString(data.dump(2)));
	}
}

PyObject *host_log(PyObject *, PyObject *args) {
	const char *id = nullptr;
	const char *message = nullptr;
	if (!PyArg_ParseTuple(args, "ss", &id, &message)) {
		return nullptr;
	}
	LOG(("[plugin:%1] %2").arg(
		QString::fromUtf8(id),
		QString::fromUtf8(message)));
	Py_RETURN_NONE;
}

PyObject *host_is_running(PyObject *, PyObject *args) {
	const char *id = nullptr;
	if (!PyArg_ParseTuple(args, "s", &id)) {
		return nullptr;
	}
	const auto plugin = PluginManager::instance().byId(
		QString::fromUtf8(id));
	return PyBool_FromLong(plugin && plugin->enabled ? 1 : 0);
}

PyObject *host_get_setting(PyObject *, PyObject *args) {
	const char *id = nullptr;
	const char *key = nullptr;
	if (!PyArg_ParseTuple(args, "ss", &id, &key)) {
		return nullptr;
	}
	const auto pluginId = QString::fromUtf8(id);
	if (!validId(pluginId)) {
		Py_RETURN_NONE;
	}
	const auto data = readSettings(pluginId);
	const auto it = data.find(key);
	if (it == data.end() || !it->is_string()) {
		Py_RETURN_NONE;
	}
	return PyUnicode_FromString(it->get<std::string>().c_str());
}

PyObject *host_set_setting(PyObject *, PyObject *args) {
	const char *id = nullptr;
	const char *key = nullptr;
	const char *value = nullptr;
	if (!PyArg_ParseTuple(args, "sss", &id, &key, &value)) {
		return nullptr;
	}
	const auto pluginId = QString::fromUtf8(id);
	if (!validId(pluginId)) {
		Py_RETURN_NONE;
	}
	auto data = readSettings(pluginId);
	data[key] = std::string(value);
	writeSettings(pluginId, data);
	Py_RETURN_NONE;
}

PyObject *host_get_all_settings(PyObject *, PyObject *args) {
	const char *id = nullptr;
	if (!PyArg_ParseTuple(args, "s", &id)) {
		return nullptr;
	}
	const auto pluginId = QString::fromUtf8(id);
	if (!validId(pluginId)) {
		return PyUnicode_FromString("{}");
	}
	return PyUnicode_FromString(
		readSettings(pluginId).dump().c_str());
}

PyObject *host_set_all_settings(PyObject *, PyObject *args) {
	const char *id = nullptr;
	const char *value = nullptr;
	if (!PyArg_ParseTuple(args, "ss", &id, &value)) {
		return nullptr;
	}
	const auto pluginId = QString::fromUtf8(id);
	if (!validId(pluginId)) {
		Py_RETURN_NONE;
	}
	try {
		const auto data = json::parse(std::string(value));
		if (data.is_object()) {
			writeSettings(pluginId, data);
		}
	} catch (...) {
	}
	Py_RETURN_NONE;
}

PyMethodDef hostMethods[] = {
	{"log", host_log, METH_VARARGS, "Write a message to the AyuGram log."},
	{"is_running", host_is_running, METH_VARARGS, "Whether the plugin is enabled."},
	{"get_setting", host_get_setting, METH_VARARGS, "Read a raw JSON setting value."},
	{"set_setting", host_set_setting, METH_VARARGS, "Write a raw JSON setting value."},
	{"get_all_settings", host_get_all_settings, METH_VARARGS, "Read all settings as JSON."},
	{"set_all_settings", host_set_all_settings, METH_VARARGS, "Replace all settings from JSON."},
	{nullptr, nullptr, 0, nullptr},
};

PyModuleDef hostModule = {
	PyModuleDef_HEAD_INIT,
	"_ayu_host",
	"AyuGram Desktop plugin host bridge.",
	-1,
	hostMethods,
	nullptr,
	nullptr,
	nullptr,
	nullptr,
};

} // namespace

PyObject *pyModuleInitHost() {
	return PyModule_Create(&hostModule);
}

} // namespace AyuPlugins

// The entry point name is derived by the interpreter import machinery.
PyMODINIT_FUNC PyInit__ayu_host() {
	return AyuPlugins::pyModuleInitHost();
}

#endif // AYU_WITH_PYTHON
