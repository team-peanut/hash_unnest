require 'spec_helper'

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
end
