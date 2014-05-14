### NAME

MMIO module for the Linux userspace `/dev/mem` device.

### SYNOPSIS

``` lua
local periphery = require('periphery')
local MMIO = periphery.MMIO

-- Module Version
MMIO.version    immutable <string>

-- Constructor
mmio = MMIO(address <physical address number>, size <number>)
mmio = MMIO{address=<physical address number>, size=<number>}

-- Methods
mmio:close()
mmio:read32(offset <number>)
mmio:read16(offset <number>)
mmio:read8(offset <number>)
mmio:read(offset <number>, length <number>) -> <table>
mmio:write32(offset <number>, value <32-bit number>)
mmio:write16(offset <number>, value <16-bit number>)
mmio:write8(offset <number>, value <8-bit number>)
mmio:write(offset <number>, data <table>)

-- Properties
mmio.base       immutable <number>
mmio.size       immutable <number>
```

### DESCRIPTION

``` lua
Property MMIO.version   immutable <string>
```
Version of MMIO module as a string (e.g. "1.0.0").

Raises an error on assignment.

--------------------------------------------------------------------------------

``` lua
MMIO(address <physical address number>, size <number>)
MMIO{address=<physical address number>, size=<number>}
```

Instantiate an MMIO object and map the region of physical memory at the specified base physical address with the specified size.

Example:
``` lua
mmio = MMIO(0x40000000, 4096)
```

Returns a new MMIO object on success. Raises a [MMIO error](#errors) on failure.

--------------------------------------------------------------------------------

``` lua
mmio:close()
```
Unmap the MMIO object's mapped physical memory.

Raises a [MMIO error](#errors) on failure.

--------------------------------------------------------------------------------

``` lua
mmio:read32(offset <number>) -> <number>
mmio:read16(offset <number>) -> <number>
mmio:read8(offset <number>) -> <number>
```
Read 32-bits, 16-bits, or 8-bits, respectively, from mapped physical memory, starting at the specified byte offset, relative to the base address the MMIO object was opened with.

Return the read unsigned integer on success. Raises a [MMIO error](#errors) on failure.

--------------------------------------------------------------------------------

``` lua
mmio:read(offset <number>, length <number>) -> <table>
```
Read an array of bytes from mapped physical memory, starting at the specified byte offset, relative to the base address the MMIO object was opened with.

Example:
``` lua
data = mmio:read(0x100, 4)
-- data is {0xff, 0xff, 0xff, 0xff}
```

Return the read bytes in a table array. Raises a [MMIO error](#errors) on failure.

--------------------------------------------------------------------------------

``` lua
mmio:write32(offset <number>, value <32-bit number>)
mmio:write16(offset <number>, value <16-bit number>)
mmio:write8(offset <number>, value <8-bit number>)
```
Write 32-bits, 16-bits, or 8-bits, respectively, to mapped physical memory, starting at the specified byte offset, relative to the base address the MMIO object was opened with.

Raises a [MMIO error](#errors) on failure.

--------------------------------------------------------------------------------

``` lua
mmio:write(offset <number>, data <table>)
```
Write an array of bytes to mapped physical memory, starting at the specified byte offset, relative to the base address the MMIO object was opened with.

Raises a [MMIO error](#errors) on failure.

--------------------------------------------------------------------------------

``` lua
Property mmio.base      immutable <number>
```
Get the base physical address the MMIO object was opened with.

Raises a [MMIO error](#errors) on assignment.

--------------------------------------------------------------------------------

``` lua
Property mmio.size      immutable <number>
```
Get the size the MMIO object was opened with.

Raises a [MMIO error](#errors) on assignment.

### ERRORS

The periphery MMIO methods and properties may raise a Lua error on failure that may be propagated to the user or caught with Lua's `pcall()`. The error object raised is a table with `code`, `c_errno`, `message` properties, which contain the error code string, underlying C error number, and a descriptive message string of the error, respectively. The error object also provides the necessary metamethod to be formatted cleanly if it is propagated to the user by the interpeter.

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

--- Open am335x control module page
local ctrl_mmio = MMIO(0x44E10000, 0x1000)
-- Read MAC address
local mac_id0_lo = ctrl_mmio:read32(0x630)
local mac_id0_hi = ctrl_mmio:read32(0x634)
print(string.format("MAC address: %04x%08x\n", mac_id0_lo, mac_id0_hi))
```

