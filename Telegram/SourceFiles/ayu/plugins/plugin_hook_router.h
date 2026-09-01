// This is the source code of AyuGram for Desktop.
//
// We do not and cannot prevent the use of our code,
// but be respectful and credit the original author.
//
// exteraGram-compatible plugin system, ported with permission of the idea.
// Copyright @Radolyn, 2026 (base), plugin engine extensions 2026
#pragma once

#include <QString>
#include <QStringList>

#include <functional>
#include <map>
#include <vector>

namespace AyuPlugins {

// Routes exteraGram (Android) plugin hook registrations to their desktop
// equivalents. A plugin calls add_menu_item()/add_hook() with Android-ish
// parameters; the router records the registration and the C++ side later
// triggers it from the matching desktop location, passing a context dict.
class HookRouter {
public:
	static HookRouter &instance();

	struct MenuItem {
		QString pluginId;
		QString id; // stable id, if the plugin passed one
		int menuType = 0; // exteraGram MenuItemType
		QString text;
		QString subtext;
		QString icon;
		int priority = 0;
		quint64 registration = 0; // python callback token
	};

	// exteraGram MenuItemType values (see base_plugin bootstrap).
	static constexpr int kMessageContextMenu = 0;
	static constexpr int kProfileActionMenu = 1;
	static constexpr int kChatContextMenu = 2;
	// DRAWER_MENU / MAIN_MENU / CHAT_ACTION_MENU have no desktop analog yet.

	// Registers a menu item. Called from the Python bridge. Returns a
	// handle used to unregister it later.
	quint64 addMenuItem(const MenuItem &item);
	void removeMenuItem(quint64 handle);

	[[nodiscard]] const std::vector<MenuItem> &menuItems() const;

	// Message context menu entries for the given plugin, sorted by
	// priority (higher first), ready to be appended to a Ui::PopupMenu.
	[[nodiscard]] std::vector<MenuItem> messageMenuItems() const;

	// Called by the bridge when a plugin is being unloaded: drop
	// everything registered by it.
	void clearPlugin(const QString &pluginId);

	// Fires a registered python callback (token) with a context dict.
	// Implemented in plugin_engine.cpp, where the interpreter lives.
	using ContextBuilder = std::function<bool(
		quint64 token,
		const std::map<QString, QString> &context)>;

	void setContextInvoker(ContextBuilder invoker);
	bool invoke(quint64 token, const std::map<QString, QString> &context);

private:
	HookRouter() = default;

	std::vector<MenuItem> _menuItems;
	quint64 _nextHandle = 1;
	ContextBuilder _invoker;

};

} // namespace AyuPlugins
