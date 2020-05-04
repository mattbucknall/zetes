/*
 * JSON library for resource constrained systems, written in C99.
 *
 * This is free and unencumbered software released into the public domain.
 *
 * Anyone is free to copy, modify, publish, use, compile, sell, or
 * distribute this software, either in source code form or as a compiled
 * binary, for any purpose, commercial or non-commercial, and by any
 * means.
 *
 * In jurisdictions that recognize copyright laws, the author or authors
 * of this software dedicate any and all copyright interest in the
 * software to the public domain. We make this dedication for the benefit
 * of the public at large and to the detriment of our heirs and
 * successors. We intend this dedication to be an overt act of
 * relinquishment in perpetuity of all present and future rights to this
 * software under copyright law.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
 * IN NO EVENT SHALL THE AUTHORS BE LIABLE FOR ANY CLAIM, DAMAGES OR
 * OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
 * ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
 * OTHER DEALINGS IN THE SOFTWARE.
 *
 * For more information, please refer to <http://unlicense.org/>
 */

#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "zetes.h"


typedef enum {
	TOKEN_TYPE_UNDEFINED,
	TOKEN_TYPE_END_OF_INPUT,
	TOKEN_TYPE_KEY_VAL_SEPARATOR,
	TOKEN_TYPE_COMMA,
	TOKEN_TYPE_OBJECT_OPEN,
	TOKEN_TYPE_OBJECT_CLOSE,
	TOKEN_TYPE_ARRAY_OPEN,
	TOKEN_TYPE_ARRAY_CLOSE,
	TOKEN_TYPE_LITERAL
} token_type_t;


typedef struct {
	zetes_t* ctx;
	zetes_write_func_t write_func;
	void* user_data;
} wstate_t;


typedef struct {
	zetes_t* ctx;
	zetes_read_func_t read_func;
	char* buffer_i;
	char* buffer_e;
	char prev_char;
	void* user_data;
	token_type_t token_type;
	zetes_value_t token_value;
} rstate_t;


static const char SYMBOL_KEY_VAL_SEPARATOR[1] = 	{':'};
static const char SYMBOL_COMMA[1] = 				{','};
static const char SYMBOL_OBJECT_OPEN[1] = 			{'{'};
static const char SYMBOL_OBJECT_CLOSE[1] = 			{'}'};
static const char SYMBOL_ARRAY_OPEN[1] = 			{'['};
static const char SYMBOL_ARRAY_CLOSE[1] = 			{']'};
static const char SYMBOL_STRING_DELIM[1] = 			{'"'};
static const char SYMBOL_NULL[4] =					{'n', 'u', 'l', 'l'};
static const char SYMBOL_FALSE[5] =					{'f', 'a', 'l', 's', 'e'};
static const char SYMBOL_TRUE[4] =					{'t', 'r', 'u', 'e'};


static bool write_value(wstate_t* state, const zetes_value_t* value);

static void parse_value(rstate_t* state);


static void set_error(zetes_t* ctx, zetes_result_t result) {
	if ( ctx->result == ZETES_RESULT_OK ) {
		ctx->result = result;
	}
}


static void* align_ptr(void* ptr) {
	return (void*) (((((uintptr_t) ptr) + (ZETES_ALIGN - 1)) / (ZETES_ALIGN)) * ZETES_ALIGN);
}


static void* alloc(zetes_t* ctx, size_t size) {
	size_t space = ctx->buffer_end - ctx->buffer_ptr;
	void* ptr;

	if ( space < size ) {
		set_error(ctx, ZETES_RESULT_OUT_OF_MEMORY);
		ptr = NULL;
	} else {
		ptr = ctx->buffer_ptr;
		ctx->buffer_ptr = align_ptr(ctx->buffer_ptr + size);
	}

	return ptr;
}


zetes_result_t zetes_init(zetes_t* ctx, size_t stack_depth, void* buffer, size_t buffer_size) {
	ZETES_ASSERT(ctx);
	ZETES_ASSERT(mem_funcs);
	ZETES_ASSERT(stack_depth >= 2);

	ctx->result = ZETES_RESULT_OK;
	ctx->buffer_begin = buffer;
	ctx->buffer_end = buffer + buffer_size;
	ctx->buffer_ptr = align_ptr(buffer);
	ctx->stack_begin = (zetes_value_t*) alloc(ctx, stack_depth * sizeof(zetes_value_t));
	ctx->stack_end = ctx->stack_begin + stack_depth;
	ctx->stack_ptr = ctx->stack_end;

	return ctx->result;
}


void zetes_cleanup(zetes_t* ctx) {
	ZETES_ASSERT(ctx);
	ZETES_ASSERT(ctx->result != ZETES_RESULT_UNINITIALIZED);

	ctx->result = ZETES_RESULT_UNINITIALIZED;
	ctx->buffer_begin = NULL;
	ctx->buffer_end = NULL;
	ctx->buffer_ptr = NULL;
	ctx->stack_begin = NULL;
	ctx->stack_end = NULL;
	ctx->stack_ptr = NULL;
}


void zetes_reset(zetes_t* ctx) {
	ZETES_ASSERT(ctx);
	ZETES_ASSERT(ctx->result != ZETES_RESULT_UNINITIALIZED);

	ctx->result = ZETES_RESULT_OK;
	ctx->buffer_ptr = ctx->buffer_begin;
	ctx->stack_ptr = ctx->stack_end;
}


