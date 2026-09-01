// This is the source code of AyuGram for Desktop.
//
// We do not and cannot prevent the use of our code,
// but be respectful and credit the original author.
//
// exteraGram-compatible plugin system, ported with permission of the idea.
// Copyright @Radolyn, 2026 (base), plugin engine extensions 2026
#include "ayu/plugins/plugin_engine.h"

#include "ayu/plugins/plugin_hook_router.h"
#include "ayu/plugins/plugin_manager.h"
#include "ayu/plugins/plugin_python_modules.h"
#include "logs.h"

#include <QDir>
#include <QFile>

#include <map>
#include <vector>

#ifdef AYU_WITH_PYTHON
#define PY_SSIZE_T_CLEAN
#include <Python.h>

extern "C" PyObject *PyInit__ayu_host();
#endif

namespace AyuPlugins {

// Defined in plugin_python_host.cpp.
void markMenuCallbacksAlive();
void releaseMenuCallbacks();
bool invokeMenuCallback(
	quint64 token,
	const std::map<QString, QString> &context);

#ifdef AYU_WITH_PYTHON

class PluginEngine::Impl {
public:
	struct Loaded {
		QString id;
		QString name;
		PyObject *instance = nullptr;
	};

	std::vector<Loaded> loaded;

	~Impl() {
		for (auto &entry : loaded) {
			Py_XDECREF(entry.instance);
		}
	}

};

namespace {

QString pythonErrorString() {
	PyObject *type = nullptr;
	PyObject *value = nullptr;
	PyObject *traceback = nullptr;
	PyErr_Fetch(&type, &value, &traceback);
	if (!type && !value) {
		return u"unknown python error"_q;
	}
	PyErr_NormalizeException(&type, &value, &traceback);
	auto result = u"python error"_q;
	const auto describe = [&](PyObject *object) {
		if (!object) {
			return;
		}
		const auto repr = PyObject_Repr(object);
		if (!repr) {
			PyErr_Clear();
			return;
		}
		const auto text = PyUnicode_AsUTF8(repr);
		if (text) {
			result = QString::fromUtf8(text);
		}
		Py_DECREF(repr);
	};
	describe(value ? value : type);
	Py_XDECREF(type);
	Py_XDECREF(value);
	Py_XDECREF(traceback);
	return result;
}

void callHook(PyObject *instance, const char *method) {
	if (!instance) {
		return;
	}
	const auto callback = PyObject_GetAttrString(instance, method);
	if (!callback) {
		PyErr_Clear();
		return;
	}
	if (!PyCallable_Check(callback)) {
		Py_DECREF(callback);
		return;
	}
	const auto result = PyObject_CallObject(callback, nullptr);
	Py_DECREF(callback);
	if (!result) {
		LOG(("Plugins engine: %1 failed: %2")
			.arg(QString::fromUtf8(method), pythonErrorString()));
	} else {
		Py_DECREF(result);
	}
}

bool loadPlugin(
		PluginEngine::Impl *impl,
		const InstalledPlugin &plugin,
		const QString &path) {
	QFile file(path);
	if (!file.open(QIODevice::ReadOnly)) {
		LOG(("Plugins engine: can't open '%1'").arg(path));
		return false;
	}
	const auto source = file.readAll();
	file.close();
	if (source.isEmpty() || source.size() > 4 * 1024 * 1024) {
		LOG(("Plugins engine: '%1' has invalid size").arg(plugin.id));
		return false;
	}

	auto globals = PyDict_New();
	auto name = PyUnicode_FromFormat(
		"ayu_plugin_%s",
		plugin.id.toStdString().c_str());
	PyDict_SetItemString(globals, "__name__", name);
	Py_DECREF(name);
	auto fileAttr = PyUnicode_FromString(path.toStdString().c_str());
	PyDict_SetItemString(globals, "__file__", fileAttr);
	Py_DECREF(fileAttr);
	PyDict_SetItemString(globals, "__builtins__", PyEval_GetBuiltins());

	const auto executed = PyRun_String(
		source.constData(),
		Py_file_input,
		globals,
		globals);
	if (!executed) {
		LOG(("Plugins engine: '%1' failed to load: %2")
			.arg(plugin.id, pythonErrorString()));
		Py_DECREF(globals);
		return false;
	}
	Py_DECREF(executed);

	const auto baseModule = PyImport_ImportModule("base_plugin");
	if (!baseModule) {
		LOG(("Plugins engine: base_plugin import failed: %1")
			.arg(pythonErrorString()));
		Py_DECREF(globals);
		return false;
	}
	const auto baseClass = PyObject_GetAttrString(baseModule, "BasePlugin");
	Py_DECREF(baseModule);
	if (!baseClass) {
		LOG(("Plugins engine: BasePlugin not found"));
		Py_DECREF(globals);
		return false;
	}

	PyObject *found = nullptr;
	PyObject *key = nullptr;
	PyObject *value = nullptr;
	auto pos = Py_ssize_t(0);
	while (PyDict_Next(globals, &pos, &key, &value)) {
		if (value == baseClass || !PyType_Check(value)) {
			continue;
		}
		const auto subclass = PyObject_IsSubclass(value, baseClass);
		if (subclass == 1) {
			found = value;
			break;
		}
		if (subclass < 0) {
			PyErr_Clear();
		}
	}
	if (!found) {
		LOG(("Plugins engine: '%1' defines no BasePlugin subclass")
			.arg(plugin.id));
		Py_DECREF(baseClass);
		Py_DECREF(globals);
		return false;
	}

	const auto instance = PyObject_CallObject(found, nullptr);
	Py_DECREF(baseClass);
	Py_DECREF(globals);
	if (!instance) {
		LOG(("Plugins engine: '%1' failed to instantiate: %2")
			.arg(plugin.id, pythonErrorString()));
		return false;
	}

	const auto idAttr = PyUnicode_FromString(plugin.id.toStdString().c_str());
	PyObject_SetAttrString(instance, "_plugin_id", idAttr);
	Py_DECREF(idAttr);
	const auto fileAttr2 = PyUnicode_FromString(path.toStdString().c_str());
	PyObject_SetAttrString(instance, "_plugin_file", fileAttr2);
	Py_DECREF(fileAttr2);

	callHook(instance, "on_plugin_load");

	impl->loaded.push_back({ plugin.id, plugin.name, instance });
	LOG(("Plugins engine: loaded '%1' (%2)")
		.arg(plugin.id, plugin.name));
	return true;
}

} // namespace

#endif // AYU_WITH_PYTHON

PluginEngine &PluginEngine::instance() {
	static PluginEngine result;
	return result;
}

PluginEngine::PluginEngine() = default;

PluginEngine::~PluginEngine() {
	shutdown();
}

void PluginEngine::initialize() {
	if (_initialized) {
		return;
	}

	const auto &manager = PluginManager::instance();
	auto enabled = std::vector<InstalledPlugin>();
	for (const auto &plugin : manager.plugins()) {
		if (plugin.enabled) {
			enabled.push_back(plugin);
		}
	}
	if (enabled.empty()) {
		return;
	}

#ifdef AYU_WITH_PYTHON
	if (PyImport_AppendInittab("_ayu_host", PyInit__ayu_host) != 0) {
		LOG(("Plugins engine: can't register _ayu_host module"));
		return;
	}
	Py_InitializeEx(0);
	markMenuCallbacksAlive();

	HookRouter::instance().setContextInvoker(
		&invokeMenuCallback);

	_impl = new Impl();

	{
		const auto dict = PyDict_New();
		const auto name = PyUnicode_FromString("ayu_plugin_bootstrap");
		PyDict_SetItemString(dict, "__name__", name);
		Py_DECREF(name);
		PyDict_SetItemString(dict, "__builtins__", PyEval_GetBuiltins());
		const auto result = PyRun_String(
			kPluginPythonBootstrap,
			Py_file_input,
			dict,
			dict);
		if (!result) {
			LOG(("Plugins engine: bootstrap failed: %1")
				.arg(pythonErrorString()));
		} else {
			Py_DECREF(result);
		}
		Py_DECREF(dict);
	}

	auto loadedCount = 0;
	for (const auto &plugin : enabled) {
		const auto path = QDir(manager.pluginsDir()).filePath(
			plugin.fileName);
		if (loadPlugin(_impl, plugin, path)) {
			++loadedCount;
		}
	}

	// exteraGram sends the START app event once every plugin is loaded.
	for (const auto &entry : _impl->loaded) {
		const auto result = PyObject_CallMethod(
			entry.instance,
			"on_app_event",
			"i",
			0); // AppEvent.START
		if (!result) {
			LOG(("Plugins engine: on_app_event of '%1' failed: %2")
				.arg(entry.id, pythonErrorString()));
		} else {
			Py_DECREF(result);
		}
	}
	LOG(("Plugins engine: %1/%2 plugin(s) loaded")
		.arg(loadedCount)
		.arg(enabled.size()));
#endif

	_initialized = true;

#ifndef AYU_WITH_PYTHON
	for (const auto &plugin : enabled) {
		LOG(("Plugins engine: '%1' (%2) is installed, "
			"but the Python runtime is not enabled in this build. "
			"Rebuild with -D AYU_WITH_PYTHON=ON to run plugins.")
			.arg(plugin.id, plugin.name));
	}
#endif
}

void PluginEngine::shutdown() {
	if (!_initialized) {
		return;
	}
	_initialized = false;

#ifdef AYU_WITH_PYTHON
	if (_impl) {
		for (const auto &entry : _impl->loaded) {
			callHook(entry.instance, "on_plugin_unload");
			HookRouter::instance().clearPlugin(entry.id);
		}
		delete _impl;
		_impl = nullptr;
	}
	if (Py_IsInitialized()) {
		releaseMenuCallbacks();
		Py_FinalizeEx();
	}
#endif
	LOG(("Plugins engine: shutdown"));
}

QString PluginEngine::sendMessageHook(const QString &text, bool &cancelled) {
	cancelled = false;
#ifdef AYU_WITH_PYTHON
	if (!_impl || _impl->loaded.empty()) {
		return text;
	}
	auto current = text;
	for (const auto &entry : _impl->loaded) {
		const auto hook = PyObject_GetAttrString(
			entry.instance,
			"on_send_message_hook");
		if (!hook) {
			PyErr_Clear();
			continue;
		}
		if (!PyCallable_Check(hook)) {
			Py_DECREF(hook);
			continue;
		}
		const auto arg = PyUnicode_FromString(current.toStdString().c_str());
		const auto args = PyTuple_Pack(1, arg);
		Py_DECREF(arg);
		const auto result = PyObject_CallObject(hook, args);
		Py_DECREF(args);
		Py_DECREF(hook);
		if (!result) {
			LOG(("Plugins engine: on_send_message_hook of '%1' failed: %2")
				.arg(entry.id, pythonErrorString()));
			continue;
		}
		if (result != Py_None) {
			const auto strategy = PyObject_GetAttrString(
				result,
				"strategy");
			auto strategyText = QString();
			if (strategy) {
				const auto repr = PyObject_Str(strategy);
				if (repr) {
					strategyText = QString::fromUtf8(
						PyUnicode_AsUTF8(repr));
					Py_DECREF(repr);
				}
				Py_DECREF(strategy);
			}
			if (strategyText == u"cancel"_q) {
				cancelled = true;
				Py_DECREF(result);
				return QString();
			}
			if (strategyText == u"modify"_q) {
				const auto message = PyObject_GetAttrString(
					result,
					"message");
				if (message) {
					const auto repr = PyObject_Str(message);
					if (repr) {
						current = QString::fromUtf8(
							PyUnicode_AsUTF8(repr));
						Py_DECREF(repr);
					}
					Py_DECREF(message);
				}
			}
		}
		Py_DECREF(result);
	}
	return current;
#else
	return text;
#endif
}

bool PluginEngine::isInitialized() const {
	return _initialized;
}

} // namespace AyuPlugins
