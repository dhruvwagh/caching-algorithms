using Plots, Measures
using YAML

files = Dict(
    "zipf_07.yaml" => "zipf_07",
    "zipf_09.yaml" => "zipf_09",
    "P6.lis" => "p6",
    # "P8.lis" => "p8",
    "P9.lis" => "p9",
    "P10.lis" => "p10",
    # "P11.lis" => "p11",
    "P12.lis" => "p12",
    "OLTP.lis" => "oltp",
    "S1.lis" => "s1",
    "S2.lis" => "s2",
    "wiki.1190153705" => "wiki1",
    "wiki.1190157306" => "wiki2",
    "wiki.1190160907" => "wiki3",
    "wiki.1190164508" => "wiki4",
)

function main(files)
    cmds = []
    for (in_f, out_f) = files
        in_f = string("./data/", in_f)
        out_f = string("./logs/", out_f, ".yaml")
        cmd = pipeline(`./bench.out $in_f`; stdout=out_f)
        push!(cmds, cmd)
    end
    run(reduce(&, cmds))
end

function plot_yaml()
    plots = []
    for file = readdir("./logs")
        name, _ = splitext(file)
        println(name)

        mkpath(string("./images/", name))

        yaml = YAML.load_file(string("./logs/", file); dicttype=Dict{Symbol,Any})
        if any(get(yaml, :lru_2, nothing) .== yaml[:lfu])
            delete!(yaml, :lru_2)
        end
        if any(get(yaml, :lru_3, nothing) .== yaml[:lfu])
            delete!(yaml, :lru_3)
        end
        if startswith(name, "wiki")
            delete!(yaml, :mru)
        end

        convex = startswith(name, "wiki") || name == "oltp"
        if !convex
            delete!(yaml, :belady)
        end

        xs = getindex.(yaml |> values |> first, :size)
        ys = map(runs -> getindex.(runs, :hit_rate), yaml |> values)
        labels = reshape(yaml |> keys |> collect .|> string, 1, :)

        p = plot(xs, ys; title=name, label=labels, markershape=:auto,
            xlabel="Number of Keys", ylabel="Hit Rate",
            legend=convex ? :bottomright : :topleft)
        push!(plots, p)
        savefig(string("./images/", name, "/hit_rate", ".png"))

        for (key, felru) = filter(keyval -> startswith(string(keyval[1]), "felru"), yaml)
            buckets = []
            for run = felru
                if haskey(run, :buckets)
                    bucket = 1 .+ vcat(0, run[:buckets])
                    p = plot(bucket; label=run[:size], line=:steppre)
                    push!(buckets, p)
                end
            end
            if !isempty(buckets)
                plot(buckets...; yscale=:log10)
                savefig(string("./images/", name, "/", key, "_bucket", ".png"))
            end
        end
    end
    plot(plots..., layout=(cld(length(plots), 2), 2), size=(800, 2400), left_margin=20mm)
    savefig("./images/all.png")
end
