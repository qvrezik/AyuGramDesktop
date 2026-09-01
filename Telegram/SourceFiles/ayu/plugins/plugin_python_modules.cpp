// This is the source code of AyuGram for Desktop.
//
// We do not and cannot prevent the use of our code,
// but be respectful and credit the original author.
//
// exteraGram-compatible plugin system, ported with permission of the idea.
// Copyright @Radolyn, 2026 (base), plugin engine extensions 2026
#include "ayu/plugins/plugin_python_modules.h"

namespace AyuPlugins {

const char kPluginPythonBootstrap[] = R"PYBOOTSTRAP(
import json
import sys
import types

try:
    import _ayu_host as _host
except Exception:
    _host = None


class AppEvent:
    START = 0
    STOP = 1
    PAUSE = 2
    RESUME = 3


class HookStrategy:
    SKIP = "skip"
    MODIFY = "modify"
    CANCEL = "cancel"


class HookResult:
    """Result of a send-message hook.

    On AyuGram Desktop only SKIP (send as is), MODIFY (replace the text)
    and CANCEL (do not send) are honored.
    """

    def __init__(self, strategy=HookStrategy.SKIP, message=None, entities=None):
        self.strategy = strategy
        self.message = message
        self.entities = entities


class MenuItemType:
    MESSAGE_CONTEXT_MENU = 0
    PROFILE_ACTION_MENU = 1
    CHAT_CONTEXT_MENU = 2


class MenuItemData:
    def __init__(self, **kwargs):
        self.__dict__.update(kwargs)


class BasePlugin:
    """Base class every exteraGram plugin inherits from.

    Lifecycle hooks that plugins override:
      on_plugin_load(self)
      on_plugin_unload(self)
      on_app_event(self, event_type)
      on_send_message_hook(self, message, *args, **kwargs)
    """

    def __init__(self):
        self._plugin_id = ""
        self._plugin_file = ""
        self._running = True

    # --- lifecycle ---------------------------------------------------------

    def on_plugin_load(self):
        pass

    def on_plugin_unload(self):
        pass

    def on_app_event(self, event_type):
        pass

    def on_send_message_hook(self, message, *args, **kwargs):
        return None

    # --- misc --------------------------------------------------------------

    def is_running(self):
        return self._running

    def log(self, *args):
        text = " ".join(str(a) for a in args)
        if _host is not None:
            _host.log(self._plugin_id, text)
        else:
            print("[plugin:%s] %s" % (self._plugin_id, text))

    # --- settings storage --------------------------------------------------

    def create_settings(self):
        return []

    def get_setting(self, key, default=None):
        if _host is None:
            return default
        raw = _host.get_setting(self._plugin_id, key)
        if raw is None or raw == "":
            return default
        try:
            value = json.loads(raw)
        except ValueError:
            return default
        return default if value is None else value

    def set_setting(self, key, value, reload_settings=False):
        if _host is not None:
            _host.set_setting(self._plugin_id, key, json.dumps(value))

    def get_all_settings(self):
        if _host is None:
            return {}
        raw = _host.get_all_settings(self._plugin_id)
        try:
            data = json.loads(raw)
        except ValueError:
            return {}
        return data if isinstance(data, dict) else {}

    def export_settings(self):
        return self.get_all_settings()

    def import_settings(self, settings, reload_settings=True):
        if not isinstance(settings, dict):
            return
        if _host is not None:
            _host.set_all_settings(self._plugin_id, json.dumps(settings))

    # --- desktop-unsupported API (kept for import compatibility) -----------

    def _unsupported(self, name):
        self.log("%s() is not supported on AyuGram Desktop yet" % name)

    def add_menu_item(self, item=None, **kwargs):
        """Register a menu entry. MESSAGE_CONTEXT_MENU and
        PROFILE_ACTION_MENU items are routed to their desktop analogs;
        other menu types are ignored with a log entry."""
        if item is not None:
            data = item
        else:
            data = kwargs
        if data is None:
            return None
        menu_type = getattr(data, "menu_type", None)
        if menu_type is None:
            menu_type = kwargs.get("menu_type", 0)
        text = getattr(data, "text", None) or kwargs.get("text", "")
        if not text:
            return None
        if _host is not None and hasattr(_host, "add_menu_item"):
            handle = _host.add_menu_item(
                self._plugin_id,
                int(menu_type),
                str(text),
                str(getattr(data, "subtext", "") or ""),
                str(getattr(data, "icon", "") or ""),
                int(getattr(data, "priority", 0) or 0),
            )
            if handle:
                callback = getattr(data, "on_click", None)
                if callback is None:
                    callback = kwargs.get("on_click")
                if callback is not None and hasattr(_host, "bind_menu_callback"):
                    _host.bind_menu_callback(handle, callback)
                return handle
        return None

