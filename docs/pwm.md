### NAME

PWM module for Linux userspace sysfs PWMs.

### SYNOPSIS

``` lua
local periphery = require('periphery')
local PWM = periphery.PWM

-- Constructor
pwm = PWM(chip <number>, channel <number>)
pwm = PWM{chip=<number>, channel=<number>}

-- Methods
pwm:enable()
pwm:disable()
pwm:close()

-- Properties
pwm.enabled         mutable <boolean>
pwm.period_ns       mutable <number>
pwm.duty_cycle_ns   mutable <number>
pwm.period          mutable <number>
pwm.duty_cycle      mutable <number>
pwm.frequency       mutable <number>
pwm.polarity        mutable <string>
pwm.chip            immutable <number>
pwm.channel         immutable <number>
```

### CONSTANTS

* PWM Polarity
    * `"normal"` - Normal polarity
    * `"reversed"` - Reversed polarity

### DESCRIPTION

``` lua
PWM(chip <number>, channel <number>) --> <PWM object>
PWM{chip=<number>, channel=<number>} --> <PWM object>
```

Instantiate an LED object and open the sysfs PWM with the specified chip and channel.

Example:
``` lua
-- Open PWM chip 0, channel 10
pwm = PWM(0, 10)
pwm = PWM{chip=0, channel=10}
```

Returns a new PWM object on success. Raises a [PWM error](#errors) on failure.

--------------------------------------------------------------------------------

``` lua
pwm:enable()
```
Enable the PWM output.

Raises a [PWM error](#errors) on failure.

--------------------------------------------------------------------------------

``` lua
pwm:disable()
```
Disable the PWM output.

Raises a [PWM error](#errors) on failure.

--------------------------------------------------------------------------------

``` lua
pwm:close()
```
Close the PWM.

Raises a [PWM error](#errors) on failure.

--------------------------------------------------------------------------------

``` lua
Property pwm.enabled        mutable <boolean>
```
Get or set the output state of the PWM.

Raises a [PWM error](#errors) on failure.

--------------------------------------------------------------------------------

``` lua
Property pwm.period_ns      mutable <number>
Property pwm.duty_cycle_ns  mutable <number>
```
Get or set the period in nanoseconds or duty cycle in nanoseconds, respectively, of the PWM.

Raises a [PWM error](#errors) on failure.

--------------------------------------------------------------------------------

``` lua
Property pwm.period         mutable <number>
Property pwm.duty_cycle     mutable <number>
Property pwm.frequency      mutable <number>
```
Get or set the period in seconds, duty cycle as a ratio between 0.0 to 1.0, or frequency in Hz, respectively, of the PWM.

Raises a [PWM error](#errors) on failure.

--------------------------------------------------------------------------------

``` lua
Proprety pwm.polarity       mutable <string>
```
Get or set the output polarity of the PWM. Can be "normal" or "inversed" (see [constants](#constants) above).

Raises a [PWM error](#errors) on failure or on assignment with an invalid polarity.

--------------------------------------------------------------------------------

``` lua
Property pwm.chip           immutable <number>
```
Get the chip number of the PWM object.

Raises a [PWM error](#errors) on assignment.

--------------------------------------------------------------------------------

``` lua
Property pwm.channel        immutable <number>
```
Get the channel number of the PWM object.

Raises a [PWM error](#errors) on assignment.

### ERRORS

The periphery PWM methods and properties may raise a Lua error on failure that can be propagated to the user or caught with Lua's `pcall()`. The error object raised is a table with `code`, `c_errno`, `message` properties, which contain the error code string, underlying C error number, and a descriptive message string of the error, respectively. The error object also provides the necessary metamethod for it to be formatted if it is propagated to the user by the interpreter.

``` lua
--- Example of error propagated to user
> periphery = require('periphery')
> pwm = periphery.PWM(0, 10)
Opening PWM: opening 'export': Permission denied [errno 13]
> 

--- Example of error caught with pcall()
> status, err = pcall(function () pwm = periphery.PWM(0, 10) end)
> =status
false
> dump(err)
{
  c_errno = 13,
  message = "Opening PWM: opening 'export': Permission denied [errno 13]",
  code = "PWM_ERROR_OPEN"
}
> 
```

| Error Code                | Description                   |
|---------------------------|-------------------------------|
| `"PWM_ERROR_ARG"`         | Invalid arguments             |
| `"PWM_ERROR_OPEN"`        | Opening PWM                   |
| `"PWM_ERROR_QUERY"`       | Querying PWM attributes       |
| `"PWM_ERROR_CONFIGURE"`   | Configuring PWM attributes    |
| `"PWM_ERROR_CLOSE"`       | Closing PWM                   |

### EXAMPLE

``` lua
local PWM = require('periphery').PWM

-- Open PWM chip 0, channel 10
local pwm = PWM(0, 10)

-- Set frequency to 1 kHz
pwm.frequency = 1e3
-- Set duty cycle to 75%
pwm.duty_cycle = 0.75

-- Enable PWM output
pwm:enable()

pwm:close()
```

