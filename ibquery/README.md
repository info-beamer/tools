Query a running info-beamer pi using python
===========================================

Uses the builtin query interface of info-beamer
to get live data from a running instance.

Example: Getting generic information
------------------------------------

```python
import ibquery
ib = ibquery.InfoBeamerQuery() # connects to localhost by default
print "%s is running %s. current fps: %d, uptime: %dsec" % (ib.addr, ib.version, ib.fps, ib.uptime)
```

Example: Query info-beamer memory usage
---------------------------------------

```python
import ibquery
ib = ibquery.InfoBeamerQuery()
print "The info-beamer process uses %dkb memory" % ib.resources.memory
```

Example: Query memory usage of each node
----------------------------------------

```python
import ibquery
ib = ibquery.InfoBeamerQuery()
for path in ib.nodes:
    print "%s requires %dkb memory" % (path, ib.node(path).mem)
```

Example: Connect to a node
--------------------------

This requires a node that react to `input` events. See
https://info-beamer.com/doc/info-beamer#node.event/input

```python
import ibquery
ib = ibquery.InfoBeamerQuery()
s = ib.node('root').io(raw=True)
s.write('hello\n')
s.flush()
print s.readline()
```