    def remove_menu_item(self, handle):
        if handle and _host is not None and hasattr(_host, "remove_menu_item"):
            _host.remove_menu_item(int(handle))

    def add_hook(self, *args, **kwargs):
        self._unsupported("add_hook")
        return None

    def remove_hook(self, *args, **kwargs):
        pass

    def add_on_send_message_hook(self, *args, **kwargs):
        self._unsupported("add_on_send_message_hook")
        return None

    def add_file_hook(self, *args, **kwargs):
        self._unsupported("add_file_hook")
        return None

    def remove_file_hook(self, *args, **kwargs):
        pass

    def add_intent_hook(self, *args, **kwargs):
        self._unsupported("add_intent_hook")
        return None

    def remove_intent_hook(self, *args, **kwargs):
        pass

    def hook_class(self, *args, **kwargs):
        self._unsupported("hook_class")
        return None

    def hook_method(self, *args, **kwargs):
        self._unsupported("hook_method")
        return None

    def hook_all_methods(self, *args, **kwargs):
        self._unsupported("hook_all_methods")
        return None

    def hook_all_constructors(self, *args, **kwargs):
        self._unsupported("hook_all_constructors")
        return None

    def hook_instance(self, *args, **kwargs):
        self._unsupported("hook_instance")
        return None

    def create_method_hook(self, *args, **kwargs):
        self._unsupported("create_method_hook")
        return None

    def get_client(self, *args, **kwargs):
        self._unsupported("get_client")
        return None

    def build_temp(self, *args, **kwargs):
        self._unsupported("build_temp")
        return None


# --- ui.settings widget descriptors ----------------------------------------


class _Setting:
    def __init__(self, **kwargs):
        self.kind = type(self).__name__
        self.__dict__.update(kwargs)


class Header(_Setting):
    pass


class Divider(_Setting):
    pass


class Text(_Setting):
    pass


class Switch(_Setting):
    pass


class Input(_Setting):
    pass


class EditText(_Setting):
    pass


class Selector(_Setting):
    pass


class Custom(_Setting):
    pass


class SimpleSettingFactory:
    def __init__(self, *args, **kwargs):
        pass


# --- module registration ----------------------------------------------------


def _register_module(name, attrs):
    module = sys.modules.get(name)
    if module is None:
        module = types.ModuleType(name)
        module.__dict__.update(attrs)
        sys.modules[name] = module
    else:
        module.__dict__.update(attrs)
    return module


_register_module("base_plugin", {
    "__version__": "1.4.5-desktop",
    "BasePlugin": BasePlugin,
    "AppEvent": AppEvent,
    "HookResult": HookResult,
    "HookStrategy": HookStrategy,
    "MenuItemData": MenuItemData,
    "MenuItemType": MenuItemType,
})

_settings = _register_module("ui.settings", {
    "__version__": "1.4.5-desktop",
    "Header": Header,
    "Divider": Divider,
    "Text": Text,
    "Switch": Switch,
    "Input": Input,
    "EditText": EditText,
    "Selector": Selector,
    "Custom": Custom,
    "SimpleSettingFactory": SimpleSettingFactory,
})

_ui = _register_module("ui", {"__path__": [], "__version__": "1.4.5-desktop"})
_ui.settings = _settings


def _make_stub(name):
    module = types.ModuleType(name)

    def _missing(attr):
        raise NotImplementedError(
            "module '%s' is not available on AyuGram Desktop" % name)

    module.__dict__["__getattr__"] = _missing
    sys.modules[name] = module
    return module


for _name in (
    "android_utils",
    "client_utils",
    "file_utils",
    "hook_utils",
    "intents",
    "markdown_utils",
    "extera_utils",
    "elyx",
    "elyxcore",
):
    if _name not in sys.modules:
        _make_stub(_name)
)PYBOOTSTRAP";

} // namespace AyuPlugins
