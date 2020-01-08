--
-- lua-periphery by vsergeev
-- https://github.com/vsergeev/lua-periphery
-- License: MIT
--

local STR_OK
local STR_FAIL

if _VERSION:match("%d.%d") == "5.1" then
    STR_OK = " [ OK ]"
    STR_FAIL = " [FAIL]"
else
    STR_OK = " [\x1b[1;32m OK \x1b[0m]"
    STR_FAIL = " [\x1b[1;31mFAIL\x1b[0m]"
end

function ptest()
    local callerInfo = debug.getinfo(2, "ln")
    print(string.format("\n\nStarting test %s():%d", callerInfo.name, callerInfo.currentline))
end

function pokay(msg)
    local callerInfo = debug.getinfo(2, "lnS")
    if callerInfo.name ~= nil then
        print(string.format("%s  %s %s():%d  %s", STR_OK, callerInfo.short_src, callerInfo.name, callerInfo.currentline, msg))
    else
        print(string.format("%s  %s %d  %s", STR_OK, callerInfo.short_src, callerInfo.currentline, msg))
    end
end

function pfail(msg)
    local callerInfo = debug.getinfo(2, "lnS")
    if callerInfo.name ~= nil then
        print(string.format("%s  %s %s():%d  %s", STR_FAIL, callerInfo.short_src, callerInfo.name, callerInfo.currentline, msg))
    else
        print(string.format("%s  %s %d  %s", STR_OK, callerInfo.short_src, callerInfo.currentline, msg))
    end
end

function passert(name, condition)
    local callerInfo = debug.getinfo(2, "lnS")
    if condition then
        print(string.format("%s  %s %s():%d  %s", STR_OK, callerInfo.short_src, callerInfo.name, callerInfo.currentline, name))
    else
        print(string.format("%s  %s %s():%d  %s", STR_FAIL, callerInfo.short_src, callerInfo.name, callerInfo.currentline, name))
        os.exit(1)
    end
end

function passert_periphery_error(name, func, code, c_errno)
    local callerInfo = debug.getinfo(2, "lnS")
    local status, err = pcall(func)
    if status then
        print(string.format("%s  %s %s():%d  %s    did not fail", STR_FAIL, callerInfo.short_src, callerInfo.name, callerInfo.currentline, name))
        os.exit(1)
    else
        if err.code ~= code then
            print(string.format("%s  %s %s():%d  %s    error code mismatch. expected %s, got %s", STR_FAIL, callerInfo.short_src, callerInfo.name, callerInfo.currentline, name, code, err.code))
            os.exit(1)
        end
        if c_errno and err.c_errno ~= c_errno then
            print(string.format("%s  %s %s():%d  %s    c_errno mismatch. expected %d, got %d", STR_FAIL, callerInfo.short_src, callerInfo.name, callerInfo.currentline, name, errno, err.c_errno))
            os.exit(1)
        end
    end
    print(string.format("%s  %s %s():%d  %s", STR_OK, callerInfo.short_src, callerInfo.name, callerInfo.currentline, name))
end

function passert_periphery_success(name, func, code)
    local callerInfo = debug.getinfo(2, "lnS")
    local status, err = pcall(func)
    if status then
        print(string.format("%s  %s %s():%d  %s", STR_OK, callerInfo.short_src, callerInfo.name, callerInfo.currentline, name))
    else
        print(string.format("%s  %s %s():%d  %s    failed with error: %s", STR_FAIL, callerInfo.short_src, callerInfo.name, callerInfo.currentline, name, tostring(err)))
        os.exit(1)
    end
end

