require 'mkmf'

PLATFORM = `uname`.strip.upcase
SHARED_FLAGS = "--std=c99 -Wall -Wextra -Werror"

# production
$CFLAGS += " #{SHARED_FLAGS} -Os -g"

# development
# $CFLAGS += " #{SHARED_FLAGS} -O0 -g -DDEBUG"
# $CFLAGS += " #{SHARED_FLAGS} -Os -g"

create_makefile('hash_unnest/hash_unnest')
