### NAME

MMIO module for the Linux userspace `/dev/mem` device.

### SYNOPSIS

``` lua
local periphery = require('periphery')
local MMIO = periphery.MMIO

-- Module Version
MMIO.version    <string>

-- Constructor
mmio = MMIO(address <number>, size <number>)
mmio = MMIO{address=<number>, size=<number>}

-- Methods
mmio:read32(offset <number>) --> <number>
mmio:read16(offset <number>) --> <number>
mmio:read8(offset <number>) --> <number>
mmio:read(offset <number>, length <number>) --> <table>
mmio:write32(offset <number>, value <number>)
mmio:write16(offset <number>, value <number>)
mmio:write8(offset <number>, value <number>)
mmio:write(offset <number>, data <table>)
mmio:close()

-- Properties
mmio.base       immutable <number>
mmio.size       immutable <number>
```

### DESCRIPTION

``` lua
Property MMIO.version   immutable <string>
```
Version of MMIO module as a string (e.g. "1.0.0").

--------------------------------------------------------------------------------

``` lua
MMIO(address <number>, size <number>) --> <MMIO Object>
MMIO{address=<number>, size=<number>} --> <MMIO Object>
```
Instantiate an MMIO object and map in the region of physical memory specified by the `address` base physical address and `size` size in bytes.

Example:
``` lua
mmio = MMIO(0x40000000, 4096)
mmio = MMIO{address=0x40001000, 0x1000}
```

Returns a new MMIO object on success. Raises an [MMIO error](#errors) on failure.

--------------------------------------------------------------------------------

``` lua
mmio:read32(offset <number>) --> <number>
mmio:read16(offset <number>) --> <number>
mmio:read8(offset <number>) --> <number>
```
Read 32-bits, 16-bits, or 8-bits, respectively, from the mapped physical memory, starting at the specified byte offset, relative to the base physical address the MMIO object was opened with.

Returns the read unsigned integer on success. Raises an [MMIO error](#errors) on failure.

--------------------------------------------------------------------------------

``` lua
mmio:read(offset <number>, length <number>) --> <table>
```
Read an array of bytes from the mapped physical memory, starting at the specified byte offset, relative to the base physical address the MMIO object was opened with.

Example:
``` lua
data = mmio:read(0x100, 4)
-- data is {0xff, 0xff, 0xff, 0xff}
```

Returns the read bytes in a table array. Raises an [MMIO error](#errors) on failure.

--------------------------------------------------------------------------------

``` lua
mmio:write32(offset <number>, value <number>)
mmio:write16(offset <number>, value <number>)
mmio:write8(offset <number>, value <number>)
```
Write 32-bits, 16-bits, or 8-bits, respectively, to mapped physical memory, starting at the specified byte offset, relative to the base physical address the MMIO object was opened with.

Raises an [MMIO error](#errors) on failure.

--------------------------------------------------------------------------------

``` lua
mmio:write(offset <number>, data <table>)
```
Write an array of bytes to mapped physical memory, starting at the specified byte offset, relative to the base physical address the MMIO object was opened with.

Raises an [MMIO error](#errors) on failure.

--------------------------------------------------------------------------------

``` lua
mmio:close()
```
Unmap the MMIO object's mapped physical memory.

Raises an [MMIO error](#errors) on failure.

--------------------------------------------------------------------------------

``` lua
Property mmio.base      immutable <number>
```
Get the base physical address the MMIO object was opened with.

Raises an [MMIO error](#errors) on assignment.

--------------------------------------------------------------------------------

``` lua
Property mmio.size      immutable <number>
```
Get the mapping size the MMIO object was opened with.

Raises an [MMIO error](#errors) on assignment.

### ERRORS

The periphery MMIO methods and properties may raise a Lua error on failure that can be propagated to the user or caught with Lua's `pcall()`. The error object raised is a table with `code`, `c_errno`, `message` properties, which contain the error code string, underlying C error number, and a descriptive message string of the error, respectively. The error object also provides the necessary metamethod for it to be formatted as a string if it is propagated to the user by the interpreter.

``` lua
--- Example of error propagated to user
> periphery = require('periphery')
> mmio = periphery.MMIO(0x40000000, 0x1000)
Opening /dev/mem: Permission denied [errno 13]
> 

--- Example of error caught with pcall()
> status, err = pcall(function () mmio = periphery.MMIO(0x400000000, 0x1000) end)
> =status
false
> dump(err)
{
  message = "Opening /dev/mem: Permission denied [errno 13]",
  c_errno = 13,
  code = "MMIO_ERROR_OPEN"
}
> 
```

| Error Code            | Description                   |
|-----------------------|-------------------------------|
| "MMIO_ERROR_ARG"      | Invalid arguments             |
| "MMIO_ERROR_OPEN"     | Opening /dev/mem              |
| "MMIO_ERROR_MAP"      | Mapping memory                |
| "MMIO_ERROR_CLOSE"    | Closing /dev/mem              |
| "MMIO_ERROR_UNMAP"    | Unmapping memory              |

### EXAMPLE

``` lua
local MMIO = require('periphery').MMIO

-- Open am335x real-time clock subsystem page
local rtc_mmio = MMIO(0x44E3E000, 0x1000)

--- Read current time
local rtc_secs = rtc_mmio:read32(0x00)
local rtc_mins = rtc_mmio:read32(0x04)
local rtc_hrs = rtc_mmio:read32(0x08)

print(string.format("hours: %02x minutes: %02x seconds: %02x", rtc_secs, rtc_mins, rtc_hrs))

rtc_mmio:close()

--- Open am335x control module page
local ctrl_mmio = MMIO(0x44E10000, 0x1000)

-- Read MAC address
local mac_id0_lo = ctrl_mmio:read32(0x630)
local mac_id0_hi = ctrl_mmio:read32(0x634)

print(string.format("MAC address: %04x %08x", mac_id0_lo, mac_id0_hi))

ctrl_mmio:close()
```

