## 1.2.0
* Client specific protocol versions (#14)
* Allow servers to accept older RakNet protocol version clients

## 1.1.0
* Fixes an issue with client/server receiving packets after calling close(), mitigate lifetime issue
* Mitigates some potential race conditions in packet handling
* clang-format, remove minecraft helpers, use mocha #8

## 1.0.9
* Fix macOS Catalina prebuild

## 1.0.8
* fix set maximum incoming connections bug. (#6) (@b23r0)

## 1.0.7
* Remove `segfault-handler` dependency

## 1.0.6
* Fix issue with finding prebuilds

## 1.0.4
* only build static raknet library

## 1.0.2
* Be more conservative with prebuild usage [#3](https://github.com/extremeheat/node-raknet-native/pull/3)

## 1.0.1

* Add a utility AES GCM cipher

## 1.0.0

* BREAKING: Client event name changes : 
  * connected -> connect
  * disconnected -> disconnect
* RakClient, RakServer: RakNet packets are returned to JS in batches. No changes to the wrapped Client and Server APIs.

* Adds a utility AES cipher implementation for Minecraft
* BREAKING: "Game" option for Client has been removed and now takes an options argument instead. Use { protocolVersion: 10 } for Minecraft instead.
* BREAKING: McPingMessage has been removed

## 0.2.0

Added kick method
