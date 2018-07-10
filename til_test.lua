local til = require("til")

local value = til.parse([[
{
    nil_value = 10.123;
}
]])

print(til.stringify(value))