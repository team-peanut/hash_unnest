require "bundler/gem_tasks"
require 'rake/extensiontask'
require 'rspec/core/rake_task'


Rake::ExtensionTask.new('hash_unnest') do |ext|
  ext.name = 'hash_unnest'                  # indicate the name of the extension.
  # ext.ext_dir = 'ext/weird_world'         # search for 'hello_world' inside it.
  ext.lib_dir = 'lib/hash_unnest'              # put binaries into this folder.
  # ext.config_script = 'custom_extconf.rb' # use instead of the default 'extconf.rb'.
  # ext.tmp_dir = 'tmp'                     # temporary folder used during compilation.
  # ext.source_pattern = "*.{c,cpp}"        # monitor file changes to allow simple rebuild.
  # ext.config_options << '--with-foo'      # supply additional options to configure script.
  # ext.gem_spec = spec                     # optionally indicate which gem specification
  #                                         # will be used.
end

RSpec::Core::RakeTask.new(:spec)

task :default => [:compile, :spec]
