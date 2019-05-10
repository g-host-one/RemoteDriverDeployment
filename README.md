# RemoteDriverDeployment

RemoteDriverDeployment is a tool for automatic upload, registration, starting/stopping driver on target machine.

##Usage

###Client:
```
Client.exe -s service [-l file | -u] host[:port]

        -s service       Register driver as <service> on target machine
        -l file          Load driver from <file> and start on target machine
        -u               Stop driver on target machine
        host             IP addres of target machine
        port             Port of target machine. Default: 5000
```

###Server:
```
Server.exe [address:port]

        address          IP address listen on target. Default: 0.0.0.0
        port             Port listen on target. Default: 5000
```