zetes_result_t zetes_result(zetes_t* ctx) {
	ZETES_ASSERT(ctx);
	ZETES_ASSERT(ctx->result != ZETES_RESULT_UNINITIALIZED);

	return ctx->result;
}


static bool ok(const zetes_t* ctx) {
	return ctx->result == ZETES_RESULT_OK;
}


static zetes_value_t* stack_emplace(zetes_t* ctx) {
	ZETES_ASSERT(ctx);
	ZETES_ASSERT(ctx->result != ZETES_RESULT_UNINITIALIZED);

	zetes_value_t* slot = NULL;

	if ( ok(ctx) ) {
		if (ctx->stack_ptr > ctx->stack_begin) {
			slot = --(ctx->stack_ptr);
		} else {
			set_error(ctx, ZETES_RESULT_STACK_FULL);
		}
	}

	return slot;
}


static bool stack_validate(zetes_t* ctx, size_t min_depth) {
	bool result = false;

	if ( ok(ctx) ) {
		if ( (ctx->stack_end - ctx->stack_ptr) >= min_depth ) {
			result = true;
		} else {
			set_error(ctx, ZETES_RESULT_STACK_EMPTY);
		}
	}

	return result;
}


static bool type_validate(zetes_t* ctx, zetes_type_t expected, zetes_type_t actual) {
	if ( expected == actual ) {
		return true;
	} else {
		set_error(ctx, ZETES_RESULT_TYPE_MISMATCH);
		return false;
	}
}


void zetes_push_null(zetes_t* ctx) {
	ZETES_ASSERT(ctx);
	ZETES_ASSERT(ctx->result != ZETES_RESULT_UNINITIALIZED);

	zetes_value_t* slot = stack_emplace(ctx);

	if ( slot ) {
		slot->type = ZETES_TYPE_NULL;
	}
}


void zetes_push_bool(zetes_t* ctx, bool value) {
	ZETES_ASSERT(ctx);
	ZETES_ASSERT(ctx->result != ZETES_RESULT_UNINITIALIZED);

	zetes_value_t* slot = stack_emplace(ctx);

	if ( slot ) {
		slot->type = ZETES_TYPE_BOOL;
		slot->variant._bool = value;
	}
}


void zetes_push_number(zetes_t* ctx, zetes_number_t value) {
	ZETES_ASSERT(ctx);
	ZETES_ASSERT(ctx->result != ZETES_RESULT_UNINITIALIZED);

	zetes_value_t* slot = stack_emplace(ctx);

	if ( slot ) {
		slot->type = ZETES_TYPE_NUMBER;
		slot->variant._number = value;
	}
}


void zetes_push_string(zetes_t* ctx, const char* value) {
	ZETES_ASSERT(ctx);
	ZETES_ASSERT(ctx->result != ZETES_RESULT_UNINITIALIZED);
	ZETES_ASSERT(value);

	zetes_value_t* slot = stack_emplace(ctx);

	if ( slot ) {
		size_t len = strlen(value) + 1;
		char* str = (char*) alloc(ctx, len);

		if ( str ) {
			memcpy(str, value, len);
			slot->type = ZETES_TYPE_STRING;
			slot->variant._string = str;
		}
	}
}


void zetes_push_new_array(zetes_t* ctx) {
	ZETES_ASSERT(ctx);
	ZETES_ASSERT(ctx->result != ZETES_RESULT_UNINITIALIZED);
	ZETES_ASSERT(value);

	zetes_value_t* slot = stack_emplace(ctx);

	if ( slot ) {
		zetes_array_t* array = (zetes_array_t*) alloc(ctx, sizeof(zetes_array_t));

		if ( array ) {
			array->first = NULL;
			array->last = NULL;
			slot->type = ZETES_TYPE_ARRAY;
			slot->variant._array = array;
		}
	}
}


void zetes_push_new_object(zetes_t* ctx) {
	ZETES_ASSERT(ctx);
	ZETES_ASSERT(ctx->result != ZETES_RESULT_UNINITIALIZED);
	ZETES_ASSERT(value);

	zetes_value_t* slot = stack_emplace(ctx);

	if ( slot ) {
		zetes_object_t* object = (zetes_object_t*) alloc(ctx, sizeof(zetes_object_t));

		if ( object ) {
			object->first = NULL;
			object->last = NULL;
			slot->type = ZETES_TYPE_OBJECT;
			slot->variant._object = object;
		}
	}
}


zetes_type_t zetes_type(zetes_t* ctx) {
	ZETES_ASSERT(ctx);
	ZETES_ASSERT(ctx->result != ZETES_RESULT_UNINITIALIZED);

	zetes_type_t type = ZETES_TYPE_NONE;

	if ( stack_validate(ctx, 1) ) {
		type = ctx->stack_ptr->type;
	}

	return type;
}


void zetes_pop(zetes_t* ctx) {
	ZETES_ASSERT(ctx);
	ZETES_ASSERT(ctx->result != ZETES_RESULT_UNINITIALIZED);

	if ( stack_validate(ctx, 1) ) {
		ctx->stack_ptr++;
	}
}


void zetes_pop_null(zetes_t* ctx) {
	ZETES_ASSERT(ctx);
	ZETES_ASSERT(ctx->result != ZETES_RESULT_UNINITIALIZED);

	if ( stack_validate(ctx, 1) && type_validate(ctx, ZETES_TYPE_NULL, ctx->stack_ptr->type) ) {
		ctx->stack_ptr++;
	}
}


