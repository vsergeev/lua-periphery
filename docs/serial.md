### NAME

Serial module for Linux userspace termios `tty` devices.

### SYNOPSIS

``` lua
local periphery = require('periphery')
local Serial = periphery.Serial

-- Module Version
Serial.version      immutable <string>

-- Constructor
serial = Serial(device <path string>, baudrate <number>) -> <Serial object>
serial = Serial{device=<path string>, baudrate=<number>, databits=8, parity="none", stopbits=1, xonxoff=false, rtscts=false} -> <Serial object>

-- Methods
serial:read(length <number>, [timeout <number>]) -> <string>
serial:read{length=<length>, timeout=nil} -> <string>
serial:write(data <string>) -> <number>
serial:flush()
serial:input_waiting() -> <number>
serial:output_waiting() -> <number>
serial:poll(timeout) -> <boolean>
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

### ENUMERATIONS

* Serial parity
    * "none": No parity
    * "odd": Odd parity
    * "even": Even parity

### DESCRIPTION

``` lua
Property Serial.version     immutable <string>
```
Version of Serial module as a string (e.g. "1.0.0").

Raises an error on assignment.

--------------------------------------------------------------------------------

``` lua
Serial(device <path string>, baudrate <number>) -> <Serial object>
Serial{device=<path string>, baudrate=<number>, databits=8, parity="none", stopbits=1, xonxoff=false, rtscts=false} -> <Serial object>

e.g.
serial = Serial("/dev/ttyUSB0", 115200)
serial = Serial{device="/dev/ttyUSB0", baudrate=115200}
```
Instantiate a serial object and open the `tty` device at the specified path (e.g. "/dev/ttyUSB0"), with the specified baudrate, and the defaults of 8 data bits, no parity, 1 stop bit, software flow control (xonxoff) off, hardware flow control (rtscts) off. Defaults may be overrided with the table constructor. 

Returns a new Serial object on success. Raises a [Serial error](#errors) on failure.

--------------------------------------------------------------------------------

``` lua
serial:read(length <number>, [timeout <number>]) -> <string>
serial:read{length=<length>, timeout=nil} -> <string>
```
Read up to `length` number of bytes from the serial port with the specified timeout. A 0 timeout can be specified for a non-blocking read. A negative timeout can be specified for a blocking read that will read until all of the requested number of bytes are read. A positive timeout in milliseconds can be specified for a blocking read with a timeout.

For a non-blocking or timeout read, `read()` returns a string with the bytes read, whose length may be less than or equal to the length requested. For a blocking read (negative timeout), `read()` returns a string with the bytes read, whose length will be the length requested. Raises a [Serial error](#errors) on failure.

--------------------------------------------------------------------------------

``` lua
serial:write(data <string>) -> <number>
```
Write the specified `data` string to the serial port.

Returns the number of bytes written. Raises a [Serial error](#errors) on failure.

--------------------------------------------------------------------------------

``` lua
serial:flush()
```
Flush the write buffer of the serial port (i.e. force its write immediately).

Raises a [Serial error](#errors) on failure.

--------------------------------------------------------------------------------

``` lua
serial:input_waiting() -> <number>
```
Query the number of bytes waiting to be read from the serial port.

Returns the number of bytes. Raises a [Serial error](#errors) on failure.

--------------------------------------------------------------------------------

``` lua
serial:output_waiting() -> <number>
```
Query the number of bytes waiting to be written to the serial port.

Returns the number of bytes. Raises a [Serial error](#errors) on failure.

--------------------------------------------------------------------------------

``` lua
serial:poll(timeout) -> <boolean>
```
Poll for data available for reading from the serial port. `timeout` can be positive for a timeout in milliseconds, 0 for a non-blocking poll, or a negative number for a blocking poll.

Returns `true` if data is available for reading from the serial port, otherwise returns `false`. Raises a [Serial error](#errors) on failure.

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

Raises a [Serial error](#errors) on invalid assignment. 

--------------------------------------------------------------------------------

``` lua
Property serial.fd          immutable <number>
```
Get the file descriptor for the underlying `tty` device of the Serial object.

Raises a [Serial error](#errors) on assignment.

--------------------------------------------------------------------------------

### ERRORS

The periphery Serial methods and properties may raise a Lua error on failure that may be propagated to the user or caught with Lua's `pcall()`. The error object raised is a table with `code`, `c_errno`, `message` properties, which contain the error code string, underlying C error number, and a descriptive message string of the error, respectively. The error object also provides the necessary metamethod to be formatted cleanly if it is propagated to the user by the interpeter.

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

| Error Code                | Description                       |
|---------------------------|-----------------------------------|
| "SERIAL_ERROR_ARG"        | Invalid arguments                 |
| "SERIAL_ERROR_OPEN"       | Opening serial port               |
| "SERIAL_ERROR_QUERY"      | Getting serial port attributes    |
| "SERIAL_ERROR_IO"         | Reading/writing serial port       |
| "SERIAL_ERROR_CONFIGURE"  | Setting serial port attributes    |
| "SERIAL_ERROR_CLOSE"      | Closing serial port               |

### EXAMPLE

``` lua
local Serial = require('periphery').Serial

-- Open /dev/ttyUSB0 with baudrate 115200, and defaults of 8N1, no flow control
local serial = periphery.Serial("/dev/ttyUSB0", 115200)

serial:write("Hello World!")
local buf = serial:read(128, 2000)
print(string.format("read %d bytes: _%s_\n", #buf, buf))

serial:close()
```

