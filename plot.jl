using Plots
using YAML

yaml = YAML.load_file("wiki.yaml"; dicttype=Dict{Symbol,Any})

foreach(key -> delete!(yaml, key), [:lru_2, :lru_3, :mru])

xs = map(d -> d[:size], yaml |> values |> first)
ys = map(runs -> map(d -> d[:hit_rate], runs), yaml |> values)
labels = map(string, yaml |> keys |> collect)
plot(xs, ys, label=reshape(labels, 1, :),
    xlabel="Number of Keys", ylabel="Hit Rate", legend=:bottomright)

