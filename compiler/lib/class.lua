local m = {}

m.functionClass = {}
m.functionClass.name = ""
m.functionClass.identifier = 0
m.functionClass.content = {
    -- Lines of graphite bytecode
}

function m.functionClass:new()
    local o = {}
    setmetatable(o, self)
    self.__index = self

    return o
end


local globalClass = {}
globalClass.name = ""
globalClass.content = ""


return m