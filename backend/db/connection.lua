-- Gerenciamento de conexão e criação automática do banco SQLite3
local sqlite3 = require("lsqlite3")
local config = require("backend.config")

local db_module = {}
local db = nil

function db_module.get_connection()
    if not db then
        db = sqlite3.open(config.db_path)
        db_module.init_tables()
    end
    return db
end

function db_module.init_tables()
    local conn = db or sqlite3.open(config.db_path)
    
    local sql_sensor = [[
        CREATE TABLE IF NOT EXISTS leituras_sensor (
            id INTEGER PRIMARY KEY AUTOINCREMENT,
            sensor_id TEXT NOT NULL,
            umidade REAL,
            temperatura REAL,
            pressao REAL,
            irradiacao_solar REAL,
            status_hardware TEXT,
            criado_em DATETIME DEFAULT CURRENT_TIMESTAMP,
            UNIQUE(sensor_id, criado_em) ON CONFLICT IGNORE
        );
    ]]
    conn:exec(sql_sensor)

    local sql_irrigacao = [[
        CREATE TABLE IF NOT EXISTS historico_irrigacao (
            id INTEGER PRIMARY KEY AUTOINCREMENT,
            origem TEXT NOT NULL,
            duracao_segundos INTEGER NOT NULL,
            criado_em DATETIME DEFAULT CURRENT_TIMESTAMP
        );
    ]]
    conn:exec(sql_irrigacao)
end

return db_module