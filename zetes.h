/** @file zetes.h
 * @brief JSON library for resource constrained systems, written in C99.
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

#ifndef _ZETES_H_
#define _ZETES_H_

#include <stdbool.h>
#include <stddef.h>


#ifndef ZETES_ASSERT
#define ZETES_ASSERT(expr)		do {} while(0)
#endif


#ifndef ZETES_ALIGN
#define ZETES_ALIGN				8
#endif


#ifndef ZETES_NUMBER_TYPE
#define ZETES_NUMBER_TYPE		double
#endif


#ifndef ZETES_TEMP_BUFFER_SIZE
#define ZETES_TEMP_BUFFER_SIZE	16
#endif


typedef enum {
	ZETES_RESULT_UNINITIALIZED,
	ZETES_RESULT_OK,
	ZETES_RESULT_OUT_OF_MEMORY,
	ZETES_RESULT_STACK_EMPTY,
	ZETES_RESULT_STACK_FULL,
	ZETES_RESULT_INDEX_OUT_OF_BOUNDS,
	ZETES_RESULT_KEY_NOT_FOUND,
	ZETES_RESULT_TYPE_MISMATCH,
	ZETES_RESULT_INVALID_STACK,
	ZETES_RESULT_WRITE_ERROR,
	ZETES_RESULT_READ_ERROR,
	ZETES_RESULT_INVALID_CHARACTER,
	ZETES_RESULT_INVALID_NUMBER,
	ZETES_RESULT_INVALID_STRING,
	ZETES_RESULT_UNKNOWN_KEYWORD,
	ZETES_RESULT_UNEXPECTED_END_OF_INPUT,
	ZETES_RESULT_SYNTAX_ERROR
} zetes_result_t;


typedef enum {
	ZETES_TYPE_NONE,
	ZETES_TYPE_NULL,
	ZETES_TYPE_BOOL,
	ZETES_TYPE_NUMBER,
	ZETES_TYPE_STRING,
	ZETES_TYPE_ARRAY,
	ZETES_TYPE_OBJECT
} zetes_type_t;


typedef ZETES_NUMBER_TYPE zetes_number_t;


typedef int (*zetes_write_func_t) (const void* buffer, int length, void* user_data);

typedef int (*zetes_read_func_t) (void* buffer, int length, void* user_data);


typedef struct zetes_t zetes_t;


zetes_result_t zetes_init(zetes_t* ctx, size_t stack_depth, void* buffer, size_t buffer_size);

void zetes_cleanup(zetes_t* ctx);

void zetes_reset(zetes_t* ctx);

zetes_result_t zetes_result(zetes_t* ctx);

void zetes_push_null(zetes_t* ctx);

void zetes_push_bool(zetes_t* ctx, bool value);

void zetes_push_number(zetes_t* ctx, zetes_number_t value);

void zetes_push_string(zetes_t* ctx, const char* value);

void zetes_push_new_array(zetes_t* ctx);

void zetes_push_new_object(zetes_t* ctx);

zetes_type_t zetes_type(zetes_t* ctx);

void zetes_pop(zetes_t* ctx);

void zetes_pop_null(zetes_t* ctx);

bool zetes_pop_bool(zetes_t* ctx);

zetes_number_t zetes_pop_number(zetes_t* ctx);

const char* zetes_pop_string(zetes_t* ctx);

size_t zetes_array_size(zetes_t* ctx);

void zetes_array_index(zetes_t* ctx, size_t index);

void zetes_array_append(zetes_t* ctx);

size_t zetes_object_size(zetes_t* ctx);

void zetes_object_index(zetes_t* ctx, size_t index);

bool zetes_object_has(zetes_t* ctx, const char* key);

void zetes_object_get(zetes_t* ctx, const char* key);

void zetes_object_set(zetes_t* ctx, const char* key);

zetes_result_t zetes_write(zetes_t* ctx, zetes_write_func_t write_func, void* user_data);

zetes_result_t zetes_read(zetes_t* ctx, zetes_read_func_t read_func, void* user_data);


#ifndef _DOXYGEN

typedef union zetes_variant_t zetes_variant_t;
typedef struct zetes_value_t zetes_value_t;
typedef struct zetes_array_element_t zetes_array_element_t;
typedef struct zetes_array_t zetes_array_t;
typedef struct zetes_object_member_t zetes_object_member_t;
typedef struct zetes_object_t zetes_object_t;


union zetes_variant_t {
	bool _bool;
	zetes_number_t _number;
	const char* _string;
	zetes_array_t* _array;
	zetes_object_t* _object;
};


struct zetes_value_t {
	zetes_type_t type;
	zetes_variant_t variant;
};


struct zetes_array_element_t {
	zetes_array_element_t* next;
	zetes_value_t value;
};


struct zetes_array_t {
	zetes_array_element_t* first;
	zetes_array_element_t* last;
};


struct zetes_object_member_t {
	zetes_object_member_t* next;
	const char* key;
	zetes_value_t value;
};


struct zetes_object_t {
	zetes_object_member_t* first;
	zetes_object_member_t* last;
};


struct zetes_t {
	zetes_result_t result;
	void* buffer_begin;
	void* buffer_end;
	void* buffer_ptr;
	zetes_value_t* stack_begin;
	zetes_value_t* stack_end;
	zetes_value_t* stack_ptr;
	char temp[ZETES_TEMP_BUFFER_SIZE];
};

#endif // _DOXYGEN

#endif /* _ZETES_H_ */
