import { EventEmitter } from 'events'
import { MessageID, PacketPriority, PacketReliability, ClientOptions, ServerOptions } from './lib/Constants';

/**
 * Internal RakNet binding in native code. Avoid using it, use Client or Server wrappers instead.
 */
export declare class RakClient {
    constructor(hostname: string, port: number, options: ClientOptions)
    ping(): void
    connect(packetCallback: (packets: [buffer: ArrayBuffer, address: string, guid: string][]) => void): void;
    send(message: Buffer, priority: PacketPriority, reliability: PacketReliability, orderingChannel: number, broadcast?: boolean): number
    close(): void
}

/**
 * Internal RakNet binding in native code. Avoid using it, use Client or Server wrappers instead.
 */
export declare class RakServer {
    constructor(hostname: string, port: number, options: ServerOptions)
    listen(packetCallback: (buffer: ArrayBuffer, address: string, guid: string) => void): void;
    send(address: string, port: number, message: Buffer, priority: PacketPriority, reliability: PacketReliability, orderingChannel: number, broadcast?: boolean): number
    close()
}

export declare class Client extends EventEmitter {
    constructor(hostname: string, port: number, options?: ClientOptions)
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
    on(event: 'connect', params: (data: { address: string, guid: string }) => void)
    /**
     * The client has been disconnected
     */
    on(event: 'disconnect', params: (data: { address: string, guid: string, reason: MessageID }) => void)
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

export { PacketPriority }
export { PacketReliability }