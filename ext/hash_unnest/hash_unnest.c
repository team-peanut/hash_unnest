#include <ruby.h>
#include <assert.h>
#include "hash_unnest.h"


// forward declarataions of closures
static int hash_unnest_size_closure(VALUE key, VALUE val, VALUE in);
static int hash_unnest_closure(VALUE key, VALUE val, VALUE in);

static VALUE eHashUnnestModule = Qnil;

static VALUE hn_buf_type = Qnil;
static VALUE int_type = Qnil;

// a (sortable) key-value entry
typedef struct {
  unsigned int magic;
  VALUE key;
  VALUE value;
} hn_record_t;

// a safe C array of key, value pairs
typedef struct {
  unsigned int magic;
  int size;
  int cursor;
  hn_record_t* buf;
} hn_buf_t;

/******************************************************************************/

static void hn_buf_free(hn_buf_t* buf) {
  LOG("*** hn_buf_free\n");

  free(buf->buf);
  buf->buf = NULL;
  buf->magic = 0;
  free(buf);
}

static void hn_buf_mark(hn_buf_t* buf) {
  LOG("*** hn_buf_mark\n");

  assert(buf->magic == 0xF00DF00D);

  for (int k = 0; k < buf->cursor; ++k) {
    LOG("  rb_gc_mark '%s'\n", StringValueCStr(buf->buf[k].key));
    rb_gc_mark(buf->buf[k].key);
    rb_gc_mark(buf->buf[k].value);
  }
}

static VALUE hn_buf_alloc(int size) {
  hn_buf_t* buf = NULL;

  buf = malloc(sizeof(hn_buf_t));
  buf->magic = 0xF00DF00D;
  buf->buf   = calloc(size+1, sizeof(hn_record_t));
  buf->size  = size;
  buf->cursor = 0;

  for (int k = 0; k < size; ++k) {
    buf->buf[k].magic = 0xF00DF00D;
    buf->buf[k].key = Qnil;
    buf->buf[k].value = Qnil;
  }
  buf->buf[size].magic = 0;
  buf->buf[size].key   = Qnil;
  buf->buf[size].value = Qnil;

  return Data_Wrap_Struct(hn_buf_type, hn_buf_mark, hn_buf_free, buf);
}

/******************************************************************************/

// Unnesting callback:
// 
// - When `val` is anything but a hash, just add it to the `in[1]` buffer,
//   using the prefix in `in[0]`.
//   
//   Note that in[1] is a pointer to the current buffer pointer;
//   so it sets the buffer cell, thenmoves the pointer to the next cell.
//
// - When `val` is a hash, call ourselves recursively, appending the
//   current key to the prefix.
//
static int hash_unnest_closure(VALUE key, VALUE val, VALUE in)
{
  VALUE prefix = rb_ary_entry(in, 0);

#ifdef DEBUG
  VALUE _str_key = rb_funcall(key,    rb_intern("inspect"), 0);
  VALUE _str_val = rb_funcall(val,    rb_intern("inspect"), 0);
  LOG("*** hash_unnest_closure (%s,%s,'%s')\n", StringValueCStr(_str_key), StringValueCStr(_str_val), StringValueCStr(prefix));
#endif


  switch(TYPE(val)) {
    case T_HASH:
      {
        VALUE new_prefix;
        VALUE new_in;

        new_prefix = rb_str_dup(prefix);
        rb_str_append(new_prefix, key);
        rb_str_cat2(new_prefix, ".");

        new_in = rb_ary_new2(2);
        rb_ary_store(new_in, 0, new_prefix);
        rb_ary_store(new_in, 1, rb_ary_entry(in, 1));

        rb_hash_foreach(val, hash_unnest_closure, new_in);
        break;
      }
    default:
      {
        VALUE new_key = rb_str_dup(prefix);
        hn_buf_t* output;

        rb_str_append(new_key, key);
        Data_Get_Struct(rb_ary_entry(in, 1), hn_buf_t, output);

        #ifdef DEBUG
          {
            VALUE _str_key = rb_funcall(new_key, rb_intern("inspect"), 0);
            VALUE _str_val = rb_funcall(val,     rb_intern("inspect"), 0);
            LOG("  adding item %d: %s, %s\n", output->cursor, StringValueCStr(_str_key), StringValueCStr(_str_val));
          }
        #endif

        assert(output->magic == 0xF00DF00D);
        assert(output->cursor >= 0);
        assert(output->cursor < output->size);
        assert(output->buf[output->cursor].magic == 0xF00DF00D);
        output->buf[output->cursor].key   = new_key;
        output->buf[output->cursor].value = val;
        ++(output->cursor);
      }
  }
  return ST_CONTINUE;
}