bool zetes_pop_bool(zetes_t* ctx) {
	ZETES_ASSERT(ctx);
	ZETES_ASSERT(ctx->result != ZETES_RESULT_UNINITIALIZED);

	bool value = false;

	if ( stack_validate(ctx, 1) && type_validate(ctx, ZETES_TYPE_BOOL, ctx->stack_ptr->type) ) {
		value = ctx->stack_ptr->variant._bool;
		ctx->stack_ptr++;
	}

	return value;
}


zetes_number_t zetes_pop_number(zetes_t* ctx) {
	ZETES_ASSERT(ctx);
	ZETES_ASSERT(ctx->result != ZETES_RESULT_UNINITIALIZED);

	zetes_number_t value = 0;

	if ( stack_validate(ctx, 1) && type_validate(ctx, ZETES_TYPE_NUMBER, ctx->stack_ptr->type) ) {
		value = ctx->stack_ptr->variant._number;
		ctx->stack_ptr++;
	}

	return value;
}


const char* zetes_pop_string(zetes_t* ctx) {
	ZETES_ASSERT(ctx);
	ZETES_ASSERT(ctx->result != ZETES_RESULT_UNINITIALIZED);

	const char* value = NULL;

	if ( stack_validate(ctx, 1) && type_validate(ctx, ZETES_TYPE_STRING, ctx->stack_ptr->type) ) {
		value = ctx->stack_ptr->variant._string;
		ctx->stack_ptr++;
	}

	return value;
}


size_t zetes_array_size(zetes_t* ctx) {
	ZETES_ASSERT(ctx);
	ZETES_ASSERT(ctx->result != ZETES_RESULT_UNINITIALIZED);

	size_t size = 0;

	if ( stack_validate(ctx, 1) && type_validate(ctx, ZETES_TYPE_ARRAY, ctx->stack_ptr->type) ) {
		zetes_array_t* array = ctx->stack_ptr->variant._array;
		zetes_array_element_t* element = array->first;

		while (element) {
			element = element->next;
			size++;
		}
	}

	return size;
}


void zetes_array_index(zetes_t* ctx, size_t index) {
	ZETES_ASSERT(ctx);
	ZETES_ASSERT(ctx->result != ZETES_RESULT_UNINITIALIZED);

	if ( stack_validate(ctx, 1) && type_validate(ctx, ZETES_TYPE_ARRAY, ctx->stack_ptr->type) ) {
		zetes_array_t* array = ctx->stack_ptr->variant._array;
		zetes_array_element_t* element = array->first;

		while (element && index) {
			index--;
		}

		if ( element ) {
			zetes_value_t* slot = stack_emplace(ctx);

			if ( slot ) {
				*slot = element->value;
			}
		} else {
			set_error(ctx, ZETES_RESULT_INDEX_OUT_OF_BOUNDS);
		}
	}
}


void zetes_array_append(zetes_t* ctx) {
	ZETES_ASSERT(ctx);
	ZETES_ASSERT(ctx->result != ZETES_RESULT_UNINITIALIZED);

	zetes_value_t* array_value = ctx->stack_ptr + 1;

	if (stack_validate(ctx, 2) && type_validate(ctx, ZETES_TYPE_ARRAY, array_value->type)) {
		zetes_array_t* array = array_value->variant._array;
		zetes_array_element_t* element = (zetes_array_element_t*) alloc(ctx, sizeof(zetes_array_element_t));

		if ( element ) {
			element->next = NULL;
			element->value = *(ctx->stack_ptr++);

			if ( array->last ) {
				array->last->next = element;
			} else {
				array->first = element;
			}

			array->last = element;
		}
	}
}


size_t zetes_object_size(zetes_t* ctx) {
	ZETES_ASSERT(ctx);
	ZETES_ASSERT(ctx->result != ZETES_RESULT_UNINITIALIZED);

	size_t size = 0;

	if ( stack_validate(ctx, 1) && type_validate(ctx, ZETES_TYPE_ARRAY, ctx->stack_ptr->type) ) {
		zetes_object_t* object = ctx->stack_ptr->variant._object;
		zetes_object_member_t* member = object->first;

		while (member) {
			member = member->next;
			size++;
		}
	}

	return size;
}


void zetes_object_index(zetes_t* ctx, size_t index) {
	ZETES_ASSERT(ctx);
	ZETES_ASSERT(ctx->result != ZETES_RESULT_UNINITIALIZED);

	if ( stack_validate(ctx, 1) && type_validate(ctx, ZETES_TYPE_ARRAY, ctx->stack_ptr->type) ) {
		zetes_object_t* object = ctx->stack_ptr->variant._object;
		zetes_object_member_t* member = object->first;

		while (member && index) {
			member = member->next;
			index--;
		}

		if ( member ) {
			zetes_value_t* slot = stack_emplace(ctx);

			if ( slot ) {
				*slot = member->value;
			}

			slot = stack_emplace(ctx);

			if ( slot ) {
				slot->type = ZETES_TYPE_STRING;
				slot->variant._string = member->key;
			}
		} else {
			set_error(ctx, ZETES_RESULT_INDEX_OUT_OF_BOUNDS);
		}
	}
}


