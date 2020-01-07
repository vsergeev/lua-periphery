--
-- lua-periphery by vsergeev
-- https://github.com/vsergeev/lua-periphery
-- License: MIT
--

require('test')
local periphery = require('periphery')
local LED = periphery.LED

--------------------------------------------------------------------------------

local led_name = nil

--------------------------------------------------------------------------------

function test_arguments()
    local led = nil

    ptest()

    -- Invalid open type
    passert_periphery_error("invalid open type", function () led = LED(123) end, "LED_ERROR_ARG")
    passert_periphery_error("invalid open type", function () led = LED{name={}} end, "LED_ERROR_ARG")
end

function test_open_config_close()
    local led = nil

    ptest()

    -- Open non-existent LED
    passert_periphery_error("non-existent LED", function () led = LED("nonexistent") end, "LED_ERROR_OPEN", 2)

    -- Open legitimate LED
    passert_periphery_success("open LED", function () led = LED(led_name) end)

    -- Check properties
    passert("property name", led.name == led_name)
    passert("max_brightness > 0", led.max_brightness > 0)

    -- Write true, read true, check brightness is max
    led:write(true)
    periphery.sleep_ms(10)
    passert("read true", led:read() == true)
    passert("brightness is max", led.brightness == led.max_brightness)

    -- Write false, read false, check brightness is zero
    led:write(false)
    periphery.sleep_ms(10)
    passert("read false", led:read() == false)
    passert("brightness is zero", led.brightness == 0)

    -- Set brightness to 1, check brightness
    led.brightness = 1
    periphery.sleep_ms(10)
    passert("brightness is non-zero", led.brightness >= 1)

    -- Set brightness to 0, check brightness
    led.brightness = 0
    periphery.sleep_ms(10)
    passert("brightness is zero", led.brightness == 0)

    -- Close LED
    passert_periphery_success("close LED", function () led:close() end)

    -- Open with table arguments
    passert_periphery_success("open LED", function () led = LED{name=led_name} end)
    passert("property name", led.name == led_name)
    passert("max_brightness > 0", led.max_brightness > 0)
    passert_periphery_success("close LED", function () led:close() end)
end

function test_loopback()
    local led = nil

    ptest()

    print("No general way to do a loopback test for LED, skipping...")
end

function test_interactive()
    local led = nil

    ptest()

    passert_periphery_success("open LED", function () led = LED(led_name) end)

    print("Starting interactive test...")
    print("Press enter to continue...")
    io.read()

    -- Check tostring
    io.write(string.format("LED description: %s\n", led:__tostring()))
    print("LED description looks OK? y/n")
    passert("interactive success", io.read() == "y")

    -- Turn LED off
    led:write(false)
    print("LED is off? y/n")
    passert("interactive success", io.read() == "y")

    -- Turn LED on
    led:write(true)
    print("LED is on? y/n")
    passert("interactive success", io.read() == "y")

    -- Turn LED off
    led:write(false)
    print("LED is off? y/n")
    passert("interactive success", io.read() == "y")

    -- Turn LED on
    led:write(true)
    print("LED is on? y/n")
    passert("interactive success", io.read() == "y")

    passert_periphery_success("close LED", function () led:close() end)
end

if #arg < 1 then
    io.stderr:write(string.format("Usage: lua %s <LED name>\n\n", arg[0]))
    io.stderr:write("[1/4] Argument test: No requirements.\n")
    io.stderr:write("[2/4] Open/close test: LED should be real.\n")
    io.stderr:write("[3/4] Loopback test: No test.\n")
    io.stderr:write("[4/4] Interactive test: LED should be observed.\n\n")
    io.stderr:write("Hint: for Raspberry Pi 3, disable triggers for led1:\n")
    io.stderr:write("    $ echo none > /sys/class/leds/led1/trigger\n")
    io.stderr:write("Observe led1 (red power LED), and run this test:\n")
    io.stderr:write(string.format("    lua %s led1\n\n", arg[0]))
    os.exit(1)
end

led_name = arg[1]

test_arguments()
pokay("Arguments test passed.")
test_open_config_close()
pokay("Open/close test passed.")
test_loopback()
pokay("Loopback test passed.")
test_interactive()
pokay("Interactive test passed.")

pokay("All tests passed!")

