#include <ruby.h>
#include <assert.h>
#include "hash_unnest.h"

static VALUE eHashUnnestModule = Qnil;

static VALUE hash_unnest_unnest(VALUE self, VALUE output, VALUE prefix);

/******************************************************************************/

static int hash_unnest_closure(VALUE key, VALUE val, VALUE in)
{
  VALUE prefix = rb_ary_entry(in, 0);
  VALUE output = rb_ary_entry(in, 1);

#ifdef DEBUG
  VALUE _str_key = rb_funcall(key, rb_intern("to_s"), 0);
  VALUE _str_val = rb_funcall(val, rb_intern("to_s"), 0);
  VALUE _str_in  = rb_funcall(in,  rb_intern("to_s"), 0);
  LOG("*** hash_unnest_closure ('%s',%s,%s)\n", StringValueCStr(_str_key), StringValueCStr(_str_val), StringValueCStr(_str_in));
#endif

  switch(TYPE(val)) {
    case T_HASH:
      {
        VALUE newprefix = rb_str_dup(prefix);
        rb_str_append(newprefix, key);
        rb_str_cat2(newprefix, ".");
        hash_unnest_unnest(val, output, newprefix);
        break;
      }
    default:
      {
        VALUE newkey = rb_str_dup(prefix);
        rb_str_append(newkey, key);
        rb_hash_aset(output, newkey, val);
      }
  }
  return ST_CONTINUE;
}

/******************************************************************************/

static VALUE hash_unnest_unnest(VALUE self, VALUE output, VALUE prefix)
{
  VALUE result = rb_hash_new();
  VALUE in = rb_ary_new2(2);

#ifdef DEBUG
  VALUE _str_self   = rb_funcall(self,   rb_intern("to_s"), 0);
  VALUE _str_output = rb_funcall(output, rb_intern("to_s"), 0);
  LOG("*** hash_unnest_unnest (%s,%s,'%s')\n", StringValueCStr(_str_self), StringValueCStr(_str_output), StringValueCStr(prefix));
#endif

  rb_ary_store(in, 0, prefix);
  rb_ary_store(in, 1, output);
    
  rb_hash_foreach(self, hash_unnest_closure, in);
  return result;
}

/******************************************************************************/

void Init_hash_unnest(void) {
  LOG("*** Init_hash_unnest\n");

  /* assume we haven't yet defined hash_unnest */
  eHashUnnestModule = rb_define_module("HashUnnest");
  assert(eHashUnnestModule != Qnil);

  rb_define_method(eHashUnnestModule, "unnest_c", hash_unnest_unnest, 2);
  return;
}
