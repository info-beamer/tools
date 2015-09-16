Query a running info-beamer pi using python
===========================================

Uses the builtin query interface of info-beamer
to get live data from a running instance.

Example: Getting generic information
------------------------------------

    import ibquery
    ib = ibquery.InfoBeamerQuery("127.0.0.1")
    print "%s is running %s. current fps: %d, uptime: %dsec" % (ib.addr, ib.version, ib.fps, ib.uptime)

Example: Query info-beamer memory usage
---------------------------------------

    import ibquery
    ib = ibquery.InfoBeamerQuery("127.0.0.1")
    print "The info-beamer process uses %dkb memory" % ib.resources.memory

Example: Query memory usage of each node
----------------------------------------

    import ibquery
    ib = ibquery.InfoBeamerQuery("127.0.0.1")
    for path in ib.nodes:
        print "%s requires %dkb memory" % (path, ib.node(path).mem)

Example: Connect to a node
--------------------------

This requires a node that react to `input` events. See
https://info-beamer.com/doc/info-beamer#node.event/input

    import ibquery
    ib = ibquery.InfoBeamerQuery("127.0.0.1")
    s = ib.node('root').io(raw=True)
    s.write('hello\n')
    s.flush()
    print s.readline()
