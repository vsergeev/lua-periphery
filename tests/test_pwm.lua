--
-- lua-periphery by vsergeev
-- https://github.com/vsergeev/lua-periphery
-- License: MIT
--

require('test')
local periphery = require('periphery')
local PWM = periphery.PWM

--------------------------------------------------------------------------------

local pwm_chip = nil
local pwm_channel = nil

--------------------------------------------------------------------------------

function test_arguments()
    local pwm = nil

    ptest()

    -- Invalid open types
    passert_periphery_error("invalid open types", function () pwm = PWM("foo", 0) end, "PWM_ERROR_ARG")
    passert_periphery_error("invalid open types", function () pwm = PWM{chip="foo", channel=0} end, "PWM_ERROR_ARG")
    passert_periphery_error("invalid open types", function () pwm = PWM(0, "foo") end, "PWM_ERROR_ARG")
    passert_periphery_error("invalid open types", function () pwm = PWM{chip=0, channel="foo"} end, "PWM_ERROR_ARG")
end

function test_open_config_close()
    local pwm = nil

    ptest()

    -- Open non-existent PWM chip
    passert_periphery_error("non-existent PWM chip", function () pwm = PWM(9999, pwm_channel) end, "PWM_ERROR_OPEN", 2)

    -- Open non-existent PWM channel
    passert_periphery_error("non-existent PWM channel", function () pwm = PWM(pwm_chip, 9999) end, "PWM_ERROR_OPEN", 19)

    -- Open legitimate PWM chip/channel
    passert_periphery_success("open PWM", function () pwm = PWM(pwm_chip, pwm_channel) end)

    -- Check properties
    passert("property chip", pwm.chip == pwm_chip)
    passert("property channel", pwm.channel == pwm_channel)

    -- Initialize period and duty cycle
    pwm.period = 5e-3
    pwm.duty_cycle = 0

    -- Set period, check period, check period_ns, check frequency
    pwm.period = 1e-3
    passert("period is correct", math.abs(pwm.period - 1e-3) < 1e-4)
    passert("period_ns is correct", math.abs(pwm.period_ns - 1000000) < 1e5)
    passert("frequency is correct", math.abs(pwm.frequency - 1000) < 100)
    pwm.period = 5e-4
    passert("period is correct", math.abs(pwm.period - 5e-4) < 1e-5)
    passert("period_ns is correct", math.abs(pwm.period_ns - 500000) < 1e4)
    passert("frequency is correct", math.abs(pwm.frequency - 2000) < 100)

    -- Set frequency, check frequency, check period, check period_ns
    pwm.frequency = 1000
    passert("frequency is correct", math.abs(pwm.frequency - 1000) < 100)
    passert("period is correct", math.abs(pwm.period - 1e-3) < 1e-4)
    passert("period_ns is correct", math.abs(pwm.period_ns - 1000000) < 1e5)
    pwm.frequency = 2000
    passert("frequency is correct", math.abs(pwm.frequency - 2000) < 100)
    passert("period is correct", math.abs(pwm.period - 5e-4) < 1e-5)
    passert("period_ns is correct", math.abs(pwm.period_ns - 500000) < 1e4)

    -- Set period_ns, check period_ns, check period, check frequency
    pwm.period_ns = 1000000
    passert("period_ns is correct", math.abs(pwm.period_ns - 1000000) < 1e5)
    passert("period is correct", math.abs(pwm.period - 1e-3) < 1e-4)
    passert("frequency is correct", math.abs(pwm.frequency - 1000) < 100)
    pwm.period_ns = 500000
    passert("period_ns is correct", math.abs(pwm.period_ns - 500000) < 1e4)
    passert("period is correct", math.abs(pwm.period - 5e-4) < 1e-5)
    passert("frequency is correct", math.abs(pwm.frequency - 2000) < 100)

    pwm.period_ns = 1000000

    -- Set duty cycle, check duty cycle, check duty_cycle_ns
    pwm.duty_cycle = 0.25
    passert("duty_cycle is correct", math.abs(pwm.duty_cycle - 0.25) < 1e-3)
    passert("duty_cycle_ns is correct", math.abs(pwm.duty_cycle_ns - 250000) < 1e4)
    pwm.duty_cycle = 0.50
    passert("duty_cycle is correct", math.abs(pwm.duty_cycle - 0.50) < 1e-3)
    passert("duty_cycle_ns is correct", math.abs(pwm.duty_cycle_ns - 500000) < 1e4)
    pwm.duty_cycle = 0.75
    passert("duty_cycle is correct", math.abs(pwm.duty_cycle - 0.75) < 1e-3)
    passert("duty_cycle_ns is correct", math.abs(pwm.duty_cycle_ns - 750000) < 1e4)

    -- Set duty_cycle_ns, check duty_cycle_ns, check duty_cycle
    pwm.duty_cycle_ns = 250000
    passert("duty_cycle_ns is correct", math.abs(pwm.duty_cycle_ns - 250000) < 1e4)
    passert("duty_cycle is correct", math.abs(pwm.duty_cycle - 0.25) < 1e-3)
    pwm.duty_cycle_ns = 500000
    passert("duty_cycle_ns is correct", math.abs(pwm.duty_cycle_ns - 500000) < 1e4)
    passert("duty_cycle is correct", math.abs(pwm.duty_cycle - 0.50) < 1e-3)
    pwm.duty_cycle_ns = 750000
    passert("duty_cycle_ns is correct", math.abs(pwm.duty_cycle_ns - 750000) < 1e4)
    passert("duty_cycle is correct", math.abs(pwm.duty_cycle - 0.75) < 1e-3)

    -- Set polarity, check polarity
    pwm.polarity = "normal"
    passert("polarity is correct", pwm.polarity == "normal")
    pwm.polarity = "inversed"
    passert("polarity is correct", pwm.polarity == "inversed")
    -- Set enabled, check enabled
    pwm.enabled = true
    passert("pwm is enabled", pwm.enabled == true)
    pwm.enabled = false
    passert("pwm is disabled", pwm.enabled == false)
    -- Use enable()/disable(), check enabled
    pwm:enable()
    passert("pwm is enabled", pwm.enabled == true)
    pwm:disable()
    passert("pwm is disabled", pwm.enabled == false)

    -- Set invalid polarity
    passert_periphery_error("set invalid polarity", function () pwm.polarity = "blah" end, "PWM_ERROR_ARG")

    -- Close PWM
    passert_periphery_success("close PWM", function () pwm:close() end)

    -- Open with table arguments
    passert_periphery_success("open PWM", function () pwm = PWM{chip=pwm_chip, channel=pwm_channel} end)
    passert("property chip", pwm.chip == pwm_chip)
    passert("property channel", pwm.channel == pwm_channel)
    passert_periphery_success("close PWM", function () pwm:close() end)
