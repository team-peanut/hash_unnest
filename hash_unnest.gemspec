# -*- encoding: utf-8 -*-
lib = File.expand_path('../lib', __FILE__)
$LOAD_PATH.unshift(lib) unless $LOAD_PATH.include?(lib)
require 'hash_unnest/version'

Gem::Specification.new do |gem|
  gem.name          = "hash_unnest"
  gem.version       = HashUnnest::VERSION
  gem.authors       = ["Julien Letessier"]
  gem.email         = ["julien.letessier@gmail.com"]
  gem.description   = %q{Fast hash unnesting}
  gem.summary       = %q{Fast hash unnesting}
  gem.homepage      = "http://github.com/mezis/hash_unnest"

  gem.add_development_dependency 'rspec'
  gem.add_development_dependency 'rake'
  gem.add_development_dependency 'rake-compiler'
  gem.add_development_dependency 'pry'

  gem.extensions    = ['ext/hash_unnest/extconf.rb']
  gem.files         = Dir.glob('lib/**/*.rb') +
                      Dir.glob('ext/**/*.{c,h,rb}') +
                      Dir.glob('*.{md,txt}')
  gem.executables   = gem.files.grep(%r{^bin/}).map{ |f| File.basename(f) }
  gem.test_files    = gem.files.grep(%r{^(test|spec|features)/})
  gem.require_paths = ["lib"]
end
