// This is the source code of AyuGram for Desktop.
//
// We do not and cannot prevent the use of our code,
// but be respectful and credit the original author.
//
// exteraGram-compatible plugin system, ported with permission of the idea.
// Copyright @Radolyn, 2026 (base), plugin engine extensions 2026
#include "ayu/plugins/plugin_engine.h"

#include "ayu/plugins/plugin_manager.h"
#include "logs.h"

namespace AyuPlugins {

PluginEngine &PluginEngine::instance() {
	static PluginEngine result;
	return result;
}

void PluginEngine::initialize() {
	if (_initialized) {
		return;
	}
	_initialized = true;

	const auto &manager = PluginManager::instance();
	auto enabled = 0;
	for (const auto &plugin : manager.plugins()) {
		if (!plugin.enabled) {
			continue;
		}
		++enabled;
#ifndef AYU_WITH_PYTHON
		LOG(("Plugins engine: '%1' (%2) is installed, "
			"but the Python runtime is not enabled in this build. "
			"Rebuild with -D AYU_WITH_PYTHON=ON to run plugins.")
			.arg(plugin.id, plugin.name));
#else
		// The CPython runtime is wired up in a follow-up commit.
		LOG(("Plugins engine: loading '%1' (%2)...")
			.arg(plugin.id, plugin.name));
#endif
	}
	if (enabled > 0) {
		LOG(("Plugins engine: %1 plugin(s) enabled at startup")
			.arg(enabled));
	}
}

void PluginEngine::shutdown() {
	if (!_initialized) {
		return;
	}
	_initialized = false;
	LOG(("Plugins engine: shutdown"));
}

bool PluginEngine::isInitialized() const {
	return _initialized;
}

} // namespace AyuPlugins
