H2
====

1. Download H2 database from its official [download](http://www.h2database.com/html/download-archive.html) link or use the zip file in this folder. The H2 version tested in our paper is 1.4.200.
2. Install the H2 database following its [guide](http://www.h2database.com/html/installation.html), or use following command:
```bash
unzip h2-2019-10-14.zip
```
3. Start the H2 TCP Server by executing following commands at the server machine:
```bash
cd h2/bin
java -cp h2*.jar org.h2.tools.Server -tcp -tcpAllowOthers -ifNotExists -tcpPassword 'yourpasswordhere' &
```
4. When you need to stop the H2 server:
```bash
java org.h2.tools.Server -tcpShutdown tcp://[your IP address]:9092 -tcpPassword 'yourpasswordhere'
```
5. The H2 official [tutorial](http://www.h2database.com/html/tutorial.html#using_server) can also help you setup the server.
