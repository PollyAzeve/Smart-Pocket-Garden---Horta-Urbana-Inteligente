-- Módulo utilitário para gravação física de logs
local logger = {}

local function write_log(level, msg, context)
    local datetime = os.date("%Y-%m-%d %H:%M:%S")
    local prefix = context and ("[" .. context .. "] ") or ""
    local log_msg = string.format("[%s] [%s] %s%s\n", datetime, level, prefix, tostring(msg))
    
    local file, f_err = io.open("error_log.txt", "a")
    if file then
        file:write(log_msg)
        file:close()
    else
        io.stderr:write("Não foi possível gravar no error_log.txt: " .. tostring(f_err) .. "\n")
    end
end

function logger.error(err, context)
    local traceback = debug.traceback(err, 2)
    local msg = string.format("ERROR: %s\nTraceback:\n%s\n----------------------------------------\n", tostring(err), traceback)
    write_log("ERROR", msg, context)
    return traceback
end

function logger.warn(msg, context)
    write_log("WARN", msg, context)
end

function logger.info(msg, context)
    write_log("INFO", msg, context)
end

return logger