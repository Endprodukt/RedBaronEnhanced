-- rbcapture.lua - record Red Baron game frames from MAME for rbcompare.
--
--   RBCAP_OUT=attract.rbcap RBCAP_FRAMES=2000 mame redbaron -autoboot_script tools/mame/rbcapture.lua
--
-- The set is redbaron: the program the source (historicalsource/red-baron,
-- RBARON.MAC linked 14-SEP-81) assembles to, byte for byte.
--
-- One FRAM record each time the main line's wait for the frame time ends
-- (INTWAIT's LSR CALFLG shifts a 1 out): RAM, vector RAM and the math box
-- as they are then. A frame of the port, started from one record, must end
-- with the next one's memory. Ahead of each FRAM, what the main line took
-- in during the frame before: the random numbers it read (RNDM), its reads
-- of the memory the interrupt keeps and of the hardware (IRDS), and the
-- addresses the interrupt wrote in that time (NWRT), which a comparison
-- leaves out. Format: src/capture/capture.h.
--
-- Environment:
--   RBCAP_OUT     output file (default rbcapture.rbcap)
--   RBCAP_FRAMES  stop recording after this many frames (default: until MAME exits)
--   RBCAP_PLAY    a played script, see below
--   RBCAP_INFO    1: print the input fields and the math box's items, then go on
--   RBCAP_OWNED   a file to write the list of addresses the interrupt wrote to

local CALFLG = 0x04     -- the interrupt counts frames due here; INTWAIT shifts it right
local INTFLG = 0x92     -- and vector generator frames here
local TESTSW = 0x0800
local FIRE   = 0x1802
local CRSHSN = 0x1808
local POTIN  = 0x1818   -- POKEY ALLPOT: the stick's pot the latch selects
local PRAND  = 0x181A   -- POKEY random
local NMIVEC = 0x7FFA
local STACK  = 0x0180   -- page 1 below this is variables

local out_path = os.getenv("RBCAP_OUT") or "rbcapture.rbcap"
local max_frames = tonumber(os.getenv("RBCAP_FRAMES") or "0") or 0

local machine = manager.machine
if machine.system.name ~= "redbaron" then
    emu.print_error("rbcapture: this is " .. machine.system.name ..
        ", the addresses are for redbaron; recording anyway")
end

local cpu = machine.devices[":maincpu"]
local space = cpu.spaces["program"]
local f = assert(io.open(out_path, "wb"))
f:write("RBCAP1\0\0")
f:write("ROMS", string.pack("<I4", 0x5000), space:read_range(0x3000, 0x7FFF, 8))

-- The math box's registers are inside its device: by its save state items.
local mb_reg, mb_result
do
    local mb = machine.devices[":mathbox"]
    for name, id in pairs(mb.items) do
        if os.getenv("RBCAP_INFO") then emu.print_info("rbcapture: mathbox item " .. name) end
        if name:find("m_reg") then mb_reg = emu.item(id) end
        if name:find("m_result") then mb_result = emu.item(id) end
    end
    if not mb_reg or not mb_result then emu.print_error("rbcapture: math box items not found") end
end

if os.getenv("RBCAP_INFO") then
    for tag, port in pairs(machine.ioport.ports) do
        for name, field in pairs(port.fields) do
            emu.print_info(string.format("rbcapture: port %s field '%s' mask %02X", tag, name, field.mask))
        end
    end
end

local nmi_count = 0
local frames = 0
local done = false
local FRAM_SIZE = 8 + 0x400 + 0x1000 + 34

taps = {}               -- keep the taps referenced: they are removed when collected
local quiet = false     -- the script's own reads go through the read taps too
local in_nmi = false

local function peek(a)
    quiet = true
    local v = space:read_u8(a)
    quiet = false
    return v
end

local function memory()
    quiet = true
    local d = space:read_range(0x0000, 0x03FF, 8) .. space:read_range(0x2000, 0x2FFF, 8)
    quiet = false
    local t = {}
    for i = 0, 15 do t[#t + 1] = string.pack("<I2", (mb_reg and mb_reg:read(i) or 0) & 0xFFFF) end
    t[#t + 1] = string.pack("<I2", (mb_result and mb_result:read(0) or 0) & 0xFFFF)
    return d .. table.concat(t)
end

-- A played script: RBCAP_PLAY is a list of "FRAME WHAT [ARGS]" separated by
-- ';', FRAME counted in game frames from power-on:
--   coin N | start N | fire N    close the switch for N frames
--   stick X Y                    the stick: each 0..255, 128 the middle
--   auto N                       from here on: fly about at random and fire
--                                every N frames
--   aim N                        from here on: steer at the nearest plane and
--                                fire every N frames
--   poke ADDR VAL                write a byte (hex) into the machine
--   dip PORT VAL                 set a bank of option switches (DSW0, DSW1) to VAL (hex)
local play = {}
for entry in (os.getenv("RBCAP_PLAY") or ""):gmatch("[^;]+") do
    local w = {}
    for word in entry:gmatch("%S+") do w[#w + 1] = word end
    if #w >= 2 then play[#play + 1] = { frame = tonumber(w[1]), what = w[2], a = w[3], b = w[4] } end
end
local ports = machine.ioport.ports
local function field(port, name)
    local p = ports[port]
    local fl = p and p.fields[name]
    if not fl then emu.print_error("rbcapture: no field '" .. name .. "' in " .. port) end
    return fl
end
local coin_field = field(":IN0", "Coin 1")
-- Start and fire are given to the program directly (what it reads at 1802),
-- and so is the stick (what the POKEY hands it at 1818).
local held = {}
local stick_x, stick_y = 0x80, 0x80
local pot_select = 0
local auto = 0
local aim = 0
local auto_left = 0
local aim_sx, aim_sy, aim_px, aim_py, aim_n = 1, 1, 1000, 1000, 0
local seed = 12345

local function rnd(n)   -- the script's own, so that a recording can be made again
    seed = (seed * 1103515245 + 12345) & 0x7fffffff
    return (seed >> 8) % n
end

local function clamp(v) if v < 0 then return 0 elseif v > 255 then return 255 else return v end end

local function play_frame()
    for name, left in pairs(held) do
        if left <= 0 then
            held[name] = nil
            if name == "coin" and coin_field then coin_field:set_value(0) end
        else
            held[name] = left - 1
        end
    end
    for _, e in ipairs(play) do
        if e.frame == frames then
            if e.what == "coin" then
                if coin_field then coin_field:set_value(1) end
                held.coin = tonumber(e.a or "3")
            elseif e.what == "start" or e.what == "fire" then
                held[e.what] = tonumber(e.a or "3")
            elseif e.what == "stick" then
                stick_x, stick_y = clamp(tonumber(e.a or "128")), clamp(tonumber(e.b or "128"))
                auto, aim = 0, 0
            elseif e.what == "auto" then
                auto = tonumber(e.a or "8")
                aim = 0
            elseif e.what == "aim" then
                aim = tonumber(e.a or "4")
                auto = 0
            elseif e.what == "dip" then
                local port = ports[":" .. e.a]
                local v = tonumber(e.b, 16)
                if port then
                    -- (a field's value is given in place, not shifted down)
                    for _, fl in pairs(port.fields) do
                        fl.user_value = v & fl.mask
                    end
                end
            elseif e.what == "poke" then
                quiet = true
                space:write_u8(tonumber(e.a, 16), tonumber(e.b, 16))
                f:write("POKE", string.pack("<I4I2B", 3, tonumber(e.a, 16), tonumber(e.b, 16)))
                quiet = false
            end
        end
    end
    if aim > 0 then
        -- Steer at the plane: PLSTAT+8..B is where it shows (X and Y, 16 bits
        -- each, the screen's middle at 0), PLSTAT+6 D7 that it is there.
        -- Which way the stick has to go is found by trying: when the plane
        -- is further off after a while of steering, the other way.
        local st = peek(0x4F + 6)
        if st >= 0x80 then
            local x = peek(0x4F + 8) | (peek(0x4F + 9) << 8)
            local y = peek(0x4F + 10) | (peek(0x4F + 11) << 8)
            if x >= 0x8000 then x = x - 0x10000 end
            if y >= 0x8000 then y = y - 0x10000 end
            aim_n = aim_n + 1
            if aim_n >= 6 then
                if math.abs(x) > aim_px + 8 then aim_sx = -aim_sx end
                if math.abs(y) > aim_py + 8 then aim_sy = -aim_sy end
                aim_px, aim_py, aim_n = math.abs(x), math.abs(y), 0
            end
            stick_x = clamp(128 + aim_sx * (x > 12 and 56 or (x < -12 and -56 or 0)))
            stick_y = clamp(128 + aim_sy * (y > 12 and 56 or (y < -12 and -56 or 0)))
        else
            stick_x, stick_y = 128, 128
            aim_n, aim_px, aim_py = 0, 1000, 1000
        end
        if frames % aim == 0 and not held.fire then held.fire = math.max(1, aim // 2) end
    end
    if auto > 0 then
        if auto_left <= 0 then
            stick_x = 64 + rnd(129)
            stick_y = 64 + rnd(129)
            auto_left = 3 + rnd(25)
        end
        auto_left = auto_left - 1
        if frames % auto == 0 and not held.fire then held.fire = math.max(1, auto // 2) end
    end
end

taps.fire = space:install_read_tap(FIRE, FIRE, "rbcap_fire",
    function(offset, data, mask)
        if quiet then return end
        -- (D7 high: fire. D6 low: start; MAME's own port has that one the wrong way round)
        return (held.fire and 0x80 or 0) | (held.start and 0 or 0x40)
    end)
taps.latch = space:install_write_tap(CRSHSN, CRSHSN, "rbcap_latch",
    function(offset, data, mask)
        pot_select = data & 1
    end)
taps.pot = space:install_read_tap(POTIN, POTIN, "rbcap_pot",
    function(offset, data, mask)
        if quiet then return end
        -- (as MAME: the latch's bit set gives the X axis)
        return pot_select ~= 0 and stick_x or stick_y
    end)

-- Inside the interrupt routine: from the CPU fetching the NMI vector (right
-- after it pushed PC and P) until the routine's RTI has pulled the return
-- address back (a read of the stack byte the first push went to).
local last_push = -1.0
local last_push_addr = 0
local nmi_return_addr = -1
local CYCLE = 1.0 / 1512000.0
taps.vector = space:install_read_tap(NMIVEC, NMIVEC, "rbcap_vector",
    function(offset, data, mask)
        if not quiet and machine.time:as_double() - last_push < 2.5 * CYCLE then
            in_nmi = true
            nmi_count = nmi_count + 1
            nmi_return_addr = last_push_addr + 2
        end
    end)

local rnds = {}
taps.prand = space:install_read_tap(PRAND, PRAND, "rbcap_prand",
    function(offset, data, mask)
        if not in_nmi and not quiet then rnds[#rnds + 1] = string.char(data & 0xFF) end
    end)

-- What the interrupt writes, the main line cannot be held to: owned is every
-- address it ever wrote, nwrt those of the current frame.
local owned = {}
local nwrt = {}
local irds = {}

local function ird(offset, data, mask)
    if quiet then return end
    if offset >= STACK and offset < 0x200 then
        if in_nmi and offset == nmi_return_addr then in_nmi = false end
        return
    end
    if in_nmi or not owned[offset] then return end
    -- (INTWAIT polls INTFLG until the interrupt has counted a frame: only
    -- the reads that found something are kept)
    if offset == INTFLG and (data & 0xFF) == 0 then return end
    -- (and CALFLG for the frame time: that poll is where a record is taken)
    if offset == CALFLG then return end
    irds[#irds + 1] = string.pack("<I2B", offset, data & 0xFF)
end
taps.ram_r = space:install_read_tap(0x0000, 0x03FF, "rbcap_ram_r", ird)
-- (Vector RAM is not watched for reads: MAME's vector generator fetches its
-- instructions through the same address space, and the main line reads
-- nothing there that the interrupt wrote.)

local function hw_read(offset, data, mask)
    if in_nmi or quiet then return end
    -- (and polls TESTSW until the vector generator has halted)
    if offset == TESTSW and (data & 0x40) == 0 then return end
    irds[#irds + 1] = string.pack("<I2B", offset, data & 0xFF)
end
taps.sw = space:install_read_tap(0x0800, 0x0C00, "rbcap_sw", hw_read)
taps.ea = space:install_read_tap(0x1820, 0x185F, "rbcap_ea", hw_read)

local function frame_record()
    frames = frames + 1
    play_frame()
    local r = table.concat(rnds)
    rnds = {}
    f:write("RNDM", string.pack("<I4", #r), r)
    local d = table.concat(irds)
    irds = {}
    f:write("IRDS", string.pack("<I4", #d), d)
    local w = {}
    for a, _ in pairs(nwrt) do w[#w + 1] = string.pack("<I2", a) end
    nwrt = {}
    f:write("NWRT", string.pack("<I4", 2 * #w), table.concat(w))
    f:write("TIME", string.pack("<I4", 8), string.pack("<d", machine.time:as_double()))
    f:write("FRAM", string.pack("<I4", FRAM_SIZE))
    f:write(string.pack("<I4I4", frames, nmi_count))
    f:write(memory())
    if max_frames > 0 and frames >= max_frames then
        done = true
        f:flush()
    end
end

taps.ram_w = space:install_write_tap(0x0000, 0x03FF, "rbcap_ram_w",
    function(offset, data, mask)
        if quiet then return end
        if offset >= STACK and offset < 0x200 then
            last_push = machine.time:as_double()
            last_push_addr = offset
            return
        end
        if in_nmi then
            owned[offset] = true
            nwrt[offset] = true
            return
        end
        if offset == CALFLG and not done then
            local old = peek(CALFLG)                    -- the tap runs before the write
            if (old & 1) ~= 0 and (data & 0xFF) == (old >> 1) then frame_record() end
        end
    end)
taps.vg_w = space:install_write_tap(0x2000, 0x2FFF, "rbcap_vg_w",
    function(offset, data, mask)
        if in_nmi and not quiet then
            owned[offset] = true
            nwrt[offset] = true
        end
    end)

stop_sub = emu.add_machine_stop_notifier(function()
    f:close()
    emu.print_info(string.format("rbcapture: %d frames written to %s", frames, out_path))
    local op = os.getenv("RBCAP_OWNED")
    if op then
        local t = {}
        for a, _ in pairs(owned) do t[#t + 1] = a end
        table.sort(t)
        local of = assert(io.open(op, "w"))
        for _, a in ipairs(t) do of:write(string.format("%04X\n", a)) end
        of:close()
    end
end)
