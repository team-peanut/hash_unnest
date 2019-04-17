#include <ruby.h>
#include <assert.h>
#include "hash_unnest.h"

// forward declarataions of closures
static int hash_unnest_size_closure(VALUE key, VALUE val, VALUE in);
static int hash_unnest_closure(VALUE key, VALUE val, VALUE in);

static VALUE eHashUnnestModule = Qnil;

// a (sortable) key-value entry
typedef struct {
  VALUE key;
  VALUE value;
} hn_record_t;

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
  hn_record_t** output = (hn_record_t**) rb_ary_entry(in, 1);

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
        rb_ary_store(new_in, 1, (VALUE) output);

        rb_hash_foreach(val, hash_unnest_closure, new_in);
        break;
      }
    default:
      {
        VALUE new_key = rb_str_dup(prefix);

        rb_str_append(new_key, key);
        (*output)->key   = new_key;
        (*output)->value = val;
        *output = *output + 1;
      }
  }
  return ST_CONTINUE;
}

/******************************************************************************/

// Recursively counts the number of leaves ("size") of a nested hash.
static int hash_unnest_size_closure(VALUE key, VALUE val, VALUE in)
{
  switch(TYPE(val)) {
    case T_HASH:
      {
        rb_hash_foreach(val, hash_unnest_size_closure, in);
        break;
      }
    default:
      {
        int* size = (int*) in;
        *size = *size + 1;
      }
  }
  return ST_CONTINUE;
}

/******************************************************************************/

// Compares the `key`s in `hn_record_t`s, for sorting purposes.
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
  VALUE result, in, prefix;
  hn_record_t* buf;
  hn_record_t* buf_ptr;
  int size = 0;

#ifdef DEBUG
  {
    VALUE _str_self   = rb_funcall(self,   rb_intern("to_s"), 0);
    LOG("*** hash_unnest_unnest (%s)\n", StringValueCStr(_str_self));
  }
#endif

  rb_hash_foreach(self, hash_unnest_size_closure, (VALUE) &size);
#ifdef DEBUG
  {
    VALUE _str_self   = rb_funcall(self,   rb_intern("to_s"), 0);
    LOG("*** size(%s): %d\n", StringValueCStr(_str_self), size);
  }
#endif

  buf = (hn_record_t*) malloc(size * sizeof(hn_record_t));
  buf_ptr = buf;

  prefix = rb_str_new("", 0);

  in = rb_ary_new2(2);
  rb_ary_store(in, 0, prefix);
  rb_ary_store(in, 1, (VALUE) &buf_ptr);
    
  rb_hash_foreach(self, hash_unnest_closure, in);

  qsort((void*) buf, size, sizeof(hn_record_t), hn_record_compare);

  result = rb_hash_new();
  for (int k = 0; k < size; ++k) {
    hn_record_t entry = buf[k];

    rb_hash_aset(result, entry.key, entry.value);
  }

  free(buf);

  return result;
}

/******************************************************************************/

void Init_hash_unnest(void) {
  LOG("*** Init_hash_unnest\n");

  /* assume we haven't yet defined hash_unnest */
  eHashUnnestModule = rb_define_module("HashUnnest");
  assert(eHashUnnestModule != Qnil);

  rb_define_method(eHashUnnestModule, "unnest_c", hash_unnest_unnest, 0);
  return;
}