bool zetes_object_has(zetes_t* ctx, const char* key) {
	ZETES_ASSERT(ctx);
	ZETES_ASSERT(ctx->result != ZETES_RESULT_UNINITIALIZED);
	ZETES_ASSERT(key);

	bool result = false;

	if ( stack_validate(ctx, 1) && type_validate(ctx, ZETES_TYPE_OBJECT, ctx->stack_ptr->type) ) {
		zetes_object_t* object = ctx->stack_ptr->variant._object;
		zetes_object_member_t* member = object->first;

		while (member) {
			if ( strcmp(member->key, key) == 0 ) {
				result = true;
				break;
			}

			member = member->next;
		}
	}

	return result;
}



void zetes_object_get(zetes_t* ctx, const char* key) {
	ZETES_ASSERT(ctx);
	ZETES_ASSERT(ctx->result != ZETES_RESULT_UNINITIALIZED);
	ZETES_ASSERT(key);

	if ( stack_validate(ctx, 1) && type_validate(ctx, ZETES_TYPE_OBJECT, ctx->stack_ptr->type) ) {
		zetes_object_t* object = ctx->stack_ptr->variant._object;
		zetes_object_member_t* member = object->first;

		while (member) {
			if ( strcmp(member->key, key) == 0 ) {
				break;
			}

			member = member->next;
		}

		if ( member ) {
			zetes_value_t* slot = stack_emplace(ctx);

			if ( slot ) {
				*slot = member->value;
			}
		} else {
			set_error(ctx, ZETES_RESULT_KEY_NOT_FOUND);
		}
	}
}


void zetes_object_set(zetes_t* ctx, const char* key) {
	ZETES_ASSERT(ctx);
	ZETES_ASSERT(ctx->result != ZETES_RESULT_UNINITIALIZED);
	ZETES_ASSERT(key);

	zetes_value_t* object_value = ctx->stack_ptr + 1;

	if ( stack_validate(ctx, 2) && type_validate(ctx, ZETES_TYPE_OBJECT, object_value->type) ) {
		zetes_object_t* object = object_value->variant._object;
		zetes_object_member_t* member = object->first;

		while (member) {
			if ( strcmp(member->key, key) == 0 ) {
				break;
			}

			member = member->next;
		}

		if ( member ) {
			member->value = *(ctx->stack_ptr++);
		} else {
			size_t key_len = strlen(key) + 1;
			char* new_key = (char*)alloc(ctx, key_len);

			if (new_key) {
				memcpy(new_key, key, key_len);

				member = (zetes_object_member_t*)alloc(ctx, sizeof(zetes_object_member_t));

				if (member) {
					member->next = NULL;
					member->key = new_key;
					member->value = *(ctx->stack_ptr++);

					if (object->last) {
						object->last->next = member;
					} else {
						object->first = member;
					}

					object->last = member;
				}
			}
		}
	}
}


static void object_set_with_interned_key(zetes_t* ctx, const char* key) {
	ZETES_ASSERT(ctx);
	ZETES_ASSERT(ctx->result != ZETES_RESULT_UNINITIALIZED);
	ZETES_ASSERT(key);

	zetes_value_t* object_value = ctx->stack_ptr + 1;

	if ( stack_validate(ctx, 2) && type_validate(ctx, ZETES_TYPE_OBJECT, object_value->type) ) {
		zetes_object_t* object = object_value->variant._object;
		zetes_object_member_t* member = object->first;

		while (member) {
			if ( strcmp(member->key, key) == 0 ) {
				break;
			}

			member = member->next;
		}

		if ( member ) {
			member->value = *(ctx->stack_ptr++);
		} else {
			member = (zetes_object_member_t*)alloc(ctx, sizeof(zetes_object_member_t));

			if (member) {
				member->next = NULL;
				member->key = key;
				member->value = *(ctx->stack_ptr++);

				if (object->last) {
					object->last->next = member;
				} else {
					object->first = member;
				}

				object->last = member;
			}
		}
	}
}


static bool write_all(wstate_t* state, const char* buffer, size_t length) {
	const char* buffer_i = buffer;
	const char* buffer_e = buffer + length;

	while(buffer_i < buffer_e) {
		int result = state->write_func(buffer_i, buffer_e - buffer_i, state->user_data);

		if ( result < 0 ) {
			set_error(state->ctx, ZETES_RESULT_WRITE_ERROR);
			return false;
		} else {
			buffer_i += result;
		}
	}

	return true;
}


static bool write_bool(wstate_t* state, bool value) {
	if ( value ) {
		return write_all(state, SYMBOL_TRUE, sizeof(SYMBOL_TRUE));
	} else {
		return write_all(state, SYMBOL_FALSE, sizeof(SYMBOL_FALSE));
	}
}


static bool write_number(wstate_t* state, zetes_number_t value) {
	int len;

	len = snprintf(state->ctx->temp, ZETES_TEMP_BUFFER_SIZE, "%.9g", value);
	return write_all(state, state->ctx->temp, len);
}


static bool write_escape_code(wstate_t* state, uint16_t code) {
	static const char HEX_LUT[] = {'0', '1', '2', '3', '4', '5', '6', '7', '8', '9','A', 'B', 'C', 'D', 'E', 'F'};

	char* temp = state->ctx->temp;

	temp[0] = '\\';
	temp[1] = 'u';
	temp[2] = HEX_LUT[(code >> 12U) & 0xF];
	temp[3] = HEX_LUT[(code >> 8U) & 0xF];
	temp[4] = HEX_LUT[(code >> 4U) & 0xF];
	temp[5] = HEX_LUT[(code >> 0U) & 0xF];

	return write_all(state, temp, 6);
}


