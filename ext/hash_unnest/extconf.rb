require 'mkmf'

PLATFORM = `uname`.strip.upcase
SHARED_FLAGS = "--std=c99 -Wall -Wextra -Werror"

# production
$CFLAGS += " #{SHARED_FLAGS} -Os"

# development
# $CFLAGS += " #{SHARED_FLAGS} -O0 -g -DDEBUG"

create_makefile('hash_unnest/hash_unnest')
