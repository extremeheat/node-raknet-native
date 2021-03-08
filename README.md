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

#### class Client, Server

The Client and Server classes are JS wrappers around the internal RakClient and RakServer classes implemented in C++ in src/. See the ts/RakNet.js for usage.

#### Example

A simple generic RakNet example:

```ts
const { Client, Server, PacketPriority, PacketReliability } = require('raknet-native')
// The third paramater is for game type, you can specify 'minecraft' or leave it blank for generic RakNet
const client = new Client('127.0.0.1', 19130)
// hostname, port, serverOptions
const server = new Server('0.0.0.0', 19130, { maxConnections: 3 })
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
const client = new Client('127.0.0.1', 19130, 'minecraft')
const server = new Server('0.0.0.0', 19130, { maxConnections: 3, minecraft: { message: new MCPingMessage().toString() }  })
```

For more usage examples see tests/.

### Exported API
See index.d.ts for full API docs.

```ts
export declare class Client extends EventEmitter {
    constructor(hostname: string, port: number, game?: string)
    /**
     * Send a RakNet PING request to the server
     */
    ping(): void
    /**
     * Start a connection request with the server using host and port passed in constructor
     */
    connect(): Promise<void>
    /**
     * Recieve a PING event from the server, the extra field with the additional pong data
     */
    on(event: 'pong', params: ({ extra: Buffer }) => void)
    /**
     * The client has connected
     */
    on(event: 'connected', params: (data: { address: string, guid: string }) => void)
    /**
     * The client has been disconnected
     */
    on(event: 'disconnected', params: (data: { address: string, guid: string, reason: MessageID }) => void)
    /**
     * Recieve an actual user packet.
     */
    on(event: 'encapsulated', params: (data: { buffer: Buffer, address: string, guid: string }) => void)
    /**
     * Send a message to the server.
     * @param message The message you want to send to the server
     * @param priority The priority, which dicates if message should be sent now or queued
     * @param reliability Options to ensure a packet arrives to the recipient
     * @param orderingChannel The RakNet ordering channel, used only for ReliableOrdered packets
     */
    send(message: Buffer, priority: PacketPriority, reliability: PacketReliability, orderingChannel: number, broadcast?: boolean): number
    /**
     * Closes the connection. This is a *blocking* call.
     */
    close(): void
}

export declare class ServerClient {
    close(): void
    send(message: Buffer, priority: PacketPriority, reliability: PacketReliability, orderingChannel: number, broadcast?: boolean): number
}

export declare class Server {
    constructor(hostname: string, port: number, options: ServerOptions)
    /**
     * The list of connections tracked by RakNet at the moment. The string key is the GUID.
     */
    connections: Map<string, Server>
    /**
     * Start listening on the specified host and port
     */
    listen(): Promise<void>
    /**
     * Send a message to the client.
     * @param address The address of the client you want to send to
     * @param port The port of the client you want to send to
     * @param message The message you want to send to the client
     * @param priority The priority, which dicates if message should be sent now or queued
     * @param reliability Options to ensure a packet arrives to the recipient
     * @param orderingChannel The RakNet ordering channel, used only for ReliableOrdered packets
     * @param broadcast Send to all clients? If true, send to all clients except `address` and `port`.
     */
    send(address: string, port: number, message: Buffer, priority: PacketPriority, reliability: PacketReliability, orderingChannel: number, broadcast?: boolean): number
    /**
     * Sets additional data to be sent along with RakNet's unconnected pong packet
     */
    setOfflineMessage(buffer: Buffer | ArrayBuffer)
    /**
     * Recieve an actual user packet.
     * `address` is the address of the connected user, `guid` is a UUID. You can map this to a `connection` above.
     */
    on(event: 'encapsulated', params: (data: { buffer: Buffer, address: string, guid: string }) => void)
    /**
     * Emited on a new connection, with a `ServerClient` paramater to make it easier to send messages to this user.
     */
    on(event: 'openConnection', params: (client: ServerClient) => void)
    /**
     * Emitted after a user closes a connection.
     */
    on(event: 'closeConnection', params: (client: ServerClient, reason: MessageID) => void)
    /**
     * Closes the connection. This is a *blocking* call.
     */
    close(): void
}
```
