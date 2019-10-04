--
-- lua-periphery by vsergeev
-- https://github.com/vsergeev/lua-periphery
-- License: MIT
--

require('test')
local periphery = require('periphery')
local I2C = periphery.I2C

--------------------------------------------------------------------------------

local device = nil

--------------------------------------------------------------------------------

function test_arguments()
    local i2c = nil

    ptest()

    -- Open with invalid type
    passert_periphery_error("open invalid type", function () i2c = I2C(123) end, "I2C_ERROR_ARG")
end

function test_open_config_close()
    local i2c = nil

    ptest()

    -- Open non-existent i2c device
    passert_periphery_error("non-existent device", function () i2c = I2C("/foo/bar") end, "I2C_ERROR_OPEN")

    -- Open legitimate i2c device
    passert_periphery_success("open i2c", function () i2c = I2C(device) end)
    passert("fd > 0", i2c.fd > 0)
    io.write(string.format("i2c: %s\n", i2c:__tostring()))

    -- Close i2c
    passert_periphery_success("close i2c", function () i2c:close() end)
end

function test_loopback()
    local i2c = nil

    ptest()

    print("No general way to do a loopback test for I2C without a real component, skipping...")
end

function test_interactive()
    local i2c = nil

    ptest()

    passert_periphery_success("open i2c", function () i2c = I2C(device) end)

    print("Starting interactive test. Get out your logic analyzer, buddy!")
    print("Press enter to continue...")
    io.read()

    -- There isn't much we can do without assuming a device on the other end,
    -- because I2C needs an acknowledgement bit on each transferred byte.
    --
    -- But we can send a transaction and expect it to time out.

    -- S [ 0x7a W ] [0xaa] [0xbb] [0xcc] [0xdd] NA
    local msgs = { { 0xaa, 0xbb, 0xcc, 0xdd } }

    print("Press enter to start transfer...")
    io.read()
    passert_periphery_error("transfer to non-existent device", function () i2c:transfer(0x7a, msgs) end, "I2C_ERROR_TRANSFER", 121)
    passert_periphery_success("close i2c", function () i2c:close() end)

    print("I2C transfer occurred? y/n")
    passert("interactive success", io.read() == "y")
end

if #arg < 1 then
    io.stderr:write(string.format("Usage: lua %s <I2C device>\n\n", arg[0]))
    io.stderr:write("[1/4] Arguments test: No requirements.\n")
    io.stderr:write("[2/4] Open/close test: I2C device should be real.\n")
    io.stderr:write("[3/4] Loopback test: No test.\n")
    io.stderr:write("[4/4] Interactive test: I2C bus should be observed with an oscilloscope or logic analyzer.\n\n")
    io.stderr:write("Hint: for Raspberry Pi 3, enable I2C1 with:\n")
    io.stderr:write("   $ echo \"dtparam=i2c_arm=on\" | sudo tee -a /boot/config.txt\n")
    io.stderr:write("   $ sudo reboot\n")
    io.stderr:write("Use pins I2C1 SDA (header pin 2) and I2C1 SCL (header pin 3),\n")
    io.stderr:write("and run this test with:\n")
    io.stderr:write(string.format("    lua %s /dev/i2c-1\n\n", arg[0]))
    os.exit(1)
end

device = arg[1]

test_arguments()
pokay("Arguments test passed.")
test_open_config_close()
pokay("Open/close test passed.")
test_loopback()
pokay("Loopback test passed.")
test_interactive()
pokay("Interactive test passed.")

pokay("All tests passed!")

