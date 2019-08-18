--
-- lua-periphery by vsergeev
-- https://github.com/vsergeev/lua-periphery
-- License: MIT
--

require('test')
local periphery = require('periphery')
local Serial = periphery.Serial

--------------------------------------------------------------------------------

local device = nil

--------------------------------------------------------------------------------

function test_arguments()
    local serial = nil

    ptest()

    -- Invalid data bits
    passert_periphery_error("invalid databits", function () serial = Serial{["device"]=device, baudrate=115200, databits=4} end, "SERIAL_ERROR_ARG")
    passert_periphery_error("invalid databits", function () serial = Serial{["device"]=device, baudrate=115200, databits=9} end, "SERIAL_ERROR_ARG")
    -- Invalid parity
    passert_periphery_error("invalid parity", function () serial = Serial{["device"]=device, baudrate=115200, databits=8, parity="blah"} end, "SERIAL_ERROR_ARG")
    -- Invalid stopbits
    passert_periphery_error("invalid stopbits", function () serial = Serial{["device"]=device, baudrate=115200, databits=8, stopbits=0} end, "SERIAL_ERROR_ARG")
    passert_periphery_error("invalid stopbits", function () serial = Serial{["device"]=device, baudrate=115200, databits=8, stopbits=3} end, "SERIAL_ERROR_ARG")

    -- Everything else is fair game, although termios might not like it.
end

function test_open_config_close()
    local serial = nil

    ptest()

    -- Make sure module version exists
    passert("module version", Serial.version ~= nil)

    passert_periphery_success("open serial", function () serial = Serial(device, 115200) end)

    -- Confirm default settings
    passert("fd > 0", serial.fd > 0)
    passert("baudrate is 115200", serial.baudrate == 115200)
    passert("databits is 8", serial.databits == 8)
    passert("parity is none", serial.parity == "none")
    passert("stopbits is 1", serial.stopbits == 1)
    passert("xonxoff is false", serial.xonxoff == false)
    passert("rtscts is false", serial.rtscts == false)
    io.write(string.format("serial: %s\n", serial:__tostring()))

    -- Change some stuff around
    passert_periphery_success("set baudrate to 4800", function () serial.baudrate = 4800 end)
    passert("baudrate is 4800", serial.baudrate == 4800)
    passert_periphery_success("set baudrate to 9600", function () serial.baudrate = 9600 end)
    passert("baudrate is 9600", serial.baudrate == 9600)
    passert_periphery_success("set databits to 7", function () serial.databits = 7 end)
    passert("databits is 7", serial.databits == 7)
    passert_periphery_success("set parity to odd", function () serial.parity = "odd" end)
    passert("parity is odd", serial.parity == "odd")
    passert_periphery_success("set stopbits to 2", function () serial.stopbits = 2 end)
    passert("stopbits is 2", serial.stopbits == 2)
    passert_periphery_success("set xonxoff to true", function () serial.xonxoff = true end)
    passert("xonxoff is true", serial.xonxoff == true)
    --[[
    passert_periphery_success("set rtscts to true", function () serial.rtscts = true end)
    print(serial.rtscts)
    passert("rtscts is true", serial.rtscts == true)
    ]]--
    -- Test serial port may not support rtscts

    passert_periphery_success("close serial", function () serial:close() end)
end