end

function test_loopback()
    local pwm = nil

    ptest()

    print("No general way to do a loopback test for PWM, skipping...")
end

function test_interactive()
    local pwm = nil

    ptest()

    passert_periphery_success("PWM open", function () pwm = PWM(pwm_chip, pwm_channel) end)

    print("Starting interactive test. Get out your oscilloscope, buddy!")
    print("Press enter to continue...")
    io.read()

    -- Set initial parameters and enable PWM
    pwm.duty_cycle = 0.0
    pwm.frequency = 1e3
    pwm.polarity = "normal"
    pwm.enabled = true

    -- Check tostring
    io.write(string.format("PWM description: %s\n", pwm:__tostring()))
    print("PWM description looks OK? y/n")
    passert("interactive success", io.read() == "y")

    -- Set 1 kHz frequency, 0.25 duty cycle
    pwm.frequency = 1e3
    pwm.duty_cycle = 0.25
    print("Frequency is 1 kHz, duty cycle is 25%? y/n")
    passert("interactive success", io.read() == "y")

    -- Set 1 kHz frequency, 0.50 duty cycle
    pwm.frequency = 1e3
    pwm.duty_cycle = 0.50
    print("Frequency is 1 kHz, duty cycle is 50%? y/n")
    passert("interactive success", io.read() == "y")

    -- Set 2 kHz frequency, 0.25 duty cycle
    pwm.frequency = 2e3
    pwm.duty_cycle = 0.25
    print("Frequency is 2 kHz, duty cycle is 25%? y/n")
    passert("interactive success", io.read() == "y")

    -- Set 2 kHz frequency, 0.50 duty cycle
    pwm.frequency = 2e3
    pwm.duty_cycle = 0.50
    print("Frequency is 2 kHz, duty cycle is 50%? y/n")
    passert("interactive success", io.read() == "y")

    pwm.duty_cycle = 0.0
    pwm.enabled = false

    passert_periphery_success("close PWM", function () pwm:close() end)
end

if #arg < 2 then
    io.stderr:write(string.format("Usage: lua %s <PWM chip> <PWM channel>\n\n", arg[0]))
    io.stderr:write("[1/4] Argument test: No requirements.\n")
    io.stderr:write("[2/4] Open/close test: PWM channel should be real.\n")
    io.stderr:write("[3/4] Loopback test: No test.\n")
    io.stderr:write("[4/4] Interactive test: PWM channel should be observed with an oscilloscope or logic analyzer.\n\n")
    io.stderr:write("Hint: for Raspberry Pi 3, enable PWM0 and PWM1 with:\n")
    io.stderr:write("   $ echo \"dtoverlay=pwm-2chan,pin=18,func=2,pin2=13,func2=4\" | sudo tee -a /boot/config.txt\n")
    io.stderr:write("   $ sudo reboot\n")
    io.stderr:write("Monitor GPIO 18 (header pin 12), and run this test with:\n")
    io.stderr:write(string.format("    lua %s 0 0\n", arg[0]))
    io.stderr:write("or, monitor GPIO 13 (header pin 33), and run this test with:\n")
    io.stderr:write(string.format("    lua %s 0 1\n\n", arg[0]))
    os.exit(1)
end

pwm_chip = tonumber(arg[1])
pwm_channel = tonumber(arg[2])

test_arguments()
pokay("Arguments test passed.")
test_open_config_close()
pokay("Open/close test passed.")
test_loopback()
pokay("Loopback test passed.")
test_interactive()
pokay("Interactive test passed.")

pokay("All tests passed!")

