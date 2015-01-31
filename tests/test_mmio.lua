--
-- lua-periphery by vsergeev
-- https://github.com/vsergeev/lua-periphery
-- License: MIT
--

require('test')
local periphery = require('periphery')
local MMIO = periphery.MMIO

--------------------------------------------------------------------------------

local PAGE_SIZE               = 4096

local CONTROL_MODULE_BASE     = 0x44e10000
local USB_VID_PID_OFFSET      = 0x7f4
local USB_VID_PID             = 0x04516141

local RTCSS_BASE              = 0x44e3e000
local RTC_SCRATCH2_REG_OFFSET = 0x68

--------------------------------------------------------------------------------

function test_arguments()
    ptest()
    -- Check offset out of bounds in test_open_config_close()
end

function test_open_config_close()
    local mmio = nil

    ptest()

    -- Make sure module version exists
    passert("module version", MMIO.version ~= nil)

    -- Open aligned base
    passert_periphery_success("open aligned", function () mmio = MMIO(CONTROL_MODULE_BASE, PAGE_SIZE) end)

    -- Check properties
    passert("property base", mmio.base == CONTROL_MODULE_BASE)
    passert("property size", mmio.size == PAGE_SIZE)
    io.write(string.format("mmio: %s\n", mmio:__tostring()))

    -- Try to write to immutable properties
    passert_periphery_error("write immutable", function () mmio.base = 1000 end, "MMIO_ERROR_ARG")
    passert_periphery_error("write immutable", function () mmio.size = 1000 end, "MMIO_ERROR_ARG")

    -- Open unaligned base
    passert_periphery_success("open unaligned", function () mmio = MMIO(CONTROL_MODULE_BASE + 123, PAGE_SIZE) end)
    passert("property base", mmio.base == CONTROL_MODULE_BASE + 123)
    passert("property size", mmio.size == PAGE_SIZE)

    -- Read out of bounds
    passert_periphery_error("read 1 byte over", function () mmio:read32(PAGE_SIZE-3) end, "MMIO_ERROR_ARG")
    passert_periphery_error("read 2 bytes over", function () mmio:read32(PAGE_SIZE-2) end, "MMIO_ERROR_ARG")
    passert_periphery_error("read 3 bytes over", function () mmio:read32(PAGE_SIZE-1) end, "MMIO_ERROR_ARG")
    passert_periphery_error("read 4 bytes over", function () mmio:read32(PAGE_SIZE) end, "MMIO_ERROR_ARG")

    passert_periphery_success("close unaligned", function () mmio:close() end)

    -- Open with table arguments
    passert_periphery_success("open aligned", function () mmio = MMIO{base = CONTROL_MODULE_BASE, size = PAGE_SIZE} end)
    passert("property base", mmio.base == CONTROL_MODULE_BASE)
    passert("property size", mmio.size == PAGE_SIZE)
    passert_periphery_success("close aligned", function () mmio:close() end)
end

