### NAME

GPIO module for Linux userspace character device and sysfs GPIOs.

### SYNOPSIS

``` lua
local periphery = require('periphery')
local GPIO = periphery.GPIO

-- Constructor (for character device GPIO)
gpio = GPIO(path <string>, line <number|string>, direction <string>)
gpio = GPIO{path=<string>, line=<number|string>, direction=<string>}
-- Constructor (for sysfs GPIO)
gpio = GPIO(line <number>, direction <string>)
gpio = GPIO{line=<number>, direction=<string>}

-- Methods
gpio:read() --> <boolean>
gpio:write(value <boolean>)
gpio:poll([timeout_ms <number|nil>]) --> <boolean>
gpio:close()

-- Methods (for character device GPIO)
gpio:read_event() --> {edge=<string>, timestamp=<number>}

-- Static methods
GPIO.poll_multiple(gpios <table>, [timeout_ms <number|nil>]) --> <table>

-- Properties
gpio.direction      mutable <string>
gpio.edge           mutable <string>
gpio.bias           mutable <string>
gpio.drive          mutable <string>
gpio.inverted       mutable <boolean>
gpio.line           immutable <number>
gpio.fd             immutable <number>
gpio.name           immutable <string>
gpio.label          immutable <string>
gpio.chip_fd        immutable <number>
gpio.chip_name      immutable <string>
gpio.chip_label     immutable <string>
```

### CONSTANTS

* GPIO Direction
    * `"in"` - Input
    * `"out"` - Output, initialized to low
    * `"low"` - Output, initialized to low
    * `"high"` - Output, initialized to high

* GPIO Edge
    * `"none"` - No interrupt edge
    * `"rising"` - Rising edge (0 -> 1 transition)
    * `"falling"` - Falling edge (1 -> 0 transition)
    * `"both"` - Both edges (X -> !X transition)

* GPIO Bias
    * `"default"` - Default line bias
    * `"pull_up"` - Pull-up
    * `"pull_down"` - Pull-up
    * `"disable"` - Disable line bias

* GPIO Drive
    * `"default"` - Default line drive (push-pull)
    * `"open_drain"` - Open drain
    * `"open_source"` - Open source

### DESCRIPTION

##### Character device GPIO

``` lua
GPIO(path <string>, line <number|string>, direction <string>) --> <GPIO object>
GPIO{path=<string>, line=<number|string>, direction=<string>} --> <GPIO object>
```

