local saidas = require("saida")

local sqlite3 = require('lsqlite3complete')

-- Cria (ou abre, se já existir) o arquivo do banco de dados local
local db = sqlite3.open('irrigacao.db')

saidas.digitar(" Criando tabelas no banco de dados SQLite3...")

-- Tabela 1: Leituras de umidade enviadas pelo ESP32 com UNIQUE e ON CONFLICT IGNORE
local tabela_sensor = [[
  CREATE TABLE IF NOT EXISTS leituras_sensor (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    sensor_id TEXT NOT NULL,
    umidade REAL NOT NULL,
    criado_em DATETIME DEFAULT CURRENT_TIMESTAMP,
    UNIQUE(sensor_id, criado_em) ON CONFLICT IGNORE
  );
]]

-- Tabela 2: Registro de acionamentos da bomba/relé
local tabela_irrigacao = [[
  CREATE TABLE IF NOT EXISTS historico_irrigacao (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    origem TEXT NOT NULL,
    duracao_segundos INTEGER NOT NULL,
    criado_em DATETIME DEFAULT CURRENT_TIMESTAMP
  );
]]

db:exec(tabela_sensor)
db:exec(tabela_irrigacao)

db:close()

print(" Banco de dados 'irrigacao.db' gerado com sucesso!")