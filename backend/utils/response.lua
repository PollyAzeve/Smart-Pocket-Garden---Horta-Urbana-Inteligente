local cjson = require("cjson")
local response_utils = {}

function response_utils.json(rep, status_code, data)
    rep:addHeader("Content-Type", "application/json")
    
    -- Correção: status é uma propriedade (atribuição direta) e não um método
    rep.status = status_code 
    
    rep:write(cjson.encode(data))
    return true
end

function response_utils.error(rep, status_code, message)
    return response_utils.json(rep, status_code, {
        status = "error",
        message = message
    })
end

return response_utils
