# esphome - Fral air conditioner IR component

External component for esphome allowing usage of IR transmitter to send control codes to Fral air conditioner.

This was tested only with Fral FSC 14.2 model, but might work with other models too. This air conditioner uses
the same protocol as Clima Butler, although it seems to have its own set of codes. Thanks to [work done in 
IRremoteESP8266 project](https://github.com/crankyoldgit/IRremoteESP8266/issues/1812) by benjy3egg & crankyoldgit
I was able to decode them and build esphome component.

Spreadsheet with decoded commands is available in ir-codes directory.

# Supported features

There were some concessions made to better fit data model used in esphome's climate components and due to me only
having one air conditioner model to work with.

- Setting mode - cooling, heating, drying
- Setting target temperature
- Setting fan mode - "night" or "sleep" mode is presented as "quiet" fan mode instead of its own preset
- Setting horizontal swing - only on and "slow" setting available
- No timer support, although settings were decoded

# Requirements

- ESP32 or ESP8266 with esphome
- IR transmitter - simple IR diode and resistor connected to GPIO pin is enough, although that setup will have short range
  and *requires* current limitng resistor in series. For ESP 8266 it should be around 275 ohms, do not do this without resistor, you may damage your board.

# Using in esphome

See `esphome-config.example.yaml` - adjust to you needs, GPIO pin might be different depending what board and IR transmitter you are using. Refer to
esphome's [remote_transmitter documentation](https://esphome.io/components/remote_transmitter.html) for more details.

# Final notes

- Thanks to [IRremoteESP8266 project](https://github.com/crankyoldgit/IRremoteESP8266) which was invaluable tool to decode and write this component.
- Also thanks to [esphome project](https://esphome.io).
- This code is provided "as is" - I do not take any repsonsibility for any damage that might occur when using it.

