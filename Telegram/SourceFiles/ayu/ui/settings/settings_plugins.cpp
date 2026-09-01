// This is the source code of AyuGram for Desktop.
//
// We do not and cannot prevent the use of our code,
// but be respectful and credit the original author.
//
// Copyright @Radolyn, 2026
#include "ayu/ui/settings/settings_plugins.h"

#include "lang_auto.h"
#include "ayu/plugins/plugin_manager.h"
#include "ayu/ui/settings/ayu_builder.h"
#include "ayu/ui/settings/settings_main.h"
#include "core/file_utilities.h"
#include "settings/settings_builder.h"
#include "settings/settings_common.h"
#include "styles/style_menu_icons.h"
#include "styles/style_settings.h"
#include "ui/vertical_list.h"
#include "ui/widgets/buttons.h"
#include "ui/wrap/vertical_layout.h"
#include "window/window_session_controller.h"

namespace Settings {

using namespace Builder;
using namespace AyuBuilder;

namespace {

void BuildPluginsList(SectionBuilder &builder, AyuSectionBuilder &ayu) {
	builder.addSkip();
	builder.addSubsectionTitle(tr::ayu_PluginsSectionHeader());

	const auto &manager = AyuPlugins::PluginManager::instance();
	const auto plugins = manager.plugins();

	if (plugins.isEmpty()) {
		builder.addDividerText(tr::ayu_PluginsEmpty());
	} else {
		for (const auto &plugin : plugins) {
			const auto id = plugin.id;
			ayu.addToggle({
				.id = u"ayu/plugin/"_q + id,
				.title = rpl::single(plugin.name),
				.getter = [=] {
					const auto current = AyuPlugins::PluginManager::instance()
						.byId(id);
					return current && current->enabled;
				},
				.setter = [=](bool enabled) {
					AyuPlugins::PluginManager::instance().setEnabled(
						id,
						enabled);
				},
				.icon = { &st::menuIconStickers },
			});
		}
		builder.addDividerText(tr::ayu_PluginsManageHint());
	}
}

void BuildPluginsFolder(SectionBuilder &builder) {
	const auto controller = builder.controller();

	builder.addSkip();
	builder.addButton({
		.id = u"ayu/plugins/openFolder"_q,
		.title = tr::ayu_PluginsOpenFolder(),
		.icon = { &st::menuIconShowInFolder },
		.onClick = [=] {
			File::ShowInFolder(
				AyuPlugins::PluginManager::instance().pluginsDir());
		},
	});
	builder.addSkip();
}

const auto kMeta = BuildHelper({
	.id = AyuPluginsSection::Id(),
	.parentId = AyuMain::Id(),
	.title = &tr::ayu_CategoryPlugins,
	.icon = &st::menuIconStickers,
}, [](SectionBuilder &builder) {
	auto ayu = AyuSectionBuilder(builder);

	BuildPluginsList(builder, ayu);
	BuildPluginsFolder(builder);
});

} // namespace

rpl::producer<QString> AyuPluginsSection::title() {
	return tr::ayu_CategoryPlugins();
}

AyuPluginsSection::AyuPluginsSection(
	QWidget *parent,
	not_null<Window::SessionController*> controller)
: Section(parent, controller) {
	setupContent();
}

void AyuPluginsSection::setupContent() {
	const auto content = Ui::CreateChild<Ui::VerticalLayout>(this);
	build(content, kMeta.build);
	Ui::ResizeFitChild(this, content);
}

Type AyuPluginsSectionId() {
	return AyuPluginsSection::Id();
}

} // namespace Settings
