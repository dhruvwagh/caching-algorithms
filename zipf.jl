using Random, Distributions

Random.seed!(001)

N = 1e6
for s = [0.7, 0.9]
    p = [1 / Float64(k)^s for k in 1:N]
    p = p ./ sum(p)
    data = rand(MersenneTwister(0), Categorical(p), 1 << 23)
    s_str = replace(string(s), "." => "")
    open("./data/zipf_$s_str.yaml", "w") do f
        for r = data
            println(f, "- ", r)
        end
    end
end