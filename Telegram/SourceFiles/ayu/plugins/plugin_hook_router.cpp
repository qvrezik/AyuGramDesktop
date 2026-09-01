// This is the source code of AyuGram for Desktop.
//
// We do not and cannot prevent the use of our code,
// but be respectful and credit the original author.
//
// exteraGram-compatible plugin system, ported with permission of the idea.
// Copyright @Radolyn, 2026 (base), plugin engine extensions 2026
#include "ayu/plugins/plugin_hook_router.h"

#include "logs.h"

namespace AyuPlugins {

HookRouter &HookRouter::instance() {
	static HookRouter result;
	return result;
}

quint64 HookRouter::addMenuItem(const MenuItem &item) {
	const auto handle = _nextHandle++;
	_menuItems.push_back(item);
	_menuItems.back().registration = handle;
	LOG(("Plugins engine: plugin '%1' added menu item '%2' (type %3)")
		.arg(item.pluginId, item.text)
		.arg(item.menuType));
	return handle;
}

void HookRouter::removeMenuItem(quint64 handle) {
	for (auto i = 0; i < _menuItems.size(); ++i) {
		if (_menuItems[i].registration == handle) {
			_menuItems.erase(_menuItems.begin() + i);
			return;
		}
	}
}

const std::vector<HookRouter::MenuItem> &HookRouter::menuItems() const {
	return _menuItems;
}

std::vector<HookRouter::MenuItem> HookRouter::messageMenuItems() const {
	auto result = std::vector<MenuItem>();
	for (const auto &item : _menuItems) {
		if (item.menuType == kMessageContextMenu) {
			result.push_back(item);
		}
	}
	std::sort(
		result.begin(),
		result.end(),
		[](const MenuItem &a, const MenuItem &b) {
			return a.priority > b.priority;
		});
	return result;
}

void HookRouter::clearPlugin(const QString &pluginId) {
	_menuItems.erase(
		std::remove_if(
			_menuItems.begin(),
			_menuItems.end(),
			[&](const MenuItem &item) {
				return item.pluginId == pluginId;
			}),
		_menuItems.end());
}

void HookRouter::setContextInvoker(ContextBuilder invoker) {
	_invoker = std::move(invoker);
}

bool HookRouter::invoke(
		quint64 token,
		const std::map<QString, QString> &context) {
	if (!_invoker) {
		return false;
	}
	return _invoker(token, context);
}

} // namespace AyuPlugins
