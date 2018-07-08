#ifndef __TIL_H__
#define __TIL_H__

#ifndef TIL_API
#define TIL_API
#endif

typedef enum
{
    TIL_NIL,
    TIL_TABLE,
    TIL_NUMBER,
    TIL_STRING,
    TIL_BOOLEAN,
} til_type_t;

typedef enum
{
    TIL_TRUE  = 1,
    TIL_FALSE = 0,
} til_bool_t;

typedef struct til_cell_t til_cell_t;

typedef struct til_value_t
{
    til_type_t type;

    union
    {
        double     number;
        til_bool_t boolean;

        struct
        {
            int                 length;
            struct til_value_t* values;
        } array;

        struct
        {
            int         length;
            til_cell_t* values;
        } table;

        struct
        {
            int   length;
            char* buffer;
        } string;
    };
} til_value_t;

struct til_cell_t
{
    til_value_t* name;
    til_value_t* value;
};

typedef struct til_state_t til_state_t;

TIL_API til_value_t* til_parse(const char* code, til_state_t** state);
TIL_API void         til_release(til_state_t* state);

#endif /* __TIL_H__ */

#ifdef TIL_IMPL

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

struct til_state_t
{
    int line;
    int column;
    int cursor;
    
    int         length;
    const char* buffer;
};

static til_state_t* make_state(const char* code)
{
    til_state_t* state = (til_state_t*)malloc(sizeof(til_state_t));
    if (state)
    {
        state->line   = 1;
        state->column = 1;
        state->cursor = 0;
        
        state->length = strlen(code);
        state->buffer = code;
    }
    return state;
}

static void free_state(til_state_t* state)
{
    if (state)
    {
        free(state);
    }
}

static int is_eof(til_state_t* state)
{
    return state->cursor >= state->length;
}

static int peek_char(til_state_t* state)
{
    if (is_eof(state))
    {
        return -1;
    }
    else
    {
        return state->buffer[state->cursor];
    }
}

static int next_char(til_state_t* state)
{
    if (is_eof(state))
    {
        return -1;
    }
    else
    {
        int c = state->buffer[++state->cursor];
        if (c == '\n')
        {
            state->line   += 1;
            state->column  = 1;
        }
        else
        {
            state->column += 1;
        }

        return c;
    }
}

static int skip_space(til_state_t* state)
{
    int c = peek_char(state);
    while (isspace(c))
    {
        c = next_char(state);
    }
    return c;
}

static int next_line(til_state_t* state)
{
    int c = peek_char(state);
    while (c > 0 && c != '\n')
    {
        c = next_char(state);
    }
    return  next_char(state);
}

static til_value_t* make_value(til_state_t* state, til_type_t type)
{
    til_value_t* value = (til_value_t*)malloc(sizeof(til_value_t));
    if (value)
    {
        value->type         = type;
        value->table.length = 0;
        value->table.values = NULL;
    }
    return value;
}

static void free_value(til_state_t* state, til_value_t* value)
{
    if (value)
    {
        free(value);
    }
}

static til_value_t* parse_table(til_state_t* state);
static til_value_t* parse_array(til_state_t* state);
static til_value_t* parse_number(til_state_t* state);
static til_value_t* parse_string(til_state_t* state);
static til_value_t* parse_single(til_state_t* state);
static til_value_t* parse_symbol(til_state_t* state);

static til_value_t* parse_symbol(til_state_t* state)
{
    if (isalpha(skip_space(state)) || peek_char(state) == '_')
    {

    }
    else
    {
        return NULL;
    }
}

static til_value_t* parse_table(til_state_t* state)
{
    if (skip_space(state) != '{')
    {
        return NULL;
    }
    else
    {
        next_char(state);
    }

    while (!(skip_space(state) <= 0 || peek_char(state) == '}'))
    {
        // Parse name
        til_value_t* name = NULL;
        int c = peek_char(state);
        if (isalpha(c))
        {                                      
            name = parse_symbol(state);
            assert(name->type == TIL_STRING);
        }
        else if (c == '[')
        {
            next_char(state);
            name = parse_string(state);
            assert(!name || name->type == TIL_STRING);
            
            if (!name)
            {
                return NULL;
            }
            else if (skip_space(state) != ']')
            {
                return NULL;
            }
            else
            {
                next_char(state);
            }
        }
        else
        {
            return NULL;
        }

        // Parse value
        til_value_t* value = parse_single(state);
        assert(value != NULL && "value cannot be null, checking parse_single()");

        
    }

    if (peek_char(state) != '}')
    {
        return NULL;
    }
    else
    {
        til_value_t* 
    }
}

til_value_t* til_parse(const char* code, til_state_t** old_state)
{
    til_state_t* state = make_state(code);
    if (!state)
    {
        return NULL;
    }

    if (skip_space(state) == '{')
    {
        return parse_table(code);
    }
    else
    {
        return NULL;
    }
}

/* END OF TIL_IMPL */
#endif /* TIL_IMPL */