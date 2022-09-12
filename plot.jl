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

all_markers = [
    :dtriangle,
    :star5,
    :diamond,
    :utriangle,
    :pentagon,
    :star4,
    :star6,
    :hexagon,
    :circle
]

function hit_rate(files)
    cmds = []
    for (in_f, out_f) = files
        in_f = string("./data/", in_f)
        out_f = string("./logs/hit_rate/", out_f, ".yaml")
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

function throughput(files)
    println("Start : ", Dates.now())
    for (in_f, out_f) = files
        in_f = string("./data/", in_f)
        out_f = string("./logs/throughput/", out_f, ".yaml")
        cmd = pipeline(`./parallel.out $in_f`; stdout=out_f)
        run(cmd)
    end
    println("End : ", Dates.now())
    plot_yaml()
end

function plot_hit_rate(name, yaml)
    convex = startswith(name, "wiki") || startswith(name, "zipf") || name == "oltp"
    p = plot(; title=name, xscale=:log10,
        xlabel="Number of Keys", ylabel="Hit Rate", dpi=300,
        legend=convex ? :bottomright : :topleft)
    markers = repeat(all_markers, cld(length(yaml), length(all_markers)))
    for (key, val) = yaml
        xs = getindex.(val, :size)
        ys = getindex.(val, :hit_rate)
        plot!(xs, ys; label=string(key),
            markershape=pop!(markers), markerstrokewidth=0.5)
    end
    savefig(string("./images/", name, "/hit_rate", ".png"))
    p
end

function plot_throughput(name, yaml)
    p = plot(; title=name, xlabel="Number of Threads", ylabel="Throughput", 
             dpi=300, legend = :bottomright)
    markers = repeat(all_markers, cld(length(yaml) * length(yaml |> values |> first), length(all_markers)))
    for (key, vals) = yaml
        for val = vals
            th = val[:threads]
            xs = getindex.(th, :num)
            ys = getindex.(th, :throughput)
            plot!(xs, ys; label=string(val[:size]),
                markershape=pop!(markers), markerstrokewidth=0.5)
        end
    end
    savefig(string("./images/", name, "/throughput", ".png"))
    p
end

function plot_yaml()
    for dir = ["hit_rate", "throughput"]
        plots = []
	for file = readdir(string("./logs/", dir))
	    name, _ = splitext(file)
            println(name)
            
            mkpath(string("./images/", name))
            
            yaml = YAML.load_file(string("./logs/", dir, "/", file); dicttype=Dict{Symbol,Any})

            delete!(yaml, :bin_lru)
            delete!(yaml, :lru_2)
	    if dir == "hit_rate"
		push!(plots, plot_hit_rate(name, yaml))
	    else
		push!(plots, plot_throughput(name, yaml))
	    end
	end
    	lines = cld(length(plots), 2)
    	plot(plots..., layout=(lines, 2), size=(800, lines * 300), left_margin=20mm, dpi=300)
	savefig(string("./images/", dir, "_all.png"))
    end
end

if abspath(PROGRAM_FILE) == @__FILE__
    hit_rate(files)
    throughput(files)
end
