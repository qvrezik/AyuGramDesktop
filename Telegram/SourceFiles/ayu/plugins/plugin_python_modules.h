// This is the source code of AyuGram for Desktop.
//
// We do not and cannot prevent the use of our code,
// but be respectful and credit the original author.
//
// exteraGram-compatible plugin system, ported with permission of the idea.
// Copyright @Radolyn, 2026 (base), plugin engine extensions 2026
#pragma once

namespace AyuPlugins {

// Python source executed once at engine startup. Registers the
// exteraGram-compatible `base_plugin` and `ui.settings` modules plus stubs
// for Android-only modules, so desktop-hostile plugins fail with clean
// errors instead of import crashes.
extern const char kPluginPythonBootstrap[];

} // namespace AyuPlugins
