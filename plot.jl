using Plots, Measures
using YAML
using Dates

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

function plot_competitive(name, yaml)
    convex = startswith(name, "p") || startswith(name, "s")
    p = plot(; title=name, xscale=:log10,
        xlabel="Number of Keys", ylabel="Competitive Ratio", dpi=300,
        legend=convex ? :topleft : :bottomright)
    markers = repeat(all_markers, cld(length(yaml), length(all_markers)))
    belady = getindex.(yaml[:belady], :hit_rate)
    for (key, val) = yaml
        xs = getindex.(val, :size)
        ys = getindex.(val, :hit_rate) ./ belady
        plot!(xs, ys; label=string(key),
            markershape=pop!(markers), markerstrokewidth=0.5)
    end
    savefig(string("./images/", name, "/competitive", ".png"))
    p
end


function plot_throughput(name, yaml)
    p = plot(; title=name, xlabel="Number of Threads", ylabel="Throughput [MOp/s]", 
             dpi=300, legend = :bottomright)
    markers = repeat(all_markers, cld(length(yaml), length(all_markers)))
    for (key, vals) = yaml
        xs = getindex.(vals, :num)
        ys = getindex.(vals, :throughput)
        plot!(xs, ys; label=string(key),
            markershape=pop!(markers), markerstrokewidth=0.5)
    end
    savefig(string("./images/", name, "/throughput", ".png"))
    p
end

avg(a::Vector) = sum(a) / length(a)

function plot_yaml()
    for dir = ["hit_rate", "throughput"]
        plots = []
        competitive = []
        sizes = nothing
        labels = nothing
	for file = readdir(string("./logs/", dir))
	    name, _ = splitext(file)
            println(name)
            
            mkpath(string("./images/", name))
            
            yaml = YAML.load_file(string("./logs/", dir, "/", file); dicttype=Dict{Symbol,Any})

            delete!(yaml, :bin_lru)
            delete!(yaml, :lru_2)
	    if dir == "hit_rate"
		push!(plots, plot_competitive(name, yaml))
                belady = getindex.(yaml[:belady], :hit_rate)
                
		push!(competitive, Dict([(key, getindex.(val, :hit_rate) ./ belady) for (key, val) in yaml]))
                sizes = getindex.(values(yaml) |> first, :size)
                labels = keys(yaml)
	    else
		push!(plots, plot_throughput(name, yaml))
	    end
	end
    	lines = cld(length(plots), 2)
    	plot(plots..., layout=(lines, 2), size=(800, lines * 300), left_margin=20mm, dpi=300)
	savefig(string("./images/", dir, "_all.png"))

        if dir == "hit_rate"
            markers = repeat(all_markers, cld(length(labels), length(all_markers)))
            plot(; title="Average Competitive Ratio",
                 xlabel="Number of Keys", ylabel="Average Competitive Ratio",
                 xscale=:log10, dpi=300, legend = :bottomright)
            for key = labels
                plot!(sizes, map(x -> avg([x...]), zip(getindex.(competitive, key)...))
                      ; label=string(key), markershape=pop!(markers), markerstrokewidth=0.5)
            end
            savefig(string("./images/avg_competitive.png"))
        end
    end
end

if abspath(PROGRAM_FILE) == @__FILE__
    plot_yaml()
end