static bool escaped(uint8_t c) {
	return (c < ' ') || (c > '~') || (c == '/') || (c == '"') || (c == '\\') || (c == '\'');
}


static int decode_utf8(const char* input_i, const char* input_e, uint16_t* code) {
	// based on code placed in the public domain by Jeff Bezanson, 2005.

	static const uint32_t OFFSETS[6] = {
		0x00000000UL, 0x00003080UL, 0x000E2080UL,
		0x03C82080UL, 0xFA082080UL, 0x82082080UL
	};

	static const int8_t TRAILING[256] = {
		0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
		0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
		0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
		0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
		0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
		0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
		1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1, 1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
		2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2, 3,3,3,3,3,3,3,3,4,4,4,4,5,5,5,5
	};

	int8_t trailing = -1;

	if ( input_i < input_e ) {
		uint16_t cp = 0;

		trailing = TRAILING[(uint8_t) (*input_i)];

		if ( input_i + trailing >= input_e ) {
			return -1;
		}

		switch(trailing) {
		case 3:		cp += (uint8_t) (*input_i++); cp <<= 6U;		// fall through
		case 2:		cp += (uint8_t) (*input_i++); cp <<= 6U;		// fall through
		case 1:		cp += (uint8_t) (*input_i++); cp <<= 6U;		// fall through
		case 0:		cp += (uint8_t) (*input_i++);
		default:	break;
		}

		cp -= OFFSETS[trailing];

		*code = cp;
	}

	return trailing + 1;
}


static bool write_string(wstate_t* state, const char* value) {
	const char* str_i = value;
	const char* str_e = str_i + strlen(value);
	const char* str_n = str_i;
	uint16_t code;

	if ( !write_all(state, SYMBOL_STRING_DELIM, sizeof(SYMBOL_STRING_DELIM)) ) {
		return false;
	}

	while (str_i < str_e) {
		do {
			if ( !escaped(*str_n) ) {
				str_n++;
			} else {
				break;
			}
		} while (str_n < str_e);

		if (str_i < str_n) {
			if (!write_all(state, str_i, str_n - str_i)) {
				return false;
			}
		}

		str_i = str_n;

		if (str_i < str_e) {
			bool result;
			int len = 1;

			code = *str_i;

			switch(code) {
			case '"':	result = write_all(state, "\\\"", 2);		break;
			case '\\':	result = write_all(state, "\\\\", 2);		break;
			case '/':	result = write_all(state, "\\/", 2);		break;
			case '\b':	result = write_all(state, "\\b", 2);		break;
			case '\f':	result = write_all(state, "\\f", 2);		break;
			case '\n':	result = write_all(state, "\\n", 2);		break;
			case '\r':	result = write_all(state, "\\r", 2);		break;
			case '\t':	result = write_all(state, "\\t", 2);		break;

			default:
				len = decode_utf8(str_i, str_e, &code);
				result = (len >= 0) && write_escape_code(state, code);
			}

			if ( !result ) return false;

			str_i += len;
		}

		str_n = str_i;
	}

	return write_all(state, SYMBOL_STRING_DELIM, sizeof(SYMBOL_STRING_DELIM));
}


static bool write_array(wstate_t* state, const zetes_array_t* array) {
	const zetes_array_element_t* element = array->first;

	if ( !write_all(state, SYMBOL_ARRAY_OPEN, sizeof(SYMBOL_ARRAY_OPEN)) ) {
		return false;
	}

	if (element) {
		if ( !write_value(state, &(element->value)) ) {
			return false;
		}

		element = element->next;
	}

	while (element) {
		if ( !write_all(state, SYMBOL_COMMA, sizeof(SYMBOL_COMMA)) ) {
			return false;
		}

		if ( !write_value(state, &(element->value)) ) {
			return false;
		}

		element = element->next;
	}

	return write_all(state, SYMBOL_ARRAY_CLOSE, sizeof(SYMBOL_ARRAY_CLOSE));
}


static bool write_member(wstate_t* state, const zetes_object_member_t* member) {
	if ( !write_string(state, member->key) ) {
		return false;
	}

	if ( !write_all(state, SYMBOL_KEY_VAL_SEPARATOR, sizeof(SYMBOL_KEY_VAL_SEPARATOR)) ) {
		return false;
	}

	return write_value(state, &member->value);
}


static bool write_object(wstate_t* state, const zetes_object_t* object) {
	const zetes_object_member_t* member = object->first;

	if ( !write_all(state, SYMBOL_OBJECT_OPEN, sizeof(SYMBOL_OBJECT_OPEN)) ) {
		return false;
	}

	if (member) {
		if ( !write_member(state, member) ) {
			return false;
		}

		member = member->next;
	}

	while (member) {
		if ( !write_all(state, SYMBOL_COMMA, sizeof(SYMBOL_COMMA)) ) {
			return false;
		}

		if ( !write_member(state, member) ) {
			return false;
		}

		member = member->next;
	}

	return write_all(state, SYMBOL_OBJECT_CLOSE, sizeof(SYMBOL_OBJECT_CLOSE));
}


