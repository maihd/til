#ifndef __TIL_H__
#define __TIL_H__

#ifndef TIL_API
#define TIL_API
#endif

#include <stdio.h>

typedef enum
{
    TIL_NIL,
    TIL_ARRAY,
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

typedef struct til_array_t
{
    int                 length;
    struct til_value_t* values;
} til_array_t;

typedef struct til_table_t
{
    int         length;
    til_cell_t* values;
} til_table_t;

typedef struct til_value_t
{
    til_type_t type;

    union
    {
        double      number;
        til_bool_t  boolean;
                    
        til_array_t array;
        til_table_t table;

        struct
        {
            int   length;
            char* buffer;
        } string;
    };
} til_value_t;

struct til_cell_t
{
    til_value_t name;
    til_value_t value;
};

typedef struct til_state_t til_state_t;

TIL_API til_value_t* til_parse(const char* code, til_state_t** state);
TIL_API void         til_release(til_state_t* state);

TIL_API void         til_print(const til_value_t* value, FILE* out);
TIL_API void         til_write(const til_value_t* value, FILE* out);

#endif /* __TIL_H__ */

#ifdef TIL_IMPL

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

typedef struct til_buffer_t 
{
    int count;
    int capacity;
    int elemsize;
} til_buffer_t;

/* @structdef: til_state_t */
struct til_state_t
{
    til_state_t* next;

    int line;
    int column;
    int cursor;
    
    int         length;
    const char* buffer;

    til_buffer_t value_buffers;
    til_buffer_t string_buffers;
};

static til_state_t* make_state(const char* code)
{
    til_state_t* state = (til_state_t*)malloc(sizeof(til_state_t));
    if (state)
    {
        state->next   = 0;

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
        free_state(state->next);
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

static til_value_t* skip_space_and_comment(til_state_t* state)
{
    if (skip_space(state) == '-')
    {
        if (!is_eof(state) && state->buffer[state->cursor + 1] == '-')
        {
            next_line(state);
        }
    }
    return  peek_char(state);
}

static til_value_t* parse_table(til_state_t* state);
static til_value_t* parse_array(til_state_t* state);
static til_value_t* parse_number(til_state_t* state);
static til_value_t* parse_string(til_state_t* state);
static til_value_t* parse_single(til_state_t* state);
static til_value_t* parse_symbol(til_state_t* state);

static til_value_t* parse_number(til_state_t* state)
{
    if (skip_space(state) < 0)
    {
		return NULL;
    }
    else
    {
		int c = peek_char(state);
		int sign = 1;
		
		if (c == '+')
		{
			c = next_char(state);
            return NULL;
			//croak(state, JSON_ERROR_UNEXPECTED,
			//	  "JSON does not support number start with '+'");
		}
		else if (c == '-')
		{
			sign = -1;
			c = next_char(state);
		}
		else if (c == '0')
		{
			c = next_char(state);
			if (!isspace(c) && !ispunct(c))
			{
                return NULL;
				//croak(state, JSON_ERROR_UNEXPECTED,
				//	  "JSON does not support number start with '0'"
				//	  " (only standalone '0' is accepted)");
			}
		}
		else if (!isdigit(c))
		{
            return NULL;
			//croak(state, JSON_ERROR_UNEXPECTED, "Unexpected '%c'", c);
		}

		int    dot    = 0;
		int    dotchk = 1;
		int    numpow = 1;
		double number = 0;

		while (c > 0)
		{
			if (c == '.')
			{
				if (dot)
				{
                    return NULL;
					//croak(state, JSON_ERROR_UNEXPECTED,
					//      "Too many '.' are presented");
				}

				if (!dotchk)
				{
					//croak(state, JSON_ERROR_UNEXPECTED, "Unexpected '%c'", c);
                    return NULL;
				}
				else
				{
					dot    = 1;
					dotchk = 0;
					numpow = 1;
				}
			}
			else if (!isdigit(c))
			{
				break;
			}
			else
			{
				dotchk = 1;
				if (dot)
				{
					numpow *= 10;
					number += (c - '0') / (double)numpow;
				}
				else
				{
					number = number * 10 + (c - '0');
				}
			}

			c = next_char(state);
		}

		if (dot && !dotchk)
		{
			//croak(state, JSON_ERROR_UNEXPECTED,
            //      "'.' is presented in number token, "
			//      "but require a digit after '.' ('%c')", c);
			return NULL;
		}
		else
		{
			til_value_t* value = make_value(state, TIL_NUMBER);
			value->number = sign * number;
			return value;
		}
    }
}

static til_value_t* parse_array(til_state_t* state)
{
    if (skip_space_and_comment(state) != '[')
    {
        return NULL;
    }
    else
    {
        next_char(state);
    }

    int          length = 0;
    til_value_t* values = NULL;
    while (!(skip_space_and_comment(state) <= 0 || peek_char(state) == ']'))
    {
        if (length > 0)
        {
            if (skip_space_and_comment(state) == ',')
            {
                next_char(state);
            }
            else
            {
                return NULL;
            }
        }

        // Parse value
        til_value_t* value = parse_single(state);
        assert(value != NULL && "value cannot be null, checking parse_single()");

        length = length + 1;
        values = realloc(values, length * sizeof(values[0]));

        values[length - 1] = *value;
    }

    if (peek_char(state) != ']')
    {
        return NULL;
    }
    else
    {
        next_char(state);

        til_value_t* value  = make_value(state, TIL_ARRAY);
        value->array.length = length;
        value->array.values = values;
        return value;
    }
}

static til_value_t* parse_single(til_state_t* state)
{
    if (skip_space_and_comment(state) > 0)
    {
        int c = peek_char(state);

        switch (c)
        {
        case '{':
            return parse_table(state);
            
        case '[':
            return parse_array(state);
            
        case '"':
            return parse_string(state);

        case '-': case '+': case '0':
        case '1': case '2': case '3':
        case '4': case '5': case '6':
        case '7': case '8': case '9':
            return parse_number(state);
        }

        if (isalpha(c))
        {
            int len = 1;
            int chr = next_char(state);
            while (isalnum(chr))
            {
                len = len + 1;
                chr = next_char(state);
            }

            const char* token = state->buffer + state->cursor - len;
            if (len == 3 && strncmp(token, "nil", len))
            {
                til_value_t* value = make_value(state, TIL_NIL);
                return value;
            }
            else if (len == 4 && strncmp(token, "true", len))
            {
                til_value_t* value = make_value(state, TIL_BOOLEAN);
                value->boolean = TIL_TRUE;
                return value;
            }
            else if (len == 5 && strncmp(token, "false", len))
            {
                til_value_t* value = make_value(state, TIL_BOOLEAN);
                value->boolean = TIL_FALSE;
                return value;
            }
            else
            {
                return NULL;
            }
        }
        else
        {
            return NULL;
        }
    }
    else
    {
        return NULL;
    }
}

static til_value_t* parse_string(til_state_t* state)
{
    if (skip_space_and_comment(state) != '"')
    {
        return NULL;
    }

    int len = 1;
    int chr = next_char(state);
    while (chr > 0 && chr != '"')
    {
        len = len + 1;
        chr = next_char(state);
    }

    if (chr != '"')
    {
        return NULL;
    }
    else
    {
        next_char(state);

        til_value_t* value = make_value(state, TIL_STRING);
        value->string.length = len;
        value->string.buffer = malloc((len + 1) * sizeof(char));
        
        value->string.buffer[len] = 0;
        memcpy(value->string.buffer, state->buffer + state->cursor - len, len);

        return value;
    }
}

static til_value_t* parse_symbol(til_state_t* state)
{
    if (isalpha(skip_space(state)) || peek_char(state) == '_')
    {
        int len = 1;
        int chr = next_char(state);
        while (isalnum(chr) || chr == '_')
        {
            len = len + 1;
            chr = next_char(state);
        }

        til_value_t* value   = make_value(state, TIL_STRING);
        value->string.length = len;
        value->string.buffer = malloc((len + 1) * sizeof(char));
        
        value->string.buffer[len] = 0;
        memcpy(value->string.buffer, state->buffer + state->cursor - len, len * sizeof(char));

        return value;
    }
    else
    {
        return NULL;
    }
}

static til_value_t* parse_table(til_state_t* state)
{
    if (skip_space_and_comment(state) != '{')
    {
        return NULL;
    }
    else
    {
        next_char(state);
    }

    int         length = 0;
    til_cell_t* values = NULL;
    while (!(skip_space_and_comment(state) <= 0 || peek_char(state) == '}'))
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

        if (skip_space_and_comment(state) == '=')
        {
            next_char(state);
        }
        else
        {
            return NULL;
        }

        // Parse value
        til_value_t* value = parse_single(state);
        assert(value != NULL && "value cannot be null, checking parse_single()");

        if (skip_space(state) == ';')
        {
            next_char(state);
        }
        else
        {
            return NULL;
        }

        length = length + 1;
        values = realloc(values, length * sizeof(til_cell_t));

        values[length - 1].name  = *name;
        values[length - 1].value = *value;
    }

    if (peek_char(state) != '}')
    {
        return NULL;
    }
    else
    {
        next_char(state);

        til_value_t* value  = make_value(state, TIL_TABLE);
        value->table.length = length;
        value->table.values = values;
        return value;
    }
}

static til_state_t* root_state = NULL;

/* @funcdef: til_parse */
til_value_t* til_parse(const char* code, til_state_t** out_state)
{
    til_state_t* state = make_state(code);
    if (!state)
    {
        return NULL;
    }

    if (skip_space_and_comment(state) == '{')
    {
        til_value_t* value = parse_table(state);
        if (value)
        {
            if (out_state)
            {
                *out_state = state;
            }
            else
            {
                if (root_state)
                {
                    root_state->next = state;
                }
                root_state = state;
            }
            return value;
        }
        else
        {
            if (out_state)
            {
                *out_state = NULL;
            }
            free_state(state);
            return NULL;
        }
    }
    else
    {
        return NULL;
    }
}

/* @funcdef: til_release */
void til_release(til_state_t* state)
{
    if (state)
    {
        free_state(state);
    }
    else
    {
        free_state(root_state);
        root_state = NULL;
    }
}

void til_print(const til_value_t* value, FILE* out)
{
    if (value)
    {
        int i, n;
        static int indent = 0;

        switch (value->type)
        {
        case TIL_NIL:
            fprintf(out, "null");
            break;

        case TIL_NUMBER:
            fprintf(out, "%lf", value->number);
            break;

        case TIL_BOOLEAN:
            fprintf(out, "%s", value->boolean ? "true" : "false");
            break;

        case TIL_STRING:
            fprintf(out, "\"%s\"", value->string.buffer);
            break;

        case TIL_ARRAY:
            fprintf(out, "[\n");

            indent++;
            for (i = 0, n = value->array.length; i < n; i++)
            {
                int j, m;
                for (j = 0, m = indent * 4; j < m; j++)
                {
                    fprintf(out, " ");
                }

                til_print(&value->array.values[i], out);
                if (i < n - 1)
                {
                    fprintf(out, ",");
                }
                fprintf(out, "\n");
            }
            indent--;

            for (i = 0, n = indent * 4; i < n; i++)
            {
                fprintf(out, " ");
            }
            fprintf(out, "]");
            break;

        case TIL_TABLE:
            fprintf(out, "{\n");

            indent++;
            for (i = 0, n = value->table.length; i < n; i++)
            {
                int j, m;
                for (j = 0, m = indent * 4; j < m; j++)
                {
                    fprintf(out, " ");
                }

                til_print(&value->table.values[i].name, out);
                fprintf(out, " = ");
                til_print(&value->table.values[i].value, out);
                fprintf(out, ";\n");
            }
            indent--;

            for (i = 0, n = indent * 4; i < n; i++)
            {
                fprintf(out, " ");
            }
            fprintf(out, "}");
            break;

        default:
            break;
        }
    }
}

void til_write(const til_value_t* value, FILE* out)
{
    if (value)
    {
        int i, n;
        static int indent = 0;

        switch (value->type)
        {
        case TIL_NIL:
            fprintf(out, "null");
            break;

        case TIL_NUMBER:
            fprintf(out, "%lf", value->number);
            break;

        case TIL_BOOLEAN:
            fprintf(out, "%s", value->boolean ? "true" : "false");
            break;

        case TIL_STRING:
            fprintf(out, "\"%s\"", value->string.buffer);
            break;

        case TIL_ARRAY:
            fprintf(out, "[\n");

            indent++;
            for (i = 0, n = value->array.length; i < n; i++)
            {
                int j, m;
                for (j = 0, m = indent * 4; j < m; j++)
                {
                    fprintf(out, " ");
                }

                til_write(&value->array.values[i], out);
                if (i < n - 1)
                {
                    fprintf(out, ",");
                }
                fprintf(out, "\n");
            }
            indent--;

            for (i = 0, n = indent * 4; i < n; i++)
            {
                fprintf(out, " ");
            }
            fprintf(out, "]");
            break;

        case TIL_TABLE:
            fprintf(out, "{\n");

            indent++;
            for (i = 0, n = value->table.length; i < n; i++)
            {
                int j, m;
                for (j = 0, m = indent * 4; j < m; j++)
                {
                    fprintf(out, " ");
                }

                til_write(&value->table.values[i].name, out);
                fprintf(out, " = ");
                til_write(&value->table.values[i].value, out);
                if (i < n - 1)
                {
                    fprintf(out, ";");
                }
                fprintf(out, "\n");
            }
            indent--;

            for (i = 0, n = indent * 4; i < n; i++)
            {
                fprintf(out, " ");
            }
            fprintf(out, "}");
            break;

        default:
            break;
        }
    }
}

/* END OF TIL_IMPL */
#endif /* TIL_IMPL */