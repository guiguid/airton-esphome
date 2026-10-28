import esphome.codegen as cg
from esphome.components import switch
import esphome.config_validation as cv
from esphome.const import CONF_DISPLAY, ENTITY_CATEGORY_CONFIG
from esphome.types import ConfigType

from .climate import CONF_AIRTON_ID, AirtonClimate, airton_ns

CODEOWNERS = ["@guiguid"]

CONF_IONIZER = "ionizer"
CONF_SLEEP = "sleep"
CONF_ECO = "eco"
CONF_HEALTH = "health"

AirtonSwitch = airton_ns.class_(
    "AirtonSwitch", switch.Switch, cg.Parented.template(AirtonClimate)
)
AirtonSetting = airton_ns.enum("AirtonSetting", is_class=True)


def _switch_schema() -> cv.Schema:
    return switch.switch_schema(
        AirtonSwitch,
        entity_category=ENTITY_CATEGORY_CONFIG,
        default_restore_mode="DISABLED",
    )


CONFIG_SCHEMA = cv.Schema(
    {
        cv.GenerateID(CONF_AIRTON_ID): cv.use_id(AirtonClimate),
        cv.Optional(CONF_DISPLAY): _switch_schema(),
        cv.Optional(CONF_IONIZER): _switch_schema(),
        cv.Optional(CONF_SLEEP): _switch_schema(),
        cv.Optional(CONF_ECO): _switch_schema(),
        cv.Optional(CONF_HEALTH): _switch_schema(),
    }
)


async def to_code(config: ConfigType) -> None:
    hub = await cg.get_variable(config[CONF_AIRTON_ID])
    for key, setting, setter in (
        (CONF_DISPLAY, AirtonSetting.AIRTON_SETTING_DISPLAY, hub.set_display_switch),
        (CONF_IONIZER, AirtonSetting.AIRTON_SETTING_IONIZER, hub.set_ionizer_switch),
        (CONF_SLEEP, AirtonSetting.AIRTON_SETTING_SLEEP, hub.set_sleep_switch),
        (CONF_ECO, AirtonSetting.AIRTON_SETTING_ECO, hub.set_eco_switch),
        (CONF_HEALTH, AirtonSetting.AIRTON_SETTING_HEALTH, hub.set_health_switch),
    ):
        if (conf := config.get(key)) is not None:
            var = await switch.new_switch(conf)
            await cg.register_parented(var, hub)
            cg.add(var.set_setting(setting))
            cg.add(setter(var))