Instantiate a GPIO object and open the character device GPIO with the specified line and direction at the specified GPIO chip path (e.g. `/dev/gpiochip0`). Line can be number or by string name. Direction can be "in", "out", "low", "high" (see [constants](#constants) above).

Example:
``` lua
-- Open GPIO 23 with output direction
gpio = GPIO(23, "out")
gpio = GPIO{line=23, direction="out"}
```

Returns a new GPIO object on success. Raises a [GPIO error](#errors) on failure.

--------------------------------------------------------------------------------

##### `sysfs` GPIO

``` lua
GPIO(line <number>, direction <string>) --> <GPIO object>
GPIO{line=<number>, direction=<string>} --> <GPIO object>
```

Instantiate a GPIO object and open the sysfs GPIO with the specified line and direction. Direction can be "in", "out", "low", "high" (see [constants](#constants) above).

Example:
``` lua
-- Open GPIO 23 with output direction
gpio = GPIO(23, "out")
gpio = GPIO{line=23, direction="out"}
```

Returns a new GPIO object on success. Raises a [GPIO error](#errors) on failure.

--------------------------------------------------------------------------------

``` lua
gpio:read() --> <boolean>
```
Read the state of the GPIO.

Returns `true` for high state, `false` for low state. Raises a [GPIO error](#errors) on failure.

--------------------------------------------------------------------------------

``` lua
gpio:write(value <boolean>)
```
Set the state of the GPIO to `value`, where `true` is a high state and `false` is a low state.

Raises a [GPIO error](#errors) on failure.

--------------------------------------------------------------------------------

``` lua
gpio:poll([timeout_ms <number|nil>]) --> <boolean>
```
Poll a GPIO for the edge event configured with the `.edge` property with an optional timeout.

For character device GPIOs, the edge event should be consumed with `read_event()`. For sysfs GPIOs, the edge event should be consumed with `read()`.

`timeout_ms` can be a positive number for a timeout in milliseconds, zero for a non-blocking poll, or negative or nil for a blocking poll. Default is a blocking poll.

Returns `true` if an edge event occurred, `false` on timeout. Raises a [GPIO error](#errors) on failure.

--------------------------------------------------------------------------------

``` lua
gpio:read_event() --> {edge=<string>, timestamp=<number>}
```
Read the edge event that occurred with the GPIO.

This method is intended for use with character device GPIOs and is unsupported by sysfs GPIOs.

Returns a table describing the edge event. `edge` is the edge event that occurred, either "rising" or "falling" (see [constants](#constants) above). `timestamp` is event time reported by Linux, in nanoseconds. Raises a [GPIO error](#errors) on failure.

--------------------------------------------------------------------------------

``` lua
GPIO.poll_multiple(gpios <table>, timeout_ms <number|nil>) --> <table>
```
Poll multiple GPIOs for the edge event configured with the `.edge` property with an optional timeout.

For character device GPIOs, the edge event should be consumed with `read_event()`. For sysfs GPIOs, the edge event should be consumed with `read()`.

`gpios` should be an array of GPIO objects to poll. `timeout_ms` can be a positive number for a timeout in milliseconds, zero for a non-blocking poll, or negative or nil for a blocking poll. Default is a blocking poll.

Returns an array of GPIO objects for which an edge event occurred. Raises a [GPIO error](#errors) on failure.

--------------------------------------------------------------------------------

``` lua
gpio:close()
```
Close the GPIO.

Raises a [GPIO error](#errors) on failure.

--------------------------------------------------------------------------------

``` lua
Property gpio.direction     mutable <string>
```
Get or set the GPIO's direction. Can be "in" or "out" (see [constants](#constants) above).

Raises a [GPIO error](#errors) on assignment with an invalid direction.

--------------------------------------------------------------------------------

``` lua
Property gpio.edge          mutable <string>
```
Get or set the GPIO's interrupt edge. Can be "none", "rising", "falling", or "both" (see [constants](#constants) above).

Raises a [GPIO error](#errors) on assignment with an invalid edge.

--------------------------------------------------------------------------------

``` lua
Property gpio.bias          mutable <string>
```
Get or set the GPIO's line bias. Can be "default", "pull_up", "pull_down", or "disable" (see [constants](#constants) above).

This property is not supported by sysfs GPIOs.

Raises a [GPIO error](#errors) on assignment with an invalid line bias or if unsupported by the GPIO type.

--------------------------------------------------------------------------------

``` lua
Property gpio.drive         mutable <string>
```
Get or set the GPIO's line drive. Can be "default", "open_drain", or "open_source" (see [constants](#constants) above).

This property is not supported by sysfs GPIOs.

Raises a [GPIO error](#errors) on assignment with an invalid line drive or if unsupported by the GPIO type.

--------------------------------------------------------------------------------

``` lua
Property gpio.inverted      mutable <boolean>
```
Get or set the GPIO's inverted (active low) property.

Raises a [GPIO error](#errors) on failure.

--------------------------------------------------------------------------------

``` lua
Property gpio.line          immutable <number>
```
Get the GPIO object's line number.

Raises a [GPIO error](#errors) on assignment.

--------------------------------------------------------------------------------

``` lua
Property gpio.fd            immutable <number>
```
Get the line file descriptor of the GPIO object.

Raises a [GPIO error](#errors) on assignment.

--------------------------------------------------------------------------------

``` lua
Property gpio.name          immutable <string>
```
Get the line name of the GPIO.

This method is intended for use with character device GPIOs and always returns the empty string for sysfs GPIOs.

Raises a [GPIO error](#errors) on assignment.

--------------------------------------------------------------------------------


``` lua
Property gpio.label         immutable <string>
```
Get the line consumer label of the GPIO.

This method is intended for use with character device GPIOs and always returns the empty string for sysfs GPIOs.

Raises a [GPIO error](#errors) on assignment.

--------------------------------------------------------------------------------

``` lua
Property gpio.chip_fd       immutable <number>
```
Get the GPIO chip file descriptor of the GPIO object.

This method is intended for use with character device GPIOs and is unsupported by sysfs GPIOs.

Raises a [GPIO error](#errors) on assignment. Raises a [GPIO error](#errors) for sysfs GPIOs.

--------------------------------------------------------------------------------

``` lua
Property gpio.chip_name     immutable <string>
```
Get the name of the GPIO chip associated with the GPIO.

Raises a [GPIO error](#errors) on assignment.

--------------------------------------------------------------------------------

``` lua
Property gpio.chip_label    immutable <string>
```
Get the label of the GPIO chip associated with the GPIO.

Raises a [GPIO error](#errors) on assignment.

### ERRORS

The periphery GPIO methods and properties may raise a Lua error on failure that can be propagated to the user or caught with Lua's `pcall()`. The error object raised is a table with `code`, `c_errno`, `message` properties, which contain the error code string, underlying C error number, and a descriptive message string of the error, respectively. The error object also provides the necessary metamethod for it to be formatted if it is propagated to the user by the interpreter.

``` lua
--- Example of error propagated to user
> periphery = require('periphery')
> gpio = periphery.GPIO(23, "in")
Opening GPIO: opening 'export': Permission denied [errno 13]
> 

--- Example of error caught with pcall()
> status, err = pcall(function () gpio = periphery.GPIO(23, "in") end)
> =status
false
> dump(err)
{
  c_errno = 13,
  message = "Opening GPIO: opening 'export': Permission denied [errno 13]",
  code = "GPIO_ERROR_OPEN"
}
> 
```

| Error Code                        | Description                           |
|-----------------------------------|---------------------------------------|
| `"GPIO_ERROR_ARG"`                | Invalid arguments                     |
| `"GPIO_ERROR_OPEN"`               | Opening GPIO                          |
| `"GPIO_ERROR_NOT_FOUND"`          | Line name not found                   |
| `"GPIO_ERROR_QUERY"`              | Querying GPIO attributes              |
| `"GPIO_ERROR_CONFIGURE"`          | Configuring GPIO attributes           |
| `"GPIO_ERROR_UNSUPPORTED"`        | Unsupported attribute or operation    |
| `"GPIO_ERROR_INVALID_OPERATION"`  | Invalid operation                     |
| `"GPIO_ERROR_IO"`                 | Reading/writing GPIO                  |
| `"GPIO_ERROR_CLOSE"`              | Closing GPIO                          |

### EXAMPLE

``` lua
local GPIO = require('periphery').GPIO

-- Open GPIO /dev/gpiochip0 line 10 with input direction
local gpio_in = GPIO("/dev/gpiochip0", 10, "in")
-- Open GPIO /dev/gpiochip0 line 12 with output direction
local gpio_out = GPIO("/dev/gpiochip0", 12, "out")

local value = gpio_in:read()
gpio_out:write(not value)

print("gpio_in properties")
print(string.format("\tdirection: %s", gpio_in.direction))
print(string.format("\tedge: %s", gpio_in.edge))
print(string.format("\tline: %d", gpio_in.line))
print(string.format("\tfd: %d", gpio_in.fd))
print(string.format("\tname: %s", gpio_in.name))
print(string.format("\tchip_fd: %d", gpio_in.chip_fd))
print(string.format("\tchip_name: %s", gpio_in.chip_name))
print(string.format("\tchip_label: %s", gpio_in.chip_label))

gpio_in:close()
gpio_out:close()
```

