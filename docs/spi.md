### NAME

SPI module for Linux userspace `spidev` devices.

### SYNOPSIS

``` lua
local periphery = require('periphery')
local SPI = periphery.SPI

-- Module Version
SPI.version         <string>

-- Constructor
spi = SPI(device <path string>, mode <number>, max_speed <number>)
spi = SPI{device=<path string>, mode=<number>, max_speed=<number>, bit_order="msb", bits_per_word=8, extra_flags=0}

-- Methods
spi:transfer(data <table>) --> <table>
spi:close()

-- Properties
spi.fd              immutable <number>
spi.mode            mutable <number>
spi.max_speed       mutable <number>
spi.bit_order       mutable <number>
spi.bits_per_word   mutable <number>
spi.extra_flags     mutable <number>
```

### CONSTANTS

* SPI Bit Order
    * `"msb"` - Most significant bit first transfer (typical)
    * `"lsb"` - Least significant bit first transfer

### DESCRIPTION

``` lua
Property SPI.version    immutable <string>
```
Version of SPI module as a string (e.g. "1.0.0").

--------------------------------------------------------------------------------

``` lua
SPI(device <path string>, mode <number>, max_speed <number>) --> <SPI object>
SPI{device=<path string>, mode=<number>, max_speed=<number>, bit_order="msb", bits_per_word=8, extra_flags=0} --> <SPI object>
```
Instantiate a SPI object and open the `spidev` device at the specified path with the specified SPI mode, specified max speed in hertz, and the defaults of "msb" bit order and 8 bits per word. SPI mode can be 0, 1, 2, or 3. Defaults may be overridden with the table constructor. Bit order can be "msb" or "lsb" (see [constants](#constants) above). Extra flags passed in the `extra_flags` argument will be bitwise-ORed with the SPI mode.

Example:
```
spi = SPI("/dev/spidev1.0", 0, 1e6)
spi = SPI{device="/dev/spidev1.0", mode=0, max_speed=1e6}
spi = SPI{device="/dev/spidev1.0", mode=0, max_speed=1e6, bit_order="lsb", bits_per_word=7, extra_flags=0}
```

Returns a new SPI object on success. Raises a [SPI error](#errors) on failure.

--------------------------------------------------------------------------------

``` lua
spi:transfer(data <table>) --> <table>
```
Shift out the array of words `data` and return an array of shifted in words.

Example:
``` lua
data_in = spi:transfer({0xaa, 0xbb, 0xcc, 0xdd})
```
data_in is the shifted in words, e.g. `{0xff, 0xff, 0xff, 0xff}`.

Returns shifted in words on success. Raises a [SPI error](#errors) on failure.

--------------------------------------------------------------------------------

``` lua
spi:close()
```
Close the `spidev` device.

Raises a [SPI error](#errors) on failure.

--------------------------------------------------------------------------------

``` lua
Property spi.fd     immutable <number>
```
Get the file descriptor for the underlying `spidev` device of the SPI object.

Raises a [SPI error](#errors) on assignment.

--------------------------------------------------------------------------------

``` lua
Property spi.mode           mutable <number>
Property spi.max_speed      mutable <number>
Property spi.bit_order      mutable <string>
Property spi.bits_per_word  mutable <number>
Property spi.extra_flags    mutable <number>
```
Get or set the mode, max speed, bit order, bits per word, or extra flags, respectively, of the underlying `spidev` device.

Mode can be 0, 1, 2, 3. Max speed is in Hertz. Bit order can be "msb" or "lsb" (see [constants](#constants) above). Extra flags will be bitwise-ORed with the SPI mode.

Raises a [SPI error](#errors) on assignment with an invalid value.

### ERRORS

The periphery SPI methods and properties may raise a Lua error on failure that can be propagated to the user or caught with Lua's `pcall()`. The error object raised is a table with `code`, `c_errno`, `message` properties, which contain the error code string, underlying C error number, and a descriptive message string of the error, respectively. The error object also provides the necessary metamethod for it to be formatted as a string if it is propagated to the user by the interpreter.

``` lua
--- Example of error propagated to user
> periphery = require('periphery')
> spi = periphery.SPI('/dev/spidev1.0', 0, 1e6)
Opening SPI device "/dev/spidev1.0": No such file or directory [errno 2]
> 

--- Example of error caught with pcall()
> status, err = pcall(function () spi = periphery.SPI("/dev/spidev1.0", 0, 1e6) end)
> =status
false
> dump(err)
{
  message = "Opening SPI device \"/dev/spidev1.0\": No such file or directory [errno 2]",
  c_errno = 2,
  code = "SPI_ERROR_OPEN"
}
> 
```

| Error Code                | Description                       |
|---------------------------|-----------------------------------|
| `"SPI_ERROR_ARG"`         | Invalid arguments                 |
| `"SPI_ERROR_OPEN"`        | Opening SPI device                |
| `"SPI_ERROR_QUERY"`       | Querying SPI device attributes    |
| `"SPI_ERROR_CONFIGURE"`   | Configuring SPI device attributes |
| `"SPI_ERROR_TRANSFER"`    | SPI transfer                      |
| `"SPI_ERROR_CLOSE"`       | Closing SPI device                |

### EXAMPLE

``` lua
local SPI = require('periphery').SPI

-- Open spidev1.0 with mode 0 and max speed 1MHz
local spi = SPI("/dev/spidev1.0", 0, 1e6)

local data_out = {0xaa, 0xbb, 0xcc, 0xdd}
local data_in = spi:transfer(data_out)

print(string.format("shifted out {0x%02x, 0x%02x, 0x%02x, 0x%02x}", unpack(data_out)))
print(string.format("shifted in  {0x%02x, 0x%02x, 0x%02x, 0x%02x}", unpack(data_in)))

spi:close()
```

