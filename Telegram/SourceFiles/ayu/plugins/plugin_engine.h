// This is the source code of AyuGram for Desktop.
//
// We do not and cannot prevent the use of our code,
// but be respectful and credit the original author.
//
// exteraGram-compatible plugin system, ported with permission of the idea.
// Copyright @Radolyn, 2026 (base), plugin engine extensions 2026
#pragma once

#include <QString>

namespace AyuPlugins {

// Loads enabled plugins at startup and runs their lifecycle hooks.
// Compiled without AYU_WITH_PYTHON the engine is inert: plugins are
// installed and listed, but not executed.
class PluginEngine {
public:
	static PluginEngine &instance();

	void initialize();
	void shutdown();

	bool isInitialized() const;

private:
	bool _initialized = false;

};

} // namespace AyuPlugins
