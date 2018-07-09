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

local CHAR_PLUS  = tobyte('+')
local CHAR_MINUS = tobyte('-')

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

    while c > 0 and c != CHAR_LINEFEED then
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

function parse(code)
    local state = makestate(code)

    local value = parsetable(state)
end

return {
    parse = parse;
}