using Plots
using YAML

files = Dict(
    "P6.lis" => "p6",
    "OLTP.lis" => "oltp",
    "S1.lis" => "s1",
    "S2.lis" => "s2",
    "wiki.1190153705" => "wiki1",
    "wiki.1190157306" => "wiki2",
    "wiki.1190160907" => "wiki3",
    "wiki.1190164508" => "wiki4",
)

function main(files)
    for (in_f, out_f) = files
        in_f = string("./data/", in_f)
        out_f = string("./logs/", out_f, ".yaml")
        println(out_f)
        run(pipeline(`./bench.out $in_f`; stdout=out_f))
    end
end

function plot_yaml()
    for file = readdir("./logs")
        name, _ = splitext(file)
        println(name)

        yaml = YAML.load_file(string("./logs/", file); dicttype=Dict{Symbol,Any})
        if yaml[:lru_2] == yaml[:lfu]
            delete!(yaml, :lru_2)
            delete!(yaml, :lru_3)
        end
        if startswith(name, "wiki")
            delete!(yaml, :mru)
        end

        foreach(key -> delete!(yaml, key), [:belady])

        xs = getindex.(yaml |> values |> first, :size)
        ys = map(runs -> getindex.(runs, :hit_rate), yaml |> values)
        labels = reshape(yaml |> keys |> collect .|> string, 1, :)

        plot(xs, ys; title=name, label=labels,
            xlabel="Number of Keys", ylabel="Hit Rate", legend=:bottomright)
        savefig(string("./images/", name, ".png"))
    end
end
