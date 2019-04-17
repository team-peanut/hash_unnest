# hash-unnest 

Un-nests hashes, ie transforms:

    { a: { b: 1 }, c: 2 }

into

    { a.b: 1, c: 2 }

## Installation

    gem install hash_unnest

## Usage

```ruby
require 'hash_unnest'
Hash.include(HashUnnest)
my_hash.unnest
```
