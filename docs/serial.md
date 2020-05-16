### NAME

Serial module for Linux userspace termios `tty` devices.

### SYNOPSIS

``` lua
local periphery = require('periphery')
local Serial = periphery.Serial

-- Constructor
serial = Serial(device <path string>, baudrate <number>)
serial = Serial{device=<path string>, baudrate=<number>, databits=8,
                parity="none", stopbits=1, xonxoff=false, rtscts=false}

-- Methods
serial:read(length <number>, [timeout_ms <number|nil>]) --> <string>
serial:read{length=<length>, timeout_ms=nil} --> <string>
serial:write(data <string>) --> <number>
serial:poll([timeout_ms <number|nil>]) --> <boolean>
serial:flush()
serial:input_waiting() --> <number>
serial:output_waiting() --> <number>
serial:close()

-- Properties
serial.baudrate     mutable <number>
serial.databits     mutable <number>
serial.parity       mutable <string>
serial.stopbits     mutable <number>
serial.xonxoff      mutable <boolean>
serial.rtscts       mutable <boolean>
serial.fd           immutable <number>
```

### CONSTANTS

* Serial parity
    * `"none"` - No parity
    * `"odd"` - Odd parity
    * `"even"` - Even parity

### DESCRIPTION

``` lua
Serial(device <path string>, baudrate <number>) --> <Serial object>
Serial{device=<path string>, baudrate=<number>, databits=8,
       parity="none", stopbits=1, xonxoff=false, rtscts=false} --> <Serial object>
```

Instantiate a serial object and open the `tty` device at the specified path with the specified baudrate, and the defaults of 8 data bits, no parity, 1 stop bit, no software flow control (xonxoff), and no hardware flow control (rtscts). Defaults may be overridden with the table constructor. Parity can be "none", "odd", "even" (see [constants](#constants) above).

Example:
``` lua
serial = Serial("/dev/ttyUSB0", 115200)
serial = Serial{device="/dev/ttyUSB0", baudrate=115200, stopbits=2}
```

Returns a new Serial object on success. Raises a [Serial error](#errors) on failure.

--------------------------------------------------------------------------------

``` lua
serial:read(length <number>, [timeout_ms <number|nil>]) --> <string>
serial:read{length=<length>, timeout_ms=nil} --> <string>
```
Read up to `length` number of bytes from the serial port with an optional timeout. `timeout_ms` can be positive for a blocking read with a timeout in milliseconds, zero for a non-blocking read, or negative or nil for a blocking read that will block until `length` number of bytes are read. Default is a blocking read.

For a non-blocking or timeout bound read, `read()` may return less than the requested number of bytes.

Returns bytes read as a string. Raises a [Serial error](#errors) on failure.

--------------------------------------------------------------------------------

``` lua
serial:write(data <string>) --> <number>
```
Write the specified `data` string to the serial port.

Returns the number of bytes written. Raises a [Serial error](#errors) on failure.

--------------------------------------------------------------------------------

``` lua
serial:poll([timeout_ms <number|nil>]) --> <boolean>
```
Poll for data available for reading from the serial port with an optional timeout. `timeout_ms` can be positive for a timeout in milliseconds, zero for a non-blocking poll, or negative or nil for a blocking poll. Default is a blocking poll.

Returns `true` if data is available for reading from the serial port, otherwise returns `false`. Raises a [Serial error](#errors) on failure.

--------------------------------------------------------------------------------

``` lua
serial:flush()
```
Flush the write buffer of the serial port, blocking until all bytes are written.

Raises a [Serial error](#errors) on failure.

--------------------------------------------------------------------------------

``` lua
serial:input_waiting() --> <number>
```
Query the number of bytes waiting to be read from the serial port.

Returns the number of bytes. Raises a [Serial error](#errors) on failure.

--------------------------------------------------------------------------------

``` lua
serial:output_waiting() --> <number>
```
Query the number of bytes waiting to be written to the serial port.

Returns the number of bytes. Raises a [Serial error](#errors) on failure.

--------------------------------------------------------------------------------

``` lua
serial:close()
```
Close the `tty` device.

Raises a [Serial error](#errors) on failure.

--------------------------------------------------------------------------------

``` lua
Property serial.baudrate    mutable <number>
Property serial.databits    mutable <number>
Property serial.parity      mutable <string>
Property serial.stopbits    mutable <number>
Property serial.xonxoff     mutable <boolean>
Property serial.rtscts      mutable <boolean>
```
Get or set the baudrate, data bits, parity, stop bits, software flow control (xonxoff), or hardware flow control (rtscts), respectively, of the underlying `tty` device.

Databits can be 5, 6, 7, or 8. Stopbits can be 1, or 2. Parity can be "none", "odd", or "even" (see [constants](#constants) above).

Raises a [Serial error](#errors) on invalid assignment.

--------------------------------------------------------------------------------

``` lua
Property serial.fd          immutable <number>
```
Get the file descriptor for the underlying `tty` device of the Serial object.

Raises a [Serial error](#errors) on assignment.

### ERRORS

The periphery Serial methods and properties may raise a Lua error on failure that can be propagated to the user or caught with Lua's `pcall()`. The error object raised is a table with `code`, `c_errno`, `message` properties, which contain the error code string, underlying C error number, and a descriptive message string of the error, respectively. The error object also provides the necessary metamethod for it to be formatted as a string if it is propagated to the user by the interpreter.

``` lua
--- Example of error propagated to user
> periphery = require('periphery')
> serial = periphery.Serial("/dev/ttyUSB0", 115200)
Opening serial port "/dev/ttyUSB0": No such file or directory [errno 2]
> 

--- Example of error caught with pcall()
> status, err = pcall(function () serial = periphery.Serial("/dev/ttyUSB0", 115200) end)
> =status
false
> dump(err)
{
  code = "SERIAL_ERROR_OPEN",
  c_errno = 2,
  message = "Opening serial port \"/dev/ttyUSB0\": No such file or directory [errno 2]"
}
> 
```

| Error Code                    | Description                           |
|-------------------------------|---------------------------------------|
| `"SERIAL_ERROR_ARG"`          | Invalid arguments                     |
| `"SERIAL_ERROR_OPEN"`         | Opening serial port                   |
| `"SERIAL_ERROR_QUERY"`        | Querying serial port attributes       |
| `"SERIAL_ERROR_CONFIGURE"`    | Configuring serial port attributes    |
| `"SERIAL_ERROR_IO"`           | Reading/writing serial port           |
| `"SERIAL_ERROR_CLOSE"`        | Closing serial port                   |

### EXAMPLE

``` lua
local Serial = require('periphery').Serial

-- Open /dev/ttyUSB0 with baudrate 115200, and defaults of 8N1, no flow control
local serial = Serial("/dev/ttyUSB0", 115200)

serial:write("Hello World!")

-- Read up to 128 bytes with 500ms timeout
local buf = serial:read(128, 500)
print(string.format("read %d bytes: _%s_", #buf, buf))

serial:close()
```