static bool write_value(wstate_t* state, const zetes_value_t* value) {
	switch(value->type) {
	case ZETES_TYPE_NULL:
		return write_all(state, SYMBOL_NULL, sizeof(SYMBOL_NULL));

	case ZETES_TYPE_BOOL:
		return write_bool(state, value->variant._bool);

	case ZETES_TYPE_NUMBER:
		return write_number(state, value->variant._number);

	case ZETES_TYPE_STRING:
		return write_string(state, value->variant._string);

	case ZETES_TYPE_ARRAY:
		return write_array(state, value->variant._array);

	case ZETES_TYPE_OBJECT:
		return write_object(state, value->variant._object);

	default:
		set_error(state->ctx, ZETES_RESULT_INVALID_STACK);
		return false;
	}
}


zetes_result_t zetes_write(zetes_t* ctx, zetes_write_func_t write_func, void* user_data) {
	ZETES_ASSERT(ctx);
	ZETES_ASSERT(ctx->result != ZETES_RESULT_UNINITIALIZED);
	ZETES_ASSERT(write_func);

	if ( ok(ctx) && stack_validate(ctx, 1) ) {
		wstate_t state;

		state.ctx = ctx;
		state.write_func = write_func;
		state.user_data = user_data;

		write_value(&state, ctx->stack_ptr);
	}

	return ctx->result;
}


static int is_end_of_input(char c)
{
	return ( c == '\0' );
}


static int is_whitespace(char c)
{
	return (c == 0x20ul) || (c == 0x0Aul) || (c == 0x0Dul) || (c == 0x09ul);
}


static int is_control(char c)
{
	return ( c >= 0 && c < ' ' );
}


static int is_key_val_separator(char c)
{
	return ( c == ':' );
}


static int is_comma(char c)
{
	return ( c == ',' );
}


static int is_object_open(char c)
{
	return ( c == '{' );
}


static int is_object_close(char c)
{
	return ( c == '}' );
}


static int is_array_open(char c)
{
	return ( c == '[' );
}


static int is_array_close(char c)
{
	return ( c == ']' );
}


static int is_quote(char c)
{
	return ( c == '"' );
}


static int is_escape(char c)
{
	return ( c == '\\' );
}


static int is_digit(char c)
{
	return ( c >= '0' && c <= '9' );
}


static int is_zero(char c)
{
	return (c == '0' );
}


static int is_digit_1_to_9(char c)
{
	return ( c >= '1' && c <= '9' );
}


static int is_lower_hex(char c)
{
	return ( c >= 'a' && c <= 'f' );
}


static int is_upper_hex(char c)
{
	return ( c >= 'A' && c <= 'F' );
}


static int is_minus(char c)
{
	return ( c == '-' );
}


static int is_plus(char c)
{
	return ( c == '+' );
}


static int is_exponent_delimiter(char c)
{
	return ( c == 'e' ) || ( c == 'E' );
}


static int is_decimal_point(char c)
{
	return ( c == '.' );
}


static char next_char(rstate_t* state) {
	if ( state->prev_char ) {
		char c = state->prev_char;
		state->prev_char = 0;
		return c;
	}

	if ( state->buffer_i >= state->buffer_e ) {
		int n_read;

		n_read = state->read_func(state->ctx->temp, ZETES_TEMP_BUFFER_SIZE, state->user_data);

		if ( n_read < 0 ) {
			set_error(state->ctx, ZETES_RESULT_READ_ERROR);
			return 0;
		}

		state->buffer_i = state->ctx->temp;
		state->buffer_e = state->buffer_i + n_read;

		if ( n_read == 0 ) {
			return 0;
		}
	}

	return *(state->buffer_i++);
}


static void putback_char(rstate_t* state, char c) {
	state->prev_char = c;
}


static void append_string(rstate_t* state, char c) {
	zetes_t* ctx = state->ctx;
	char* buffer_i = (char*) (ctx->buffer_ptr);
	char* buffer_e = (char*) (ctx->buffer_end);

	if ( buffer_i >= buffer_e ) {
		set_error(state->ctx, ZETES_RESULT_OUT_OF_MEMORY);
	} else {
		*buffer_i++ = c;
		ctx->buffer_ptr = buffer_i;
	}
}


