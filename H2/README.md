H2
====

1. Download H2 database from its official website or use the [download](http://www.h2database.com/html/download-archive.html) link here. The H2 version tested in our paper is 1.4.200.
2. Install the H2 database following its [guide](http://www.h2database.com/html/installation.html).
3. Start the H2 TCP Server by executing following commands at the server machine:
```bash
cd h2/bin
java -cp h2/bin/h2*.jar org.h2.tools.Server -tcp -tcpAllowOthers -ifNotExists -tcpPassword 'yourpasswordhere' &
```
4. When you need to stop the H2 server:
```bash
java org.h2.tools.Server -tcpShutdown tcp://128.105.144.233:9092 -tcpPassword 'yourpasswordhere'
```
