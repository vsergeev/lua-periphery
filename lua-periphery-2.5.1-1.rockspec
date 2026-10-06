rockspec_format = "3.0"
package = "lua-periphery"
version = "2.5.1-1"
source = {
    url = "git+https://github.com/vsergeev/lua-periphery",
    tag = "v2.5.1",
}
description = {
    summary = "Linux Peripheral I/O (GPIO, LED, PWM, SPI, I2C, MMIO, Serial) with Lua",
    detailed = [[
        lua-periphery is a library for GPIO, LED, PWM, SPI, I2C, MMIO, and Serial peripheral I/O interface access in userspace Linux. It is useful in embedded Linux environments (including Raspberry Pi, BeagleBone, etc. platforms) for interfacing with external peripherals. lua-periphery requires Lua 5.1 or greater, has no dependencies outside the standard C library and Linux, is portable across architectures, and is MIT licensed.
    ]],
    homepage = "https://github.com/vsergeev/lua-periphery",
    maintainer = "Vanya Sergeev <v@sergeev.io>",
    license = "MIT/X11",
}
dependencies = {
    "lua >= 5.1",
}
build = {
    type = "make",
    variables = {
        LUA_INCDIR = "$(LUA_INCDIR)",
        LUA_LIBDIR = "$(LIBDIR)",
    },
    copy_directories = { "docs" },
}