/******************************************************************************/

// Recursively counts the number of leaves ("size") of a nested hash.
static int hash_unnest_size_closure(VALUE key, VALUE val, VALUE in)
{
  int* size = NULL;
  switch(TYPE(val)) {
    case T_HASH:
      {
        rb_hash_foreach(val, hash_unnest_size_closure, in);
        break;
      }
    default:
      {
        Data_Get_Struct(in, int, size);
        *size = *size + 1;
      }
  }
  return ST_CONTINUE;
}

/******************************************************************************/

/* Compares the `key`s in `hn_record_t`s, for sorting purposes. */
int hn_record_compare(const void* a, const void* b) {
  hn_record_t* record_a = (hn_record_t*) a;
  hn_record_t* record_b = (hn_record_t*) b;

  LOG("compare '%s' to '%s'\n", StringValueCStr(record_a->key), StringValueCStr(record_b->key));

  // NOTE: use StringValue + memcmp if slow
  return strcmp(
    StringValueCStr(record_a->key),
    StringValueCStr(record_b->key)
  );
}

/******************************************************************************/

static VALUE hash_unnest_unnest(VALUE self)
{
  VALUE result, in, buf, prefix;
  hn_buffer_t* c_buf = NULL;
  int size = 0;

#ifdef DEBUG
  {
    VALUE _str_self   = rb_funcall(self,   rb_intern("to_s"), 0);
    LOG("*** hash_unnest_unnest (%s)\n", StringValueCStr(_str_self));
  }
#endif

  rb_hash_foreach(
      self,
      hash_unnest_size_closure,
      Data_Wrap_Struct(int_type, NULL, NULL, &size)
  );

  result = rb_hash_new();
  /* result = rb_hash_new_with_size(size); */

#ifdef DEBUG
  {
    VALUE _str_self   = rb_funcall(self,   rb_intern("to_s"), 0);
    LOG("*** size(%s): %d\n", StringValueCStr(_str_self), size);
  }
#endif

  prefix = rb_str_new("", 0);
  buf = hn_buf_alloc(size);

  in = rb_ary_new2(2);
  rb_ary_store(in, 0, prefix);
  rb_ary_store(in, 1, buf);
    
  rb_hash_foreach(self, hash_unnest_closure, in);

  Data_Get_Struct(buf, hn_buf_t, c_buf);
  assert(c_buf != NULL);
  assert(c_buf->buf != NULL);
  assert(c_buf->magic == 0xF00DF00D);
  assert(c_buf->cursor == size);
  qsort((void*) c_buf->buf, c_buf->cursor, sizeof(hn_record_t), hn_record_compare);

  for (int k = 0; k < c_buf->cursor; ++k) {
    hn_record_t entry = c_buf->buf[k];

    assert(entry.magic == 0xF00DF00D);
    assert(entry.key != Qnil);
    assert(entry.value != Qnil);
    rb_hash_aset(result, entry.key, entry.value);
  }

  return result;
}

/******************************************************************************/

void Init_hash_unnest(void) {
  LOG("*** Init_hash_unnest\n");

  LOG("sizeof(VALUE) = %lu\n", sizeof(VALUE));
  LOG("sizeof(int*)  = %lu\n", sizeof(int*));
  assert(sizeof(VALUE) == sizeof(int*));

  /* assume we haven't yet defined hash_unnest */
  eHashUnnestModule = rb_define_module("HashUnnest");
  hn_buf_type = rb_define_class_under(eHashUnnestModule, "CHnBuf", rb_cObject);
  int_type = rb_define_class_under(eHashUnnestModule, "CInt", rb_cObject);

  assert(eHashUnnestModule != Qnil);

  rb_define_method(eHashUnnestModule, "unnest_c", hash_unnest_unnest, 0);
  return;
}
