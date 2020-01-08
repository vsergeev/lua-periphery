### NAME

LED module for Linux userspace sysfs LEDs.

### SYNOPSIS

``` lua
local periphery = require('periphery')
local LED = periphery.LED

-- Constructor
led = LED(name <string>)
led = LED{name=<string>}

-- Methods
led:read() --> <boolean>
led:write(value <boolean|number>)
led:close()

-- Properties
led.brightness      mutable <number>
led.max_brightness  immutable <number>
led.name            immutable <string>
```

### DESCRIPTION

``` lua
LED(name <string>) --> <LED object>
LED{name=<string>} --> <LED object>
```

Instantiate an LED object and open the sysfs LED with the specified name.

Example:
``` lua
-- Open LED led0
led = LED("led0")
led = LED{name="led0"}
```

Returns a new LED object on success. Raises an [LED error](#errors) on failure.

--------------------------------------------------------------------------------

``` lua
led:read() --> <boolean>
```
Read the state of the LED, where `true` is non-zero brightness, and `false` is
zero brightness.

Raises an [LED error](#errors) on failure.

--------------------------------------------------------------------------------

``` lua
led:write(value <boolean|number>)
```
Write the state of the LED to `value`. Value can be a boolean (where `true` is
max brightness, and `false` is zero brightness), or an integer value for a
specific brightness.

Raises an [LED error](#errors) on failure.

--------------------------------------------------------------------------------

``` lua
led:close()
```
Close the LED.

Raises an [LED error](#errors) on failure.

--------------------------------------------------------------------------------

``` lua
Property led.brightness         mutable <number>
```
Get or set the LED's brightness.

Raises an [LED error](#errors) on failure.

--------------------------------------------------------------------------------

``` lua
Property led.max_brightness     immutable <number>
```
Get the LED's max brightness.

Raises an [LED error](#errors) on failure and on assignment.

--------------------------------------------------------------------------------

``` lua
Property led.name               immutable <string>
```
Get the name of the sysfs LED.

Raises an [LED error](#errors) on assignment.

### ERRORS

The periphery LED methods and properties may raise a Lua error on failure that can be propagated to the user or caught with Lua's `pcall()`. The error object raised is a table with `code`, `c_errno`, `message` properties, which contain the error code string, underlying C error number, and a descriptive message string of the error, respectively. The error object also provides the necessary metamethod for it to be formatted if it is propagated to the user by the interpreter.

``` lua
--- Example of error propagated to user
> periphery = require('periphery')
> led = periphery.LED("led0")
Opening LED: opening 'brightness': Permission denied [errno 13]
> 

--- Example of error caught with pcall()
> status, err = pcall(function () led = periphery.LED("led0") end)
> =status
false
> dump(err)
{
  c_errno = 13,
  message = "Opening LED: opening 'brightness': Permission denied [errno 13]",
  code = "LED_ERROR_OPEN"
}
> 
```

| Error Code            | Description                       |
|-----------------------|-----------------------------------|
| `"LED_ERROR_ARG"`     | Invalid arguments                 |
| `"LED_ERROR_OPEN"`    | Opening LED                       |
| `"LED_ERROR_QUERY"`   | Querying LED attributes           |
| `"LED_ERROR_IO"`      | Reading/writing LED brightness    |
| `"LED_ERROR_CLOSE"`   | Closing LED                       |

### EXAMPLE

``` lua
local LED = require('periphery').LED

-- Open LED led0
local led = LED("led0")

-- Turn on LED (set max brightness)
led:write(true)

-- Set half brightness
led:write(math.floor(led.max_brightness / 2))

-- Turn off LED (set zero brightness)
led:write(false)

led:close()
```

