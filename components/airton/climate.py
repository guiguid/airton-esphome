import esphome.codegen as cg
from esphome.components import climate, uart
import esphome.config_validation as cv
from esphome.types import ConfigType

CODEOWNERS = ["@guiguid"]
DEPENDENCIES = ["climate", "uart"]

CONF_AIRTON_ID = "airton_id"

airton_ns = cg.esphome_ns.namespace("airton")
AirtonClimate = airton_ns.class_(
    "AirtonClimate", uart.UARTDevice, climate.Climate, cg.Component
)

CONFIG_SCHEMA = cv.All(
    climate.climate_schema(AirtonClimate)
    .extend(uart.UART_DEVICE_SCHEMA)
    .extend(cv.COMPONENT_SCHEMA)
)

FINAL_VALIDATE_SCHEMA = uart.final_validate_device_schema(
    "airton", baud_rate=9600, require_rx=True, require_tx=True
)


async def to_code(config: ConfigType) -> None:
    var = await climate.new_climate(config)
    await cg.register_component(var, config)
    await uart.register_uart_device(var, config)
