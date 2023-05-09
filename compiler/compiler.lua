-- Graphite language compiler
-- V1.0, this is different from the Graphite Virtual Machine

-- Parse graphite code into commands
-- When the command is an import, call the importEval() function, which will generate a blob of
-- graphite code
-- After imports are evaled, generate table of commands
-- Generate sector 0, which contains a GOTOSECTOR command that links to the main function
-- Loop over table of commands, and generate graphite bytecode

local serpent = require("serpent")
local file = "examples/hello_world.gr"
local currentScope = "main"
local sectorId = 0
local bufferId = 0
local sectors = {}
local variables = {}

table.insert(variables, {
    bufferSlot = 0, -- does not matter if the var is global
    name = "GRAPHITE_VERSION",
    content = "v1.0",
    global = true,
    scope = "main" -- Function name
})


local function getVarByName(name, scope)
    for _, var in pairs(sectors[sectorId].sectorWriteAheads) do
        if var.bufferName == name then
            return var
        end
    end
end

local function getFunctionByName(name)
    for _, sector in pairs(sectors) do
        if sector.sectorName == name then
            return sector
        end
    end
end

local function getGlobal(name)
    for _, global in pairs(variables) do
        if global.global and global.name == name then
            return global
        end
    end
end

local function makeVar()
end

local function makeGlobal()
end

local function split(inputstr, sep)
    if sep == nil then
        sep = "%s"
    end
    local t = {}
    for str in string.gmatch(inputstr, "([^" .. sep .. "]+)") do
        table.insert(t, str)
    end
    return t
end

local function trim(s)
    return s:match("^%s*(.-)%s*$")
end

local function importStdEval(importFile)
    -- Loop over every line in imported file, find import statments, anmd
    local standardLibraryFile = io.open("std/" .. importFile .. ".gr", "r")

    if standardLibraryFile == nil then
        error("Invalid standard library module: " .. importFile)
    end

    local standardLibraryContent = standardLibraryFile:read("*all")
    standardLibraryFile:close()

    _G["importedCodeString"] = _G["importedCodeString"] .. "-- BEGIN STD IMPORT: " .. importFile .. "\n"
    for line in standardLibraryContent:gmatch("([^\n]*)\n?") do
        if line == "" then
            goto continue
        end

        local fullLine = line
        line = line:sub(2)
        local lineSplit = split(line, " ")

        if lineSplit[1] == "importStd" then
            lineSplit[2] = lineSplit[2]:sub(1, -2)
            importStdEval(lineSplit[2])
        else
            _G["importedCodeString"] = _G["importedCodeString"] .. "\n" .. fullLine
        end

        ::continue::
    end
    _G["importedCodeString"] = _G["importedCodeString"] .. "\n" .. "-- END STD IMPORT: " .. importFile .. "\n"
end

local function evalExpr(line)
    local isEscaped = false
    local statementContent = ""
    local isInStatment = false
    local tick = 0

    for i = 0, #line do
        local c = line:sub(i, i)

        if isEscaped and tick == 3 then
            isEscaped = false
            tick = 0
        end

        if isEscaped then
            tick = tick + 1
        end

        if c == '\\' then
            isEscaped = true
            tick = 1
        end



        if c == "$" and not isEscaped and not isInStatment then
            -- EVAL
            isInStatment = true
            statementContent = ""
        elseif c == "$" and not isEscaped and isInStatment then
            local global = getGlobal(statementContent)
            local scopeVar = getVarByName(statementContent)

            -- Global variables take preference over scope variables

            if global == nil and scopeVar == nil then
                print("Syntax error content: " .. line)
                error("Syntax error: variable specified in expression does not exist in the ENTIRE PROGRAM")
            end

            if global ~= nil then
                line = line:gsub("%$" .. statementContent .. "%$", global.content)
            else
                line = line:gsub("%$" .. statementContent .. "%$", "$#"..scopeVar.bufferSlot.."$")
            end

            isInStatment = false
        elseif isInStatment and not isEscaped then
            statementContent = statementContent .. c
        end
    end

    return line
end

print("Graphite language compiler v1.0")
print("Graphite virtual machine v1.0")
print("Graphite standard library v1.0")
print("Author: webmaster@interfiber.dev")
print("This compiler generates graphite virtual machine bytecode")
print("--------------------------------")

-- Read in file
print("Reading graphite code from disk...")

local grFile = io.open(file, "r")
local grFileCode = ""

if grFile == nil then
    error("Failed to open " .. file)
end

grFileCode = grFile:read("*all")

sectors[0] = {
    sectorContent = {},
    sectorWriteAheads = {},
    sectorId = 0,
    sectorName = "main"
}

print("Evaluating imports to generate full code...")

-- SECTION: Recursively eval import statements

