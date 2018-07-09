--
--
--

--
-- Characters
--

function tobyte(character)
    return string.byte(character, 1)
end

local CHAR_BRACES_0 = tobyte('{')
local CHAR_BRACES_1 = tobyte('}')

local CHAR_BRACKS_0 = tobyte('[')
local CHAR_BRACKS_1 = tobyte(']')

local CHAR_COMMA     = tobyte(',')
local CHAR_SEMICOLON = tobyte(';')
local CHAR_QUOTATION = tobyte('"')

local CHAR_DOT   = tobyte('.')
local CHAR_PLUS  = tobyte('+')
local CHAR_MINUS = tobyte('-')
local CHAR_EQUAL = tobyte('=')

local CHAR_HYPHEN = tobyte('_')

local CHAR_TAB      = tobyte('\t')
local CHAR_SPACE    = tobyte('  ')
local CHAR_RETURN   = tobyte('\r')
local CHAR_LINEFEED = tobyte('\n')

local CHAR_0 = tobyte('0');
local CHAR_9 = tobyte('9');

local CHAR_A = tobyte('A');
local CHAR_Z = tobyte('Z');
local CHAR_a = tobyte('a');
local CHAR_z = tobyte('z'); 

function isdigit(character)
    return (character >= CHAR_0) and (character <= CHAR_9)
end

function isalpha(character)
    return
        (character >= CHAR_A and character <= CHAR_Z) or
        (character >= CHAR_a and character <= CHAR_z) 
end

function isalnum(character)
    return isdigit(character) or isalpha(character)
end

function isspace(character)
    return character == CHAR_TAB 
        or character == CHAR_SPACE
        or character == CHAR_RETURN
        or character == CHAR_LINEFEED
end

--
-- State management
--

function makestate(code)
    return {
        line   = 1;
        column = 1;
        cursor = 1;
        buffer = code;
    }
end

function iseof(state)
    return state.cursor > #state.buffer
end 

function peekchar(state)
    if iseof(state) then
        return -1
    else
        return string.byte(state.buffer, state.cursor)
    end
end

function nextchar(state)
    if iseof(state) then
        return -1
    else
        state.cursor = state.cursor + 1
        local c = string.byte(state.buffer, state.cursor)

        if c == CHAR_LINEFEED then
            state.line   = state.line + 1
            state.column = 1
        else
            state.column = state.column + 1
        end

        return c
    end
end

function skipspace(state)
    local c = peekchar(state)

    while isspace(c) do
        c = nextchar(c)
    end

    return c
end

function nextline(state)
    local c = nextchar(state)

    while c > 0 and c ~= CHAR_LINEFEED do
        c = nextchar(state)
    end

    return nextchar(state)
end

function skipredundant(state)
    if skipspace(state) == CHAR_MINUS then
        if string.byte(state.buffer, state.cursor + 1) == CHAR_MINUS then
            nextline(state)
        end
    end

    return peekchar(state)
end

--
-- Parsing
--

function parsenumber(state)
    local chr = skipredundant(state)
    local sign = 1
    if chr == CHAR_0 then
        chr = string.byte(state.buffer, state.cursor)
        if isnumber(chr) then
            error("Number starting with '0' is not supported")
        elseif chr ~= CHAR_DOT then
            return 0
        end
    elseif chr == CHAR_PLUS then 
        error("Number statting with '+' is not supported")
    elseif chr == CHAR_MINUS then
        sign = -1
        chr = nextchar(state)
    else
        error("Unexpected token")
    end
        
    local num = 0
    local pow = 1
    local dot = false
    local dotchk = true

    while true do
        if c == CHAR_DOT then
            if dot then
                error("Dot is presented")
            elseif dotchk then 
                error("Before dot must be a digit")
            else
                dot    = true
                dotchk = true
            end
        elseif isdigit(c) then
            dotchk = false
            if dot then
                num = num * 10 + chr - CHAR_0
            else
                num = num + (chr - CHAR_0) / pow
            end
        else
            break
        end

        chr = nextchars(state)
    end

    if dotchk then
        error("After dot must be a digit")
    end

    return num
end

function parsestring(state)
    if skipredundant(state) ~= CHAR_QUOTATION then 
        return nil
    else
        local len = 0
        local chr = nextchar(state)
        
        while chr > 0 and chr ~= CHAR_QUOTATION do
            len = len + 1
            chr = nextchar(state)
        end

        if chr ~= CHAR_QUOTATION then
            error("Unterminated string")
        end

        return string.sub(state.buffer, state.cursor - len, state.cursor)
    end
end

function parsearray(state)
    if skipredundant(state) ~= CHAR_BRACKS_0 then
        return nil
    else
        local result = {}
        while skipredundant(state) > 0 and peekchar(state) ~= CHAR_BRACKS_1 do
            if #result > 0 then
                if peekchar(state) ~= CHAR_COMMA then
                    error("Require ',' to seperated table value")
                else
                    nextchar(state)
                end
            end

            local value = parsesingle(state)
            if value == nil then
                break
            end
            
            result[#result] = value
        end

        if peekchar(state) ~= CHAR_BRACKS_1 then
            error("Unterminated array")
        else
            nextchar(state)
            return result
        end
    end
end

function parsetable(state)
    if skipredundant(state) ~= CHAR_BRACES_0 then
        return nil
    else
        local result = {}
        while skipredundant(state) > 0 and peekchar(state) ~= CHAR_BRACES_1 do
            local name
            if isalpha(peekchar(state)) then
                name = parsesymbol(state)
            elseif peekchar(state) == CHAR_BRACKS_0 then 
                nextchar(state)

                name = parsestring(state)

                if peekchar(state) == CHAR_BRACKS_1 then
                    nextchar(state)
                else
                    error("Unterninated ']'")
                end
            else
                error("Unexpected in name of table")
            end

            if skipredundant(state) == CHAR_EQUAL then
                nextchar(state)
            else
                error("Require '=' to separate name and value")
            end

            local value = parsesingle(state)
            
            if skipredundant(state) == CHAR_SEMICOLON then
                nextchar(state)
            else
                error("Require ';' to end name-value pair of table")
            end

            result[name] = value
        end

        if peekchar(state) ~= CHAR_BRACES_1 then
            error("Unterminated table")
        else
            nextchar(state)
            return result
        end
    end
end 

function parsesingle(state)
    if skipredundant(state) <= 0 then
        return nil
    else
        local c = peekchar(state)

        if c == CHAR_BRACES_0 then
            return parsetable(state)
        elseif c == CHAR_BRACKS_1 then
            return parsearray(state)
        elseif c == CHAR_QUOTATION then
            return parsestring(state)
        elseif c == CHAR_PLUS or c == CHAR_MINUS or isdigit(c) then
            
        elseif isalpha(c) then
            local len = 1
            while isalpha(c) do
                len = len + 1
                c = nextchar(state)
            end 

            local str = string.sub(state.buffer, state.cursor - len, state.cursor)
            if str == "nil" then
                return nil
            elseif str == "true" then
                return true
            elseif str == "false" then
                return false
            else
                error("Unknown token")
            end
        else
            return nil
        end 
    end 
end

function parse(code)
    local state = makestate(code)

    if skipredundant(state) == CHAR_BRACES_0 then
        local value = parsetable(state)

        return value
    else
        return nil
    end
end

return {
    parse = parse;
}