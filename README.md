# HummingBirdCache
一个高性能轻量级的缓存服务器
## overview
包含client和一个server，这个server可以处理用户的上传文件，采用线程池和epoll控制，对大文件可以分块加速传输
## usage
```
$ cd server/src
$ make
$ ./Humming_bird_server [port]
```
另一个终端
```
$ cd client/src
$ make
$ ./Humming_bird_client [port]
```
