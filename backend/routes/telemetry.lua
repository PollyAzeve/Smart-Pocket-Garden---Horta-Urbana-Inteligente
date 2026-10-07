local saidas = require("saida")
local cjson = require("cjson")
local telemetry_db = require("backend.db.telemetry_db")
local response = require("backend.utils.response")
local logger = require("backend.utils.logger")

local telemetry_route = {}

local function validate_payload(payload)
    if type(payload) ~= "table" then 
        return false, "O Payload deve ser um objeto JSON valido" 
    end
    
    if not payload.sensor_id or type(payload.sensor_id) ~= "string" or #payload.sensor_id == 0 then 
        return false, "O campo 'sensor_id' e obrigatorio e deve ser uma string nao vazia" 
    end
    
    if not payload.leituras or type(payload.leituras) ~= "table" or #payload.leituras == 0 then 
        return false, "O campo 'leituras' e obrigatorio e deve ser um array nao vazio" 
    end

    for i, item in ipairs(payload.leituras) do
        if type(item) ~= "table" then 
            return false, string.format("O item na posicao %d de 'leituras' deve ser um objeto", i) 
        end
        
        if not item.umidade or type(item.umidade) ~= "number" then 
            return false, string.format("O item na posicao %d deve possuir o campo numerico 'umidade'", i) 
        end
        
        if item.temperatura ~= nil and type(item.temperatura) ~= "number" then 
            return false, string.format("O campo 'temperatura' do item %d deve ser numerico", i) 
        end
        
        if item.pressao ~= nil and type(item.pressao) ~= "number" then 
            return false, string.format("O campo 'pressao' do item %d deve ser numerico", i) 
        end
        
        if item.irradiacao_solar ~= nil and type(item.irradiacao_solar) ~= "number" then 
            return false, string.format("O campo 'irradiacao_solar' do item %d deve ser numerico", i) 
        end
    end
    
    return true, nil
end

function telemetry_route.handle(req, rep)
    local method = req:method()
    local path = req:path()

    -- GET /api/umidade ou GET /api/telemetry (Consulta de dados pelo navegador/frontend)
    if method == "GET" and (path == "/api/umidade" or path == "/api/telemetry") then
        --print("\n[GET] Consulta de dados solicitada na rota " .. path)
        
        local success, readings = pcall(telemetry_db.get_all_readings)
        if not success then
            print("[ERRO GET] Falha ao consultar o banco de dados!")
            logger.error("Erro no GET", "GET_FAIL")
            return response.error(rep, 500, "Erro interno ao consultar banco de dados")
        end
        
       -- print("[SUCESSO GET] Retornando " .. tostring(#readings) .. " registros.")
        return response.json(rep, 200, readings)
    end

    -- POST /api/telemetry (Envio de dados pelo ESP32)
    if method == "POST" and path == "/api/telemetry" then
        
        -- CORREÇÃO: Captura o corpo bruto da requisição como string
        local body = req:receiveBody()
        
        print("\n==============================================")
        print("[POST] Recebendo nova requisicao na API!")
        print("Conteudo bruto recebido:")
        print(tostring(body))
        print("==============================================\n")

        if not body or type(body) ~= "string" or #body == 0 then 
            saidas.digitar("[ALERTA] Requisicao bloqueada: Corpo da mensagem esta vazio ou nao e texto.")
            return response.error(rep, 400, "Corpo da requisicao esta vazio") 
        end

        local ok, payload = pcall(cjson.decode, body)
        if not ok then 
            print("[ALERTA] Requisicao bloqueada: O formato JSON e invalido/malformado.")
            return response.error(rep, 400, "JSON malformado ou invalido") 
        end

        local is_valid, err_msg = validate_payload(payload)
        if not is_valid then 
            print("[ALERTA] Payload rejeitado na validacao de seguranca: " .. err_msg)
            return response.error(rep, 400, err_msg) 
        end

        local db_ok, db_err = telemetry_db.insert_batch(payload.sensor_id, payload.leituras)
        if not db_ok then
            print("[ERRO BD] Falha ao gravar no banco: " .. tostring(db_err))
            logger.error(db_err, "DB_INSERT_FAIL")
            return response.error(rep, 500, "Erro interno ao salvar leituras no banco de dados")
        end
        
        print("[SUCESSO] Lote do sensor '" .. payload.sensor_id .. "' com " .. #payload.leituras .. " leituras gravado perfeitamente no banco de dados!")
        return response.json(rep, 200, { 
            status = "success", 
            message = "Dados de telemetria gravados com sucesso" 
        })
    end
    
    return false
end

return telemetry_route