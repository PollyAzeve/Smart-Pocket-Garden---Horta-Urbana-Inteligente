-- Ponto de entrada com proteção anti-crash
local saidas = require("saida")
local pegasus = require("pegasus")
local config = require("backend.config")
local telemetry_route = require("backend.routes.telemetry")
local response = require("backend.utils.response")
local logger = require("backend.utils.logger")

local server = pegasus:new({
    port = config.port,
    location = config.location
})

server:start(function(req, rep)
    local ok, handled = xpcall(function()
        return telemetry_route.handle(req, rep)
    end, function(err)
        logger.error(err, "SERVER_CRASH")
        return err
    end)

    if not ok then
        response.error(rep, 500, "Erro interno no processamento")
    elseif not handled then
        response.error(rep, 404, "Rota nao encontrada")
    end
end)

saidas.digitar("Servidor rodando na porta " .. tostring(config.port) .. "...")