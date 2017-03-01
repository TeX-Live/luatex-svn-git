if not modules then modules = { } end modules ['luatex-core'] = {
    version   = 1.04,
    comment   = "companion to luatex",
    author    = "Hans Hagen & Luigi Scarso",
    copyright = "LuaTeX Development Team",
}

-- This file overloads some Lua functions. The readline variants provide the same
-- functionality as LuaTeX <= 1.04 and doing it this way permits us to keep the
-- original io libraries clean. Performance is probably even a bit better now.

local type, next, getmetatable, require = type, next, getmetatable, require
local find, gsub = string.find, string.gsub

local io_open             = io.open
local io_popen            = io.popen

local fio_readline        = fio.readline
local fio_checkpermission = fio.checkpermission
local fio_recordfilename  = fio.recordfilename

local mt                  = getmetatable(io.stderr)

local saferoption         = false
local shellescape         = status.shell_escape     -- 0 1 2
local restricted          = status.shell_restricted -- 0 1
local kpseused            = status.kpse_used        -- 0 1

local function luatex_io_open(name,how)
    if not how then
        how = "r"
    end
    local f = io_open(name,how)
    if f then
        if type(how) == "string" and find(how,"w") then
            fio_recordfilename(name,"w")
        else
            fio_recordfilename(name,"r")
        end
    end
    return f
end

local function luatex_io_open_readonly(name,how)
    if how then
        how = "r"
    else
        how = gsub(how,"[^rb]","")
        if how == "" then
            how = "r"
        end
    end
    local f = io_open(name,how)
    if f then
        fio_recordfilename(name,"r")
    end
    return f
end

local function luatex_io_popen(name,...)
    local okay, found = fio_checkpermission(name)
    if okay and found then
        return io_popen(found,...)
    end
end

local function luatex_io_lines(name)
    local f = io_open(name,'r')
    if f then
        return function()
            return fio_readline(f)
        end
    end
end

local function luatex_io_readline(f)
    return function()
        return fio_readline(f)
    end
end

io.saved_lines = io.lines
mt.saved_lines = mt.lines

io.lines = luatex_io_lines
mt.lines = luatex_io_readline

if kpseused == 1 then

    io.open  = luatex_io_open
    io.popen = luatex_io_popen

    if saferoption then

        os.execute = nil
        os.spawn   = nil
        os.exec    = nil

        io.popen   = nil
        io.open    = luatex_io_open_readonly

        os.rename  = nil
        os.remove  = nil

        io.tmpfile = nil
        io.output  = nil

        lfs.chdir  = nil
        lfs.lock   = nil
        lfs.touch  = nil
        lfs.rmdir  = nil
        lfs.mkdir  = nil

    end

    if saferoption or shellescape == 0 or (shellescape == 1 and restricted == 1) then
        local ffi = require("ffi")
        for k, v in next, ffi do
            if k ~= 'gc' then
                ffi[k] = nil
            end
            ffi = nil
        end
    end

    -- todo: os.execute
    -- todo: os.spawn
    -- todo: os.exec

else

    -- We assume management to be provided by the replacement of kpse. This is the
    -- case in ConTeXt.

end
