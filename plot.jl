using Plots, Measures
using YAML
using Dates

files = Dict(
    "zipf_07.yaml" => "zipf_07",
    "zipf_09.yaml" => "zipf_09",
    "P6.lis" => "p6",
    "P8.lis" => "p8",
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
    println("Start : ", Dates.now())
    for cmds = Iterators.partition(cmds, 6)
        try
            run(reduce(&, cmds))
        catch e
            println(e)
            if isa(e, ProcessFailedException)
                fail = map(p -> p.cmd, e.procs)
                cmds = filter(p -> p.cmd in fail, cmds)
                println(cmds)
                foreach(run, cmds)
            end
        end
    end
    println("End : ", Dates.now())
    plot_yaml()
end

function plot_yaml()
    plots = []
    all_markers = [:dtriangle, :star5, :diamond, :utriangle, :pentagon, :star4, :star6, :hexagon, :circle]
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

        dist = startswith(name, "wiki") || startswith(name, "zipf")
        if dist
            delete!(yaml, :mru)
        end

        convex = dist || name == "oltp"
        shape = startswith(name, "s") || startswith(name, "p")

        p = plot(; title=name, xscale=:log10, yscale=shape ? :log10 : :auto,
            xlabel="Number of Keys", ylabel="Hit Rate", dpi=300,
            legend=convex || shape ? :bottomright : :topleft)
        markers = repeat(all_markers, cld(length(yaml), length(all_markers)))
        for (key, val) = yaml
            xs = getindex.(val, :size)
            ys = getindex.(val, :hit_rate)
            plot!(xs, ys; label=string(key),
                markershape=pop!(markers), markerstrokewidth=0.5)
        end
        push!(plots, p)
        savefig(string("./images/", name, "/hit_rate", ".png"))

        yaml = filter(keyval -> all(haskey.(keyval[2], :buckets)), yaml)
        for (key, bin) = yaml
            buckets =
                [plot(1 .+ vcat(0, run[:buckets]);
                    label=run[:size], line=:steppre) for run = bin]
            plot(buckets...; yscale=:log10, dpi=300,
                xlabel="Evicted bucket size", ylabel="frequency", guidefont=8)
            savefig(string("./images/", name, "/", key, "_bucket", ".png"))
        end
    end
    plot(plots..., layout=(cld(length(plots), 2), 2), size=(800, 2400), left_margin=20mm, dpi=300)
    savefig("./images/all.png")
end

if abspath(PROGRAM_FILE) == @__FILE__
    main(files)
end
