-- Graphite language compiler
-- V1.0, this is different from the Graphite Virtual Machine

-- Parse graphite code into commands
-- When the command is an import, call the importEval() function, which will generate a blob of
-- graphite code
-- After imports are evaled, generate table of commands
-- Generate sector 0, which contains a GOTOSECTOR command that links to the main function
-- Loop over table of commands, and generate graphite bytecode

local class = require("lib.class")

local func = class.functionClass:new()