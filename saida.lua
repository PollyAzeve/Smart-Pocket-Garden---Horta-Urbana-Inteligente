local ImpressaoLenta = {}
local function velocidade(segundos)
    os.execute("sleep " .. segundos)
end
--- Digita um texto na tela caractere por caractere de forma lenta.
-- @param texto string O texto que será exibido no terminal.
function ImpressaoLenta.digitar(texto)
    -- string.gmatch com "." extrai cada caractere individualmente
    for caractere in string.gmatch(texto, ".") do
        io.write(caractere)
        io.flush()
        velocidade(0.0003) -- Ajuste a velocidade conforme necessário
    end
    print() -- Pula uma linha ao terminar o texto
end
return ImpressaoLenta