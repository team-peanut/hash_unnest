require 'hash_unnest/hash_unnest'

module HashUnnest
  def unnest
    {}.tap do |out|
      unnest_c(out, '')
    end
  end
end
