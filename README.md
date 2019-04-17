# hash-unnest 

Un-nests hashes, ie transforms:

    { a: { b: 1 }, c: 2 }

into

    { a.b: 1, c: 2 }
    
Keys in the output hash will be sorted in lexicographical order.

## Installation

    gem install hash_unnest

## Usage

```ruby
require 'hash_unnest'
Hash.include(HashUnnest)

h = { a: { b: 1 }, c: 2 }
h.unnest
# => { a.b: 1, c: 2 }
```