static void lex_string(rstate_t* state) {
	zetes_t* ctx = state->ctx;
	char* str = (char*) (ctx->buffer_ptr);
	char c;
	int n;
	uint16_t code_point;
	uint8_t bits;

	for (;;) {
		c = next_char(state);

		if ( is_end_of_input(c) ) {
			set_error(ctx, ZETES_RESULT_UNEXPECTED_END_OF_INPUT);
			return;
		} else if ( is_control(c) ) {
			set_error(ctx, ZETES_RESULT_INVALID_CHARACTER);
			return;
		} else if ( is_quote(c) ) {
			append_string(state, '\0');

			state->token_type = TOKEN_TYPE_LITERAL;
			state->token_value.type = ZETES_TYPE_STRING;
			state->token_value.variant._string = str;

			break;
		} else if ( is_escape(c) ) {
			char* buffer_i = (char*) (ctx->buffer_ptr);
			char* buffer_e = (char*) (ctx->buffer_end);

			c = next_char(state);

			switch(c) {
			case '"':	append_string(state, '"');		break;
			case '\\':	append_string(state, '\\');		break;
			case '/': 	append_string(state, '/');		break;
			case 'b': 	append_string(state, '\b');		break;
			case 'f': 	append_string(state, '\f');		break;
			case 'n': 	append_string(state, '\n');		break;
			case 'r': 	append_string(state, '\r');		break;
			case 't': 	append_string(state, '\t');		break;

			case 'u':
				code_point = 0;

				for (n = 0; n < 4; n++) {
					c = next_char(state);

					if ( is_digit(c) ) {
						code_point = (code_point << 4U) | (c - '0');
					} else if ( is_lower_hex(c) ) {
						code_point = (code_point << 4U) | (c - 'a' + 10);
					} else if ( is_upper_hex(c) ) {
						code_point = (code_point << 4U) | (c - 'A' + 10);
					} else {
						state->ctx->result = ZETES_RESULT_INVALID_STRING;
						return;
					}
				}

				if ( code_point < 0x80UL ) {
					bits = 0x00;
					n = 1;
				} else if ( code_point < 0x800UL ) {
					bits = 0xC0;
					n = 2;
				} else if ( code_point < 0x10000UL ) {
					bits = 0xE0;
					n = 3;
				} else if ( code_point < 0x200000UL ) {
					bits = 0xF0;
					n = 4;
				} else if ( code_point < 0x4000000UL ) {
					bits = 0xF8;
					n = 5;
				} else {
					bits = 0xFC;
					n = 6;
				}

				if ( buffer_i + n >= buffer_e ) {
					set_error(ctx, ZETES_RESULT_OUT_OF_MEMORY);
					return;
				}

				for (int len = n - 1; len > 0; --len) {
					buffer_i[len] = (char) ((code_point & 0x3FUL) | 0x80UL);
					code_point >>= 6U;
				}

				buffer_i[0] = (char) (code_point | bits);
				state->ctx->buffer_ptr += n;
				break;

			default:
				set_error(ctx, ZETES_RESULT_INVALID_STRING);
				return;
			}

		} else {
			append_string(state, c);
		}
	}
}


static void lex_number(rstate_t* state) {
	zetes_t* ctx = state->ctx;
	char c;
	int is_negative = 0;
	zetes_number_t value = 0;
	long exp = 0;
	zetes_number_t power;

	c = next_char(state);

	if ( is_minus(c) )
	{
		is_negative = 1;
		c = next_char(state);
	}

	if ( is_digit_1_to_9(c) )
	{
		do
		{
			value = (value * (zetes_number_t) 10.0) + (zetes_number_t) c - (zetes_number_t) '0';
			c = next_char(state);

		} while ( is_digit(c) );
	}
	else if ( !is_zero(c) )
	{
		set_error(ctx, ZETES_RESULT_INVALID_NUMBER);
		return;
	}
	else
	{
		c = next_char(state);
	}

	if ( is_decimal_point(c) )
	{
		c = next_char(state);

		if ( !is_digit(c) )
		{
			set_error(ctx, ZETES_RESULT_INVALID_NUMBER);
			return;
		}

		do
		{
			value = (value * (zetes_number_t) 10.0) + (zetes_number_t) c - (zetes_number_t) '0';
			exp--;

			c = next_char(state);

		} while ( is_digit(c) );
	}

	if ( is_exponent_delimiter(c) )
	{
		long exp_part = 0;
		int exp_negative = 0;

		c = next_char(state);

		if ( is_plus(c) )
		{
			c = next_char(state);
		}
		else if ( is_minus(c) )
		{
			exp_negative = 1;
			c = next_char(state);
		}

		if ( !is_digit(c) )
		{
			set_error(ctx, ZETES_RESULT_INVALID_NUMBER);
			return;
		}

		do
		{
			exp_part = (exp_part * 10) + (long) c - (long) '0';
			c = next_char(state);
		} while ( is_digit(c) );

		if ( exp_negative ) exp -= exp_part;
		else exp += exp_part;
	}

	putback_char(state, c);

	power = (zetes_number_t) 10.0;

	if ( exp < 0 )
	{
		exp = -exp;

		while (exp)
		{
			if ( exp & 1 ) value /= power;
			exp >>= 1;
			power *= power;
		}
	}
	else
	{
		while (exp)
		{
			if ( exp & 1 ) value *= power;
			exp >>= 1;
			power *= power;
		}
	}

	if ( is_negative ) value = -value;

	state->token_type = TOKEN_TYPE_LITERAL;
	state->token_value.type = ZETES_TYPE_NUMBER;
	state->token_value.variant._number = value;
}