function test_loopback()
    local serial = nil
    local lorem_ipsum = "Lorem ipsum dolor sit amet, consectetur adipisicing elit, sed do eiusmod tempor incididunt ut labore et dolore magna aliqua. Ut enim ad minim veniam, quis nostrud exercitation ullamco laboris nisi ut aliquip ex ea commodo consequat. Duis aute irure dolor in reprehenderit in voluptate velit esse cillum dolore eu fugiat nulla pariatur. Excepteur sint occaecat cupidatat non proident, sunt in culpa qui officia deserunt mollit anim id est laborum."
    local lorem_hugesum = nil
    local buf = nil
    local ret = nil

    ptest()

    passert_periphery_success("open serial", function () serial = Serial(device, 115200) end)

    -- Test write/flush/read
    passert("write lorem ipsum", serial:write(lorem_ipsum) == #lorem_ipsum)
    passert_periphery_success("flush serial", function () serial:flush() end)
    passert_periphery_success("read lorem ipsum", function () buf = serial:read(#lorem_ipsum) end)
    passert("compare write/read lorem ipsum", buf == lorem_ipsum)

    -- Test poll/write/flush/poll/input waiting/read
    passert("poll timed out", serial:poll(500) == false)
    passert("write lorem ipsum", serial:write(lorem_ipsum) == #lorem_ipsum)
    passert_periphery_success("flush serial", function () serial:flush() end)
    passert("poll succeeded", serial:poll(500) == true)
    periphery.sleep_ms(500)
    passert("input waiting is lorem ipsum size", serial:input_waiting() == #lorem_ipsum)
    passert_periphery_success("read lorem ipsum", function () buf = serial:read(#lorem_ipsum) end)
    passert("compare write/read lorem ipsum", buf == lorem_ipsum)

    -- Test non-blocking poll
    passert("non-blocking poll", serial:poll(0) == false)

    -- Test a very large read-write (likely to exceed internal buffer size (~4096))
    lorem_hugesum = string.rep("\xaa", 4096*3)
    passert("write lorem hugesum", serial:write(lorem_hugesum) == #lorem_hugesum)
    passert_periphery_success("flush", function () serial:flush() end)
    passert_periphery_success("read lorem hugesum", function () buf = serial:read(#lorem_hugesum) end)
    passert("compare write/read lorem hugesum", buf == lorem_hugesum)

    -- Test read timeout
    local tic = os.time()
    passert("read timed out", serial:read(4096*3, 2000) == "")
    local toc = os.time()
    passert("time elapsed", (toc-tic) > 1)

    -- Test non-blocking read
    tic = os.time()
    passert("read non-blocking", serial:read(4096*3, 0) == "")
    toc = os.time()
    -- Assuming we weren't context switched out for a second and weren't on a
    -- thin time boundary ;)
    passert("almost no time elapsed", (toc-tic) == 0)

    passert_periphery_success("close serial", function () serial:close() end)
end

function test_interactive()
    local serial = nil
    local buf = "Hello World!"

    ptest()

    passert_periphery_success("open serial", function () serial = Serial(device, 4800) end)

    print("Starting interactive test. Get out your logic analyzer, buddy!")
    print("Press enter to continue...")
    io.read()

    print("Press enter to start transfer...")
    io.read()
    passert("serial write", serial:write(buf) == #buf)
    print("Serial transfer baudrate 4800, 8n1 occurred? y/n")
    passert("interactive success", io.read() == "y")

    passert_periphery_success("set baudrate to 9600", function () serial.baudrate = 9600 end)

    print("Press enter to start transfer...")
    io.read()
    passert("serial write", serial:write(buf) == #buf)
    print("Serial transfer baudrate 9600, 8n1 occurred? y/n")
    passert("interactive success", io.read() == "y")

    passert_periphery_success("set baudrate to 115200", function () serial.baudrate = 115200 end)

    print("Press enter to start transfer...")
    io.read()
    passert("serial write", serial:write(buf) == #buf)
    print("Serial transfer baudrate 115200, 8n1 occurred? y/n")
    passert("interactive success", io.read() == "y")

    passert_periphery_success("close serial", function () serial:close() end)
end

if #arg < 1 then
    io.stderr:write(string.format("Usage: lua %s <serial port device>\n\n", arg[0]))
    io.stderr:write("[1/4] Arguments test: No requirements.\n")
    io.stderr:write("[2/4] Open/close test: Serial port device should be real.\n")
    io.stderr:write("[3/4] Loopback test: Serial TX and RX should be connected with a wire.\n")
    io.stderr:write("[4/4] Interactive test: Serial TX should be observed with an oscilloscope or logic analyzer.\n\n")
    io.stderr:write("Hint: for Raspberry Pi 3, enable UART0 with:\n")
    io.stderr:write("   $ echo \"dtoverlay=pi3-disable-bt\" | sudo tee -a /boot/config.txt\n")
    io.stderr:write("   $ sudo systemctl disable hciuart\n")
    io.stderr:write("   $ sudo reboot\n")
    io.stderr:write("   (Note that this will disable Bluetooth)\n")
    io.stderr:write("Use pins UART0 TXD (header pin 8) and UART0 RXD (header pin 10),\n")
    io.stderr:write("connect a loopback between TXD and RXD, and run this test with:\n")
    io.stderr:write(string.format("    lua %s /dev/ttyAMA0\n\n", arg[0]))
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