_G["importedCodeString"] = ""
for line in grFileCode:gmatch("([^\n]*)\n?") do
    if line == "" then
        goto continue
    end

    local fullLine = line
    line = line:sub(1, -2)
    line = line:sub(2)
    local lineSplit = split(line, " ")

    if lineSplit[1] == "importStd" then
        importStdEval(lineSplit[2])
    else
        _G["importedCodeString"] = _G["importedCodeString"] .. "\n" .. fullLine
    end

    ::continue::
end

print(_G["importedCodeString"])

print("Generated full graphite code")
print("Compiling into graphite machine code")
for line in _G["importedCodeString"]:gmatch("([^\n]*)\n?") do
    if line == "" then
        goto continue
    end

    if line:sub(1, 1) .. line:sub(2, 2) == "--" then
        goto continue
    end

    local fullLine = line
    line = line:sub(2)
    line = line:sub(1, -2)
    local lineSplit = split(line, " ")

    -- Search for builtins

    if lineSplit[1] == "registerWrite" then
        table.remove(lineSplit, 1)

        local paramsCombined = table.concat(lineSplit, " ")
        local paramsSplit = split(paramsCombined, ",")

        local registerSlot = trim(paramsSplit[1])
        local registerValue = trim(paramsSplit[2])

        if registerSlot == nil or registerValue == nil then
            print("Syntax error content: " .. fullLine)
            error("Syntax error: the register value, or the register slot is not provided")
        end

        table.insert(sectors[sectorId].sectorContent, "REGWRITE-" .. registerSlot .. "," .. evalExpr(registerValue))
    elseif lineSplit[1] == "registerCopyToBuffer" then
        table.remove(lineSplit, 1)

        local paramsCombined = table.concat(lineSplit, " ")
        local paramsSplit = split(paramsCombined, ",")

        local registerSlot = trim(paramsSplit[1])
        local bufferSlot = trim(paramsSplit[2])

        if registerSlot == nil or bufferSlot == nil then
            print("Syntax error content: " .. fullLine)
            error("Syntax error: the register slot, or the buffer slot is not provided")
        end

        table.insert(sectors[sectorId].sectorContent, "REGCPYTOBUF-" .. registerSlot .. "," .. bufferSlot)
    elseif lineSplit[1] == "bufferCopyToRegister" then
        table.remove(lineSplit, 1)

        local paramsCombined = table.concat(lineSplit, " ")
        local paramsSplit = split(paramsCombined, ",")

        local bufferSlot = trim(paramsSplit[1])
        local registerSlot = trim(paramsSplit[2])

        if registerSlot == nil or bufferSlot == nil then
            print("Syntax error content: " .. fullLine)
            error("Syntax error: the register slot, or the buffer slot is not provided")
        end

        table.insert(sectors[sectorId].sectorContent, "BUFCPYTOREG-" .. bufferSlot .. "," .. registerSlot)
    elseif lineSplit[1] == "add" then
        table.remove(lineSplit, 1)

        local paramsCombined = table.concat(lineSplit, " ")
        local paramsSplit = split(paramsCombined, ",")

        local num1 = trim(paramsSplit[1])
        local num2 = trim(paramsSplit[2])

        if num1 == nil or num2 == nil then
            print("Syntax error content: " .. fullLine)
            error("Syntax error: number 1, or number 2 for ADD operation not provided")
        end

        table.insert(sectors[sectorId].sectorContent,
            "ADD-" .. evalExpr(tostring(num1)) .. "," .. evalExpr(tostring(num2)))
    elseif lineSplit[1] == "subtract" then
        table.remove(lineSplit, 1)

        local paramsCombined = table.concat(lineSplit, " ")
        local paramsSplit = split(paramsCombined, ",")

        local num1 = trim(paramsSplit[1])
        local num2 = trim(paramsSplit[2])

        if num1 == nil or num2 == nil then
            print("Syntax error content: " .. fullLine)
            error("Syntax error: number 1, or number 2 for SUB operation not provided")
        end

        table.insert(sectors[sectorId].sectorContent,
            "SUB-" .. evalExpr(tostring(num1)) .. "," .. evalExpr(tostring(num2)))
    elseif lineSplit[1] == "divide" then
        table.remove(lineSplit, 1)

        local paramsCombined = table.concat(lineSplit, " ")
        local paramsSplit = split(paramsCombined, ",")

        local num1 = trim(paramsSplit[1])
        local num2 = trim(paramsSplit[2])

        if num1 == nil or num2 == nil then
            print("Syntax error content: " .. fullLine)
            error("Syntax error: number 1, or number 2 for DIV operation not provided")
        end

        table.insert(sectors[sectorId].sectorContent,
            "DIV-" .. evalExpr(tostring(num1)) .. "," .. evalExpr(tostring(num2)))
    elseif lineSplit[1] == "multiply" then
        table.remove(lineSplit, 1)

        local paramsCombined = table.concat(lineSplit, " ")
        local paramsSplit = split(paramsCombined, ",")

        local num1 = trim(paramsSplit[1])
        local num2 = trim(paramsSplit[2])

        if num1 == nil or num2 == nil then
            print("Syntax error content: " .. fullLine)
            error("Syntax error: number 1, or number 2 for MUL operation not provided")
        end

        table.insert(sectors[sectorId].sectorContent,
            "MUL-" .. evalExpr(tostring(num1)) .. "," .. evalExpr(tostring(num2)))
    elseif lineSplit[1] == "tmpBufferCopy" then
        table.remove(lineSplit, 1)

        local paramsCombined = table.concat(lineSplit, " ")
        local paramsSplit = split(paramsCombined, ",")

        local tmpBufferSlot = trim(paramsSplit[1])
        local bufferSlot = trim(paramsSplit[2])

        if tmpBufferSlot == nil or bufferSlot == nil then
            print("Syntax error content: " .. fullLine)
            error("Syntax error: tmp buffer slot, or buffer slot not provided for temporary buffer copy operation")
        end

        table.insert(sectors[sectorId].sectorContent, "TMPBUFCPY-" .. tmpBufferSlot .. "," .. bufferSlot)
    elseif lineSplit[1] == "tmpBufferRemove" then
        table.remove(lineSplit, 1)

        local paramsCombined = table.concat(lineSplit, " ")
        local paramsSplit = split(paramsCombined, ",")

        local tmpBufferSlot = trim(paramsSplit[1])

        if tmpBufferSlot == nil then
            print("Syntax error content: " .. fullLine)
            error("Syntax error: tmp buffer slot not provided for temporary buffer remove operation")
        end

        table.insert(sectors[sectorId].sectorContent, "TMPBUFRM-" .. tmpBufferSlot)
    elseif lineSplit[1] == "function" then
        table.remove(lineSplit, 1)
        local paramsCombined = trim(table.concat(lineSplit, " "))
        local functionParamsString = split(paramsCombined, "::")[2]
        functionParamsString = functionParamsString:sub(2)
        functionParamsString = functionParamsString:sub(1, #functionParamsString-1)
        
        local functionParams = split(functionParamsString, ",")

        local sector = {
            sectorContent = {},
            sectorId = sectorId + 1,
            sectorWriteAheads = {},
            sectorName = split(paramsCombined, "::")[1]
        }
        
        -- Generate buffers

        local writeAheadBufferId = 0
        for _, param in pairs(functionParams) do
            -- Write Ahead Buffer allows writing into sector specific memory
            local writeAheadBuffer = {
                bufferSlot = writeAheadBufferId,
                bufferName = param,
                bufferContent = ""
            }

            table.insert(sector.sectorWriteAheads, writeAheadBuffer)
            table.insert(sector.sectorContent, "WABCPYTOBUF-"..writeAheadBuffer.bufferSlot..","..writeAheadBuffer.bufferSlot)

            writeAheadBufferId = writeAheadBufferId + 1
        end
        

        sectorId = sectorId + 1

        sectors[sectorId] = sector

        -- print(paramsCombined)
    elseif lineSplit[1] == "endfunction" then
        -- Delete the arguments from memory

        for _, param in pairs(sectors[sectorId].sectorWriteAheads) do
            table.insert(sectors[sectorId].sectorContent, "BUFRM-"..param.bufferSlot)
        end

        sectorId = sectorId - 1
    else
        -- Find function by name
        local func = getFunctionByName(lineSplit[1])
        
        if func ~= nil then
            table.remove(lineSplit, 1)
            
            local paramsCombined = table.concat(lineSplit, " ")
            local functionParams = split(paramsCombined, ",")

            -- Insert write aheads
            for _, wab in pairs(func.sectorWriteAheads) do
                table.insert(sectors[sectorId].sectorContent, "WABWRITE-"..func.sectorId..","..bufferId..","..functionParams[wab.bufferSlot + 1])
                bufferId = bufferId + 1
            end

            table.insert(sectors[sectorId].sectorContent, "GOTOSECTOR-"..func.sectorId)
        end
    end

    ::continue::
end

print("Assemblying sectors...")
local fullCode = ""

for i=0,#sectors,1 do
    local sector = sectors[i]
    local fullSectorCode = "#-#\n"
    for _, contentLine in pairs(sector.sectorContent) do
        fullSectorCode = fullSectorCode .. contentLine .. "\n"
    end
    fullSectorCode = fullSectorCode .. "#-#\n"
    fullCode = fullCode .. fullSectorCode
end

print("Writing machine code to ./output.grbc...")
local outputFile = io.open("output.grbc", "w+")

if outputFile == nil then
    error("Failed to open output file!")
end

outputFile:write(fullCode)
outputFile:close()

print("Compiler is complete, assembled code is located at ./output.grbc, as mentioned above")