static void next_token(rstate_t* state) {
	zetes_t* ctx = state->ctx;
	state->token_type = TOKEN_TYPE_UNDEFINED;
	state->token_value.type = ZETES_TYPE_NONE;

	for (;;) {
		char c = next_char(state);

		if ( is_whitespace(c) ) {
			continue;
		} else if ( is_end_of_input(c) ) {
			state->token_type = TOKEN_TYPE_END_OF_INPUT;
		} else if ( is_key_val_separator(c) ) {
			state->token_type = TOKEN_TYPE_KEY_VAL_SEPARATOR;
		} else if ( is_comma(c) ) {
			state->token_type = TOKEN_TYPE_COMMA;
		} else if ( is_object_open(c) ) {
			state->token_type = TOKEN_TYPE_OBJECT_OPEN;
		} else if ( is_object_close(c) ) {
			state->token_type = TOKEN_TYPE_OBJECT_CLOSE;
		} else if ( is_array_open(c) ) {
			state->token_type = TOKEN_TYPE_ARRAY_OPEN;
		} else if ( is_array_close(c) ) {
			state->token_type = TOKEN_TYPE_ARRAY_CLOSE;
		} else if ( is_quote(c) ) {
			lex_string(state);
		} else if ( is_digit(c) || is_minus(c) ) {
			putback_char(state, c);
			lex_number(state);
		} else if ( c == SYMBOL_NULL[0] ) {
			if ( next_char(state) != SYMBOL_NULL[1] ||
					next_char(state) != SYMBOL_NULL[2] ||
					next_char(state) != SYMBOL_NULL[3] ) {
				set_error(ctx, ZETES_RESULT_UNKNOWN_KEYWORD);
			} else {
				state->token_type = TOKEN_TYPE_LITERAL;
				state->token_value.type = ZETES_TYPE_NULL;
			}
		} else if ( c == SYMBOL_FALSE[0] ) {
			if ( next_char(state) != SYMBOL_FALSE[1] ||
					next_char(state) != SYMBOL_FALSE[2] ||
					next_char(state) != SYMBOL_FALSE[3] ||
					next_char(state) != SYMBOL_FALSE[4] ) {
				set_error(ctx, ZETES_RESULT_UNKNOWN_KEYWORD);
			} else {
				state->token_type = TOKEN_TYPE_LITERAL;
				state->token_value.type = ZETES_TYPE_BOOL;
				state->token_value.variant._bool = false;
			}
		} else if ( c == SYMBOL_TRUE[0] ) {
			if ( next_char(state) != SYMBOL_TRUE[1] ||
					next_char(state) != SYMBOL_TRUE[2] ||
					next_char(state) != SYMBOL_TRUE[3] ) {
				set_error(ctx, ZETES_RESULT_UNKNOWN_KEYWORD);
			} else {
				state->token_type = TOKEN_TYPE_LITERAL;
				state->token_value.type = ZETES_TYPE_BOOL;
				state->token_value.variant._bool = true;
			}
		} else {
			set_error(ctx, ZETES_RESULT_INVALID_CHARACTER);
		}
		
		break;
	}
}


static bool match_token(rstate_t* state, token_type_t type) {
	if ( state->token_type == type ) {
		next_token(state);
		return true;
	} else {
		return false;
	}
}


static bool expect_token(rstate_t* state, token_type_t type) {
	zetes_t* ctx = state->ctx;

	if ( state->token_type != type ) {
		set_error(ctx, ZETES_RESULT_SYNTAX_ERROR);
		return false;
	} else {
		return true;
	}
}


static void parse_array(rstate_t* state) {
	zetes_t* ctx = state->ctx;

	zetes_push_new_array(ctx);

	if ( !match_token(state, TOKEN_TYPE_ARRAY_CLOSE) ) {
		while ( ok(ctx) ) {
			parse_value(state);
			zetes_array_append(ctx);

			if ( match_token(state, TOKEN_TYPE_COMMA) ) {
				continue;
			}

			if ( !match_token(state, TOKEN_TYPE_ARRAY_CLOSE) ) {
				set_error(ctx, ZETES_RESULT_SYNTAX_ERROR);
			}

			break;
		}
	}
}


static void parse_object(rstate_t* state) {
	zetes_t* ctx = state->ctx;

	zetes_push_new_object(ctx);

	if ( !match_token(state, TOKEN_TYPE_OBJECT_CLOSE) ) {
		while ( ok(ctx) ) {
			const char* key;

			if ( !expect_token(state, TOKEN_TYPE_LITERAL) || state->token_value.type != ZETES_TYPE_STRING ) {
				set_error(ctx, ZETES_RESULT_SYNTAX_ERROR);
				break;
			}

			key = state->token_value.variant._string;

			next_token(state);

			if ( !match_token(state, TOKEN_TYPE_KEY_VAL_SEPARATOR) ) {
				break;
			}

			parse_value(state);
			object_set_with_interned_key(ctx, key);

			if ( match_token(state, TOKEN_TYPE_COMMA) ) {
				continue;
			}

			if ( !match_token(state, TOKEN_TYPE_OBJECT_CLOSE) ) {
				set_error(ctx, ZETES_RESULT_SYNTAX_ERROR);
			}

			break;
		}
	}
}


static void parse_value(rstate_t* state) {
	if ( !ok(state->ctx) ) return;

	if ( match_token(state, TOKEN_TYPE_ARRAY_OPEN) ) {
		parse_array(state);
	} else if ( match_token(state, TOKEN_TYPE_OBJECT_OPEN) ) {
		parse_object(state);
	} else {
		if ( expect_token(state, TOKEN_TYPE_LITERAL ) ) {
			zetes_value_t* slot = stack_emplace(state->ctx);

			if ( slot ) {
				*slot = state->token_value;
				next_token(state);
			}
		}
	}
}


zetes_result_t zetes_read(zetes_t* ctx, zetes_read_func_t read_func, void* user_data) {
	ZETES_ASSERT(ctx);
	ZETES_ASSERT(ctx->result != ZETES_RESULT_UNINITIALIZED);
	ZETES_ASSERT(read_func);

	if ( ok(ctx) ) {
		rstate_t rstate;

		rstate.ctx = ctx;
		rstate.read_func = read_func;
		rstate.user_data = user_data;
		rstate.buffer_i = NULL;
		rstate.buffer_e = NULL;
		rstate.prev_char = 0;
		rstate.token_type = TOKEN_TYPE_UNDEFINED;

		next_token(&rstate);
		parse_value(&rstate);

		if ( ok(ctx) ) {
			expect_token(&rstate, TOKEN_TYPE_END_OF_INPUT);
		}
	}

	return ctx->result;
}
