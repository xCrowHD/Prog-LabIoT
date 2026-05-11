"""
utils/sensors.py
Hardware-specific utility functions for sensor data conversion.
"""


def adc_to_klux(adc_value: float) -> float:
    """
    Convert a raw ADC reading from a KY-018 LDR module to kilo-lux.

    Assumes:
      - 10 kΩ fixed resistor in the voltage divider
      - 3.3 V supply voltage (ESP8266)
      - 10-bit ADC (0–1023)

    Returns a rounded kLux value (float).
    """
    if adc_value <= 0:
        return 0.0
    if adc_value >= 1023:
        return 1000.0  # Clamp at upper limit to avoid division errors

    v_out = (adc_value * 3.3) / 1024.0
    r_ldr = (10_000.0 * (3.3 - v_out)) / v_out       # Ω
    lux   = 500 / (r_ldr / 1_000.0)                   # kΩ → Lux via empirical curve

    return round(lux / 1000, 2)