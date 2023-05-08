-- Graphite language compiler
-- V1.0, this is different from the Graphite Virtual Machine

-- Parse graphite code into commands
-- When the command is an import, call the importEval() function, which will generate a blob of
-- graphite code
-- After imports are evaled, generate table of commands
-- Generate sector 0, which contains a GOTOSECTOR command that links to the main function
-- Loop over table of commands, and generate graphite bytecode

local file = "examples/hello_world.gr"
local sectorId = 0
local bufferId = 0
local sectors = {}
local commands = {}

local function split (inputstr, sep)
        if sep == nil then
                sep = "%s"
        end
        local t={}
        for str in string.gmatch(inputstr, "([^"..sep.."]+)") do
                table.insert(t, str)
        end
        return t
end

local function importEval(importFile)
    -- Loop over every line in imported file, find import statments, anmd 
    print("Importing standard library: "..importFile)
    local standardLibraryFile = io.open("std/"..importFile..".gr", "r+")

    if standardLibraryFile == nil then
        error("Invalid standard library module: "..importFile)
    end

    local standardLibraryContent = standardLibraryFile:read("a")
    standardLibraryFile:close()

    for line in standardLibraryContent:gmatch("([^\n]*)\n?") do
       if line == "" then
        goto continue
       end


        ::continue::
    end
end

local function parseLineIntoCommand(line)
end

print("Graphite language compiler v1.0")
print("Graphite virtual machine v1.0")
print("Graphite standard library v1.0")
print("Author: webmaster@interfiber.dev")
print("This compiler generates graphite virtual machine bytecode")
print("--------------------------------")

-- Read in file
print("Reading graphite code from disk...")

local grFile = io.open(file, "r+")
local grFileCode = ""
local fullGrCode = ""

if grFile == nil then
    error("Failed to open "..file)
end

grFileCode = grFile:read("a")

table.insert(sectors, {
    sectorContent = "",
    sectorId = 0
})
sectorId = sectorId + 1

print("Evaluating imports to generate full code...")

-- SECTION: Recursively eval import statements

for line in grFileCode:gmatch("([^\n]*)\n?") do
    print("Pre-processing line")

    if line == "" then
        print("!!!! Not pre-processing empty line")
        goto continue
    end
    
    line = line:sub(2)
    local lineSplit = split(line, " ")

    if lineSplit[1] == "importStd" then
        lineSplit[2] = lineSplit[2]:sub(1, -2)
        _G["importedCodeString"] = ""
        importEval(lineSplit[2])
    end

    ::continue::
end