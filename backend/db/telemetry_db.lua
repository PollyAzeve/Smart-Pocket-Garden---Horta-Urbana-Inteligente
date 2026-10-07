-- Operações de consulta e gravação seguras (Prepared Statements)
local db_module = require("backend.db.connection")
local logger = require("backend.utils.logger")

local telemetry_db = {}

function telemetry_db.insert_batch(sensor_id, leituras)
    local db = db_module.get_connection()
    
    db:exec("BEGIN TRANSACTION;")
    
    local stmt = db:prepare([[
        INSERT OR IGNORE INTO leituras_sensor 
        (sensor_id, umidade, temperatura, pressao, irradiacao_solar, status_hardware, criado_em)
        VALUES (?, ?, ?, ?, ?, ?, ?);
    ]])
    
    if not stmt then
        db:exec("ROLLBACK;")
        logger.error("Falha ao preparar declaração SQL", "DB_PREPARE_FAIL")
        return false, "Falha ao preparar declaração SQL"
    end
    
    local success, err_msg = pcall(function()
        for _, item in ipairs(leituras) do
            local timestamp = item.timestamp or item.criado_em or os.date("%Y-%m-%d %H:%M:%S")
            stmt:bind_values(
                sensor_id, 
                item.umidade, 
                item.temperatura, 
                item.pressao, 
                item.irradiacao_solar, 
                item.status_hardware, 
                timestamp
            )
            local step_result = stmt:step()
            stmt:reset()
            
            if step_result ~= 101 then
                error("Erro SQLite: " .. tostring(step_result))
            end
        end
    end)
    
    stmt:finalize()
    
    if not success then
        db:exec("ROLLBACK;")
        logger.error(err_msg, "DB_INSERT_BATCH_FAIL")
        return false, err_msg
    end
    
    db:exec("COMMIT;")
    return true, nil
end

function telemetry_db.get_all_readings()
    local db = db_module.get_connection()
    local readings = {}
    
    local stmt = db:prepare([[
        SELECT 
            id, 
            sensor_id, 
            umidade, 
            temperatura, 
            pressao, 
            irradiacao_solar, 
            status_hardware, 
            criado_em 
        FROM leituras_sensor 
        ORDER BY id DESC;
    ]])
    
    if not stmt then
        logger.error("Falha ao preparar SELECT", "DB_SELECT_FAIL")
        return readings
    end
    
    local success, err_msg = pcall(function()
        for row in stmt:nrows() do
            table.insert(readings, row)
        end
    end)
    
    stmt:finalize()
    
    if not success then
        logger.error(err_msg, "DB_SELECT_ROWS_FAIL")
    end
    
    return readings
end

return telemetry_db