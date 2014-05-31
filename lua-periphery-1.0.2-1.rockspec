package = "lua-periphery"
version = "1.0.2-1"
source = {
    url = "git://github.com/vsergeev/lua-periphery",
    tag = "v1.0.2",
}
description = {
    summary = "Linux Peripheral I/O (GPIO, SPI, I2C, MMIO, Serial) with Lua",
    detailed = [[
        lua-periphery is a library for GPIO, SPI, I2C, MMIO, and Serial peripheral I/O interface access in userspace Linux. It is useful in embedded Linux environments (including BeagleBone, Raspberry Pi, etc. platforms) for interfacing with external peripherals. lua-periphery requires Lua 5.1 or greater, has no dependencies outside the standard C library and Linux, is portable across architectures, and is MIT licensed.
    ]],
    homepage = "https://github.com/vsergeev/lua-periphery",
    maintainer = "Vanya Sergeev <vsergeev@gmail.com>",
    license = "MIT/X11",
}
dependencies = {
    "lua >= 5.1",
}
build = {
    type = "command",
    build_command = "git clone git://github.com/vsergeev/c-periphery --depth 1 --branch v1.0.0 && LUA=$(LUA) LUA_INCDIR=$(LUA_INCDIR) make",
    install = {
       lib = {
           ["periphery"] = "periphery.so",
       },
    },
    copy_directories = { "docs" },
}
