### NAME

I2C module for Linux userspace `i2c-dev` devices.

### SYNOPSIS

``` lua
local periphery = require('periphery')
local I2C = periphery.I2C

-- Module Version
I2C.version     <string>

-- Constructor
i2c = I2C(device <path string>)
i2c = I2C{device=<path string>}

-- Methods
i2c:transfer(address <number>, messages <table>)
i2c:close()

-- Properties
i2c.fd          immutable <number>

-- Constants
I2C.I2C_M_TEN
I2C.I2C_M_RD
I2C.I2C_M_STOP
I2C.I2C_M_NOSTART
I2C.I2C_M_REV_DIR_ADDR
I2C.I2C_M_IGNORE_NAK
I2C.I2C_M_NO_RD_ACK
I2C.I2C_M_RECV_LEN
```

### CONSTANTS

* I2C Message Flags
    * `I2C.I2C_M_TEN`
    * `I2C.I2C_M_RD`
    * `I2C.I2C_M_STOP`
    * `I2C.I2C_M_NOSTART`
    * `I2C.I2C_M_REV_DIR_ADDR`
    * `I2C.I2C_M_IGNORE_NAK`
    * `I2C.I2C_M_NO_RD_ACK`
    * `I2C.I2C_M_RECV_LEN`

### DESCRIPTION

``` lua
Property I2C.version    <string>
```
Version of I2C module as a string (e.g. "1.0.0").

--------------------------------------------------------------------------------

``` lua
I2C(device <path string>) --> <I2C object>
I2C{device=<path string>} --> <I2C object>
```
Instantiate an I2C object and open the `i2c-dev` device at the specified path.

Example:
``` lua
i2c = I2C("/dev/i2c-1")
i2c = I2C{device="/dev/i2c-1"}
```

Returns a new I2C object on success. Raises an [I2C error](#errors) on failure.

--------------------------------------------------------------------------------

``` lua
i2c:transfer(address <number>, messages <table>)
```
Transfer `messages` to the specified I2C `address`. Modifies the `messages` table with the results of any read transactions.

The messages table is an array of message tables. Each message table is an array of bytes, as well a `flags` field containing any bitwise-ORd message flags (see the [constants](#constants) section above). The most common message flag is the `I2C.I2C_M_RD` flag, which specifies a read transaction. The read length is inferred from the number of placeholder bytes in the message table.

Example:
``` lua
local msgs = { { 0xaa, 0xbb }, { 0x00, 0x00, 0x00, flags = I2C.I2C_M_RD } }
i2c:transfer(0x50, msgs)
```
The `msgs` messages table above specifies one write transaction with two bytes `0xaa, 0xbb`, and one read transaction with placeholders for three bytes. After the transfer completes successfully, the read message contents (`0x00, 0x00, 0x00`) will be replaced with the data read from the I2C bus in that read transaction.

Raises an [I2C error](#errors) on failure.

--------------------------------------------------------------------------------

``` lua
i2c:close()
```
Close the I2C device.

Raises an [I2C error](#errors) on failure.

--------------------------------------------------------------------------------

``` lua
Property i2c.fd     immutable <number>
```
Get the file descriptor of the underlying `i2c-dev` device.

Raises an [I2C error](#errors) on assignment.

### ERRORS

The periphery I2C methods and properties may raise a Lua error on failure that can be propagated to the user or caught with Lua's `pcall()`. The error object raised is a table with `code`, `c_errno`, `message` properties, which contain the error code string, underlying C error number, and a descriptive message string of the error, respectively. The error object also provides the necessary metamethod for it to be formatted as a string if it is propagated to the user by the interpreter.

``` lua
--- Example of error propagated to user
> periphery = require('periphery')
> i2c = periphery.I2C("/dev/i2c-1")
Opening I2C device "/dev/i2c-1": No such file or directory [errno 2]
> 

--- Example of error caught with pcall()
> status, err = pcall(function () i2c = periphery.I2C("/dev/i2c-1") end)
> =status
false
> dump(err)
{
  c_errno = 2,
  code = "I2C_ERROR_OPEN",
  message = "Opening I2C device \"/dev/i2c-1\": No such file or directory [errno 2]"
}
> 
```

| Error Code                  | Description                           |
|-----------------------------|---------------------------------------|
| `"I2C_ERROR_ARG"`           | Invalid arguments                     |
| `"I2C_ERROR_OPEN"`          | Opening I2C device                    |
| `"I2C_ERROR_QUERY_SUPPORT"` | Querying I2C support on I2C device    |
| `"I2C_ERROR_NOT_SUPPORTED"` | I2C not supported on this device      |
| `"I2C_ERROR_TRANSFER"`      | I2C transfer                          |
| `"I2C_ERROR_CLOSE"`         | Closing I2C device                    |

### EXAMPLE

``` lua
local I2C = require('periphery').I2C

-- Open i2c-0 controller
local i2c = I2C("/dev/i2c-0")

--- Read byte at address 0x100 of EEPROM at 0x50
local msgs = { {0x01, 0x00}, {0x00, flags = I2C.I2C_M_RD} }
i2c:transfer(0x50, msgs)
print(string.format("0x100: 0x%02x", msgs[2][1]))

i2c:close()
```

