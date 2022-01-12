H2 Server Setup
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

OLTPBench Setup
====

* The repository has moved to [Benchbase](https://github.com/cmu-db/benchbase/tree/main).

1. Clone OLTPBench(Benchbase):
```bash
git clone --depth 1 https://github.com/cmu-db/benchbase.git
```

2. Add H2 database dependency in pom.xml:
```bash
<dependency>
    <groupId>com.h2database</groupId>
    <artifactId>h2</artifactId>
    <version>1.4.200</version>
</dependency>
```

3. Build Benchbase:
```bash
cd benchbase
./mvnw clean package   #This produces artifacts in the target folder
cd target
tar xvzf benchbase-x-SNAPSHOT.tgz  # Change x appropriately.
cd benchbase-x-SNAPSHOT
```

4. Add support for H2 database:
```bash
cd benchbase-x-SNAPSHOT/config
mkdir h2
```
Then, copy sample_tpcc_config.xml into this h2 folder
