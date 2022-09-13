include("plot.jl")

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
    hit_rate(files, :redis)
    hit_rate(files, :memcache)
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

if abspath(PROGRAM_FILE) == @__FILE__
    hit_rate(files)
    # throughput(files)
end
