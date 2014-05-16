### NAME

Module for GPIO, SPI, I2C, MMIO, and Serial Linux peripheral I/O interfaces.

### SYNOPSIS

``` lua
local periphery = require('periphery')

-- Module Version
periphery.version   <string>

-- Submodules
periphery.GPIO
periphery.SPI
periphery.I2C
periphery.MMIO
periphery.Serial

-- Helper Functions
periphery.sleep(seconds <number>)
periphery.sleep_ms(milliseconds <number>)
periphery.sleep_us(microseconds <number>)
```

### DESCRIPTION

``` lua
Property periphery.version  immutable <string>
```
Version of periphery module as a string (e.g. "1.0.0").

--------------------------------------------------------------------------------

``` lua
periphery.GPIO
```
GPIO module. See [GPIO documentation](gpio.md) for more information.

--------------------------------------------------------------------------------

``` lua
periphery.SPI
```
SPI module. See [SPI documentation](spi.md) for more information.

--------------------------------------------------------------------------------

``` lua
periphery.I2C
```
I2C module. See [I2C documentation](i2c.md) for more information.

--------------------------------------------------------------------------------

``` lua
periphery.MMIO
```
MMIO module. See [MMIO documentation](mmio.md) for more information.

--------------------------------------------------------------------------------

``` lua
periphery.Serial
```
Serial module. See [Serial documentation](serial.md) for more information.

--------------------------------------------------------------------------------

``` lua
periphery.sleep(seconds <number>)
```
Sleep for the specified number of seconds.

--------------------------------------------------------------------------------

``` lua
periphery.sleep_ms(milliseconds <number>)
```
Sleep for the specified number of milliseconds.

--------------------------------------------------------------------------------

``` lua
periphery.sleep_us(microseconds <number>)
```
Sleep for the specified number of microseconds.

### EXAMPLE

``` lua
local periphery = require('periphery')

print("Hello World!")
periphery.sleep_ms(500)
print("Hello World 500ms later!")
```

