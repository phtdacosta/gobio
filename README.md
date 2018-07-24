# Welcome to Gobio
Minimal, fast, and native socket support for Windows NT 4.0 (and beyond) compatible with C99

## Main features
* **Native** based on the estabilished and well documented *Winsock 2* API
* **Asynchronous** sockets relying on *Input/Output Completion Ports* (IOCP)
* Automatic **multi-thread** support when available for maximum hardware usage
* Benchmarks against industry standards confirms the **blazing-fast performance**
* Created to be **unopinionated** and *fully* **hackable** to easily fit any networking needs
* Backwards **compatibility with Windows NT 4.0 and C99** makes it widely suited for legacy projects
* Everything in **less than 300 lines of code**

## Benchmarks
Siege 3.0.5 ```-b -t60s``` OS: **Windows 8.1 6.3.9600** CPU: **Intel i3-4012Y 1.5GHz** RAM: **4GB 1600MHz**
> UPDATED 07/24/2018

 Feature | NodeJS 10.0.0 | Gobio 1.0.0-beta
--------|--------|--------
Transactions      |          22331 hits  | **85387 hits**
Availability      |          100.00 % | 100.00 %
Elapsed time      |          59.95 secs | 59.21 secs
Data transferred  |          0.45 MB | 2.77 MB
Response time     |          0.00 secs | 0.00 secs
Transaction rate  |          372.48 trans/sec | **1442.13 trans/sec**
Throughput        |          0.01 MB/sec | 0.05 MB/s
Concurrency       |          1.05 | 1.34
Successful transactions    | 22346 | 85399
Failed transactions        | 0 | 0
Longest transaction        | 0.14 | 0.60
Shortest transaction       | 0.00 | 0.00

**Without any fine tuning or specific optimizations**, Gobio sucessfully handled **3.87x more requests/second** than the NodeJS counterpart.
#### Why to test against NodeJS?
Like Gobio, NodeJS main engine relies on the IOCP from the Windows API to pursue performance, but some core development decisions made it considerably slower.

## Quickstart
The source file itself contains an usage example of an HTTP response, accessible at the address ```http://127.0.0.1:7070``` after the following steps:
```
> git clone https://github.com/phtdacosta/gobio.git
> cd gobio
> gcc gobio.c -o gobio -lwsock32 -lws2_32
> gobio.exe 
```

## License and usage
Gobio is **open source** currently under the GPL 3.0 license.
*Any specific use for the project will probably require code changes.*
Best use cases include:
* RESTful Application program interfaces (API)
* Static content delivery networks (CDN)
* Pure socket microservices

### Contributions
Open an issue to discuss any found bugs or enhancement ideas.
### Vision
Gobio should remain a simple and straightforward way to deliver performant socket support for Windows and Unix systems.
Although not implemented yet, Unix support is going to be part of the project sometime in the future.

## Known issues
Currently there are no known issues.