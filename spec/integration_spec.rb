require 'spec_helper'
require 'json'
require 'pathname'

RSpec.describe do
  Hash.include(HashUnnest)

  it { expect({}.unnest).to eq({}) }
  it { expect({ 'a' => 1 }.unnest).to eq({ 'a' => 1 }) }
  it { expect({ 'a' => { 'b' => 1 }, 'c' => 2 }.unnest).to eq({ 'a.b' => 1, 'c' => 2 }) }
  it { expect({ 'a' => { 'b' => 1 }, 'c' => { 'd' => { 'e' => 2 } } }.unnest).to eq({ 'a.b' => 1, 'c.d.e' => 2 }) }
  
  # sorting
  specify do
    expect({ 'b' => { 'c' => 1, 'a' => 2 }, 'a' => 3 }.unnest.keys).
      to eq(%w[a b.a b.c])
  end

  # large hash
  let(:large_hashes) {
    Pathname.glob('spec/*.json').map { |path|
      JSON.parse(path.read)
    }
  }
  it { expect(large_hashes.sample.unnest.keys.length).to eq(2244) }
  it { 2000.times { expect(large_hashes.sample.unnest.keys.length).to eq(2244) } }
end
