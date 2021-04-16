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