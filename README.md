# node-raknet-native
Native RakNet bindings for Node.js

## Install

```sh
npm install raknet-native
```

Prebuilds are provided for 64-bit Windows 10, Linux and macOS Catalina. If a prebuild does not work, create an issue and set enviornment variable FORCE_BUILD to force a manual build.

See [#Usage](#Usage) below.

## Build
You must clone the repository recursively. 

```sh
git clone --recursive https://github.com/extremeheat/node-raknet-native.git && cd node-raknet-native
npm install
```
### Build on mac

You need to install xcode utilities first:

```
xcode-select --install
```

## Usage
A simple generic RakNet example:

```ts
const { Client, Server, PacketPriority, PacketReliability } = require('raknet-native')
// hostname, port, optionalGameType
// The third paramater is for game type, you can specify 'minecraft' or leave it blank for generic RakNet
const client = new RakClient('127.0.0.1', 19130)
// hostname, port, serverOptions
const server = new RakServer('0.0.0.0', 19130, { maxConnections: 3 })
server.listen()
client.connect()
client.on('encapsulated', (buffer) => {
  console.assert(buffer.toString() == '\xA0 Hello world')
})

server.on('openConnection', (client) => {
  client.send(Buffer.from('\xA0 Hello world'), PacketPriority.HIGH_PRIORITY, PacketReliability.UNRELIABLE, 0)
})
```

For Minecraft Bedrock, use:
```ts
const { Client, Server, PacketPriority, MCPingMessage } = require('raknet-native')
const client = new RakClient('127.0.0.1', 19130, 'minecraft')
const server = new RakServer('0.0.0.0', 19130, { maxConnections: 3, minecraft: { message: new MCPingMessage().toString() }  })
```

For more usage examples see tests/.

### Exported API
```ts
export class Client extends EventEmitter {
    constructor(hostname: string, port: number, game?: string)
    ping(): void
    connect(): Promise<void>
    close(): void

    on(event: 'pong', params: ({ extra: Buffer }) => void)
    on(event: 'encapsulated', params: ({ buffer: Buffer }) => void)

    send(message: Buffer, priority: PacketPriority, reliability: PacketReliability, orderingChannel: number, broadcast?: boolean): number
}

export class ServerClient {
    close():void
    send(message: Buffer, priority: PacketPriority, reliability: PacketReliability, orderingChannel: number, broadcast?: boolean): number
}

export class Server {
    constructor(hostname: string, port: number, options: ServerOptions)
    connections: Map<string, Server>
    listen(): Promise<void>
    send(address: string, port: number, message: Buffer, priority: PacketPriority, reliability: PacketReliability, orderingChannel: number, broadcast?: boolean): number
    on(event: 'encapsulated', params: ({ buffer: Buffer }) => void)
    on(event: 'openConnection', params: (client: ServerClient) => void)
    on(event: 'closeConnection', params: (cient: ServerClient) => void)
    close(): void
}
```