function test_loopback()
    local mmio = nil
    local value32 = nil
    local data = nil

    -- Read USB VID/PID
    passert_periphery_success("open control module", function () mmio = MMIO(CONTROL_MODULE_BASE, PAGE_SIZE) end)
    passert_periphery_success("read USB VID/PID offset", function () value32 = mmio:read32(USB_VID_PID_OFFSET) end)
    passert("compare USB VID/PID", value32 == USB_VID_PID)
    passert_periphery_success("close control module", function () mmio:close() end)

    -- Read USB VID/PID via byte read
    passert_periphery_success("open control module", function () mmio = MMIO(CONTROL_MODULE_BASE, PAGE_SIZE) end)
    passert_periphery_success("read USB VID/PID offset", function () data = mmio:read(USB_VID_PID_OFFSET, 4) end)
    passert("byte table length is 4", #data == 4)
    passert("compare byte 1", data[1] == bit32.band(USB_VID_PID, 0xff))
    passert("compare byte 2", data[2] == bit32.band(bit32.rshift(USB_VID_PID, 8), 0xff))
    passert("compare byte 3", data[3] == bit32.band(bit32.rshift(USB_VID_PID, 16), 0xff))
    passert("compare byte 4", data[4] == bit32.band(bit32.rshift(USB_VID_PID, 24), 0xff))
    passert_periphery_success("close control module", function () mmio:close() end)

    -- Write/Read RTC Scratch2 register
    passert_periphery_success("open RTCSS", function () mmio = MMIO(RTCSS_BASE, PAGE_SIZE) end)
    passert_periphery_success("write SCRATCH2 reg", function () mmio:write32(RTC_SCRATCH2_REG_OFFSET, 0xdeadbeef) end)
    passert_periphery_success("read SCRATCH2 reg", function () value32 = mmio:read32(RTC_SCRATCH2_REG_OFFSET) end)
    passert("compare write and readback", value32 == 0xdeadbeef)
    passert_periphery_success("close RTCSS", function () mmio:close() end)

    -- Write/Read RTC Scratch2 register via byte write
    passert_periphery_success("open RTCSS", function () mmio = MMIO(RTCSS_BASE, PAGE_SIZE) end)
    data = { 0xaa, 0xbb, 0xcc, 0xdd }
    passert_periphery_success("write SCRATCH2 reg", function () mmio:write(RTC_SCRATCH2_REG_OFFSET, data) end)
    passert_periphery_success("read SCRATCH2 reg", function () data = mmio:read(RTC_SCRATCH2_REG_OFFSET, 4) end)
    passert("compare byte 1", data[1] == 0xaa)
    passert("compare byte 2", data[2] == 0xbb)
    passert("compare byte 3", data[3] == 0xcc)
    passert("compare byte 4", data[4] == 0xdd)
    passert_periphery_success("close RTCSS", function () mmio:close() end)
end

--[[
struct rtc_ss {
    volatile uint32_t seconds;          /* 0x00 */
    volatile uint32_t minutes;          /* 0x04 */
    volatile uint32_t hours;            /* 0x08 */
    volatile uint32_t days;             /* 0x0C */
    volatile uint32_t months;           /* 0x10 */
    volatile uint32_t years;            /* 0x14 */
    volatile uint32_t weeks;            /* 0x18 */

    volatile uint32_t reserved1;        /* 0x1C */

    volatile uint32_t alarm_seconds;    /* 0x20 */
    volatile uint32_t alarm_minutes;    /* 0x24 */
    volatile uint32_t alarm_hours;      /* 0x28 */
    volatile uint32_t alarm_days;       /* 0x2C */
    volatile uint32_t alarm_months;     /* 0x30 */
    volatile uint32_t alarm_years;      /* 0x34 */

    volatile uint32_t reserved2;        /* 0x38 */
    volatile uint32_t reserved3;        /* 0x3C */

    volatile uint32_t rtc_ctrl;         /* 0x40 */
    volatile uint32_t rtc_status;       /* 0x44 */
    volatile uint32_t rtc_interrupts;   /* 0x48 */
};
--]]

function _bcd_hi(x)
    return bit32.band(bit32.rshift(x, 4), 0xf)
end

function _bcd_lo(x)
    return bit32.band(x, 0xf)
end

function _bcd2dec(x)
    return 10*_bcd_hi(x) + _bcd_lo(x)
end

function test_interactive()
    local mmio = nil
    local tic = nil
    local toc = nil
    local rtc_tic = nil
    local rtc_toc = nil

    ptest()

    -- Open real time clock subsystem
    passert_periphery_success("open RTCSS", function () mmio = MMIO(RTCSS_BASE, PAGE_SIZE) end)

    io.write("Waiting for seconds ones digit to reset to 0...\n")

    tic = os.time()
    -- Wait until seconds low go to 0, so we don't have to deal with overflows
    -- in comparing times
    while (_bcd_lo(mmio:read32(0x00)) ~= 0) do
        periphery.sleep(1)
        passert("less than 12 seconds elapsed", (os.time() - tic) < 12)
    end

    -- Compare passage of os time with rtc time
    tic = os.time()
    rtc_tic = _bcd_lo(mmio:read32(0x00))

    io.write(string.format("Date: %04d-%02d-%02d\n", 2000 + _bcd2dec(mmio:read32(0x14)), _bcd2dec(mmio:read32(0x10)), _bcd2dec(mmio:read32(0x0c))))
    io.write(string.format("Time: %02d:%02d:%02d\n", _bcd2dec(bit32.band(mmio:read32(0x08), 0x7f)), _bcd2dec(mmio:read32(0x04)), _bcd2dec(mmio:read32(0x00))))

    periphery.sleep(3)

    io.write(string.format("Date: %04d-%02d-%02d\n", 2000 + _bcd2dec(mmio:read32(0x14)), _bcd2dec(mmio:read32(0x10)), _bcd2dec(mmio:read32(0x0c))))
    io.write(string.format("Time: %02d:%02d:%02d\n", _bcd2dec(bit32.band(mmio:read32(0x08), 0x7f)), _bcd2dec(mmio:read32(0x04)), _bcd2dec(mmio:read32(0x00))))

    toc = os.time()
    rtc_toc = _bcd_lo(mmio:read32(0x00))

    passert("real time elapsed", (toc-tic) > 2)
    passert("rtc time elapsed", (rtc_toc - rtc_tic) > 2)

    passert_periphery_success("close RTCSS", function () mmio:close() end)
end

print("WARNING: This test suite assumes a BeagleBone Black (AM335x) host!")
print("Other systems may experience unintended and dire consequences!")
print("Press enter to continue!")
io.read()

test_arguments()
pokay("Arguments test passed.")
test_open_config_close()
pokay("Open/close test passed.")
test_loopback()
pokay("Loopback test passed.")
test_interactive()
pokay("Interactive test passed.")

pokay("All tests passed!")

