#include <ruby.h>
#include <assert.h>
#include "hash_unnest.h"

#define RECORD_MAGIC 0xF00DF00D
#define BUFFER_MAGIC 0xBA0BAB00

// forward declarataions of closures
static int hn_size_closure(VALUE key, VALUE val, VALUE in);
static int hn_unnest_closure(VALUE key, VALUE val, VALUE in);

static VALUE hn_hash_unnest_m = Qnil;

static VALUE hn_buffer_type = Qnil;
static VALUE hn_int_type = Qnil;

// a (sortable) key-value entry
typedef struct {
  #ifndef NDEBUG
    unsigned int magic;
  #endif
  VALUE key;
  VALUE value;
} hn_entry_t;

// a safe C array of key, value pairs
typedef struct {
  #ifndef NDEBUG
    unsigned int magic;
  #endif
  int size;
  int cursor;
  hn_entry_t* buf;
} hn_buffer_t;

/******************************************************************************/

static void hn_buffer_free(hn_buffer_t* buf) {
  LOG("*** hn_buffer_free\n");

  free(buf->buf);
  buf->buf = NULL;
  #ifndef NDEBUG
    buf->magic = 0;
  #endif
  free(buf);
}

static void hn_buffer_mark(hn_buffer_t* buf) {
  LOG("*** hn_buffer_mark\n");

  assert(buf->magic == BUFFER_MAGIC);

  for (int k = 0; k < buf->cursor; ++k) {
    LOG("  rb_gc_mark '%s'\n", StringValueCStr(buf->buf[k].key));
    rb_gc_mark(buf->buf[k].key);
    rb_gc_mark(buf->buf[k].value);
  }
}

static VALUE hn_buffer_alloc(int size) {
  hn_buffer_t* buf = NULL;

  buf = malloc(sizeof(hn_buffer_t));
  #ifndef NDEBUG
    buf->magic = BUFFER_MAGIC;
  #endif
  buf->buf   = calloc(size+1, sizeof(hn_entry_t));
  buf->size  = size;
  buf->cursor = 0;

  for (int k = 0; k < size; ++k) {
    #ifndef NDEBUG
      buf->buf[k].magic = RECORD_MAGIC;
    #endif
    buf->buf[k].key = Qnil;
    buf->buf[k].value = Qnil;
  }

  // sentinel entry
  #ifndef NDEBUG
    buf->buf[size].magic = 0;
  #endif
  buf->buf[size].key   = Qnil;
  buf->buf[size].value = Qnil;

  return Data_Wrap_Struct(hn_buffer_type, hn_buffer_mark, hn_buffer_free, buf);
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
static int hn_unnest_closure(VALUE key, VALUE val, VALUE in)
{
  VALUE prefix = rb_ary_entry(in, 0);

#ifdef DEBUG
  VALUE _str_key = rb_funcall(key,    rb_intern("inspect"), 0);
  VALUE _str_val = rb_funcall(val,    rb_intern("inspect"), 0);
  LOG("*** hn_unnest_closure (%s,%s,'%s')\n", StringValueCStr(_str_key), StringValueCStr(_str_val), StringValueCStr(prefix));
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

        rb_hash_foreach(val, hn_unnest_closure, new_in);
        break;
      }
    default:
      {
        VALUE new_key = rb_str_dup(prefix);
        hn_buffer_t* c_buf;

        rb_str_append(new_key, key);
        Data_Get_Struct(rb_ary_entry(in, 1), hn_buffer_t, c_buf);

        #ifdef DEBUG
          {
            VALUE _str_key = rb_funcall(new_key, rb_intern("inspect"), 0);
            VALUE _str_val = rb_funcall(val,     rb_intern("inspect"), 0);
            LOG("  adding item %d: %s, %s\n", c_buf->cursor, StringValueCStr(_str_key), StringValueCStr(_str_val));
          }
        #endif

        assert(c_buf->magic == BUFFER_MAGIC);
        assert(c_buf->cursor >= 0);
        assert(c_buf->cursor < c_buf->size);
        assert(c_buf->buf[c_buf->cursor].magic == RECORD_MAGIC);
        c_buf->buf[c_buf->cursor].key   = new_key;
        c_buf->buf[c_buf->cursor].value = val;
        ++(c_buf->cursor);
      }
  }
  return ST_CONTINUE;
}

/******************************************************************************/

// Recursively counts the number of leaves ("size") of a nested hash.
static int hn_size_closure(VALUE key, VALUE val, VALUE in)
{
  int* size = NULL;
  switch(TYPE(val)) {
    case T_HASH:
      {
        rb_hash_foreach(val, hn_size_closure, in);
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

/* Compares the `key`s in `hn_entry_t`s, for sorting purposes. */
int hn_entry_compare(const void* a, const void* b) {
  hn_entry_t* record_a = (hn_entry_t*) a;
  hn_entry_t* record_b = (hn_entry_t*) b;

  LOG("compare '%s' to '%s'\n", StringValueCStr(record_a->key), StringValueCStr(record_b->key));

  // NOTE: use StringValue + memcmp if slow
  return strcmp(
    StringValueCStr(record_a->key),
    StringValueCStr(record_b->key)
  );
}

/******************************************************************************/

static VALUE hn_unnest(VALUE self)
{
  VALUE result, in, buf, prefix;
  hn_buffer_t* c_buf = NULL;
  int size = 0;

#ifdef DEBUG
  {
    VALUE _str_self   = rb_funcall(self,   rb_intern("to_s"), 0);
    LOG("*** hn_unnest (%s)\n", StringValueCStr(_str_self));
  }
#endif

  // count leaves in input
  rb_hash_foreach(
      self,
      hn_size_closure,
      Data_Wrap_Struct(hn_int_type, NULL, NULL, &size)
  );

#ifdef DEBUG
  {
    VALUE _str_self   = rb_funcall(self,   rb_intern("to_s"), 0);
    LOG("*** size(%s): %d\n", StringValueCStr(_str_self), size);
  }
#endif

  // unnest `self` into `buf`
  prefix = rb_str_new("", 0);
  buf = hn_buffer_alloc(size);

  in = rb_ary_new2(2);
  rb_ary_store(in, 0, prefix);
  rb_ary_store(in, 1, buf);
  
  rb_hash_foreach(self, hn_unnest_closure, in);

  // sort the C array of key, value pairs
  Data_Get_Struct(buf, hn_buffer_t, c_buf);
  assert(c_buf != NULL);
  assert(c_buf->buf != NULL);
  assert(c_buf->magic == BUFFER_MAGIC);
  assert(c_buf->cursor == size);

  qsort((void*) c_buf->buf, c_buf->cursor, sizeof(hn_entry_t), hn_entry_compare);

  // copy the array into a new hash
  result = rb_hash_new();

  for (int k = 0; k < c_buf->cursor; ++k) {
    hn_entry_t entry = c_buf->buf[k];

    assert(entry.magic == RECORD_MAGIC);
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
  hn_hash_unnest_m = rb_define_module("HashUnnest");
  hn_buffer_type = rb_define_class_under(hn_hash_unnest_m, "CHnBuf", rb_cObject);
  hn_int_type = rb_define_class_under(hn_hash_unnest_m, "CInt", rb_cObject);

  assert(hn_hash_unnest_m != Qnil);

  rb_define_method(hn_hash_unnest_m, "unnest_c", hn_unnest, 0);
  return;
}
