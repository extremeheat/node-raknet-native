/// <reference path="./ts/RakNet.d.ts" />
import { EventEmitter } from 'events'
import { PacketPriority, PacketReliability, ServerOptions } from './ts/Constants';

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