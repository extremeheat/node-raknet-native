import { PacketPriority, PacketReliability, ServerOptions } from "./Constants";

export declare class RakClient {
  constructor(hostname: string, port: number, game?: string)
  ping()
  connect(packetCallback: (buffer: ArrayBuffer, address: string, guid: string) => void): void;
  send(message: Buffer, priority: PacketPriority, reliability: PacketReliability, orderingChannel: number, broadcast?: boolean): number
  close()
}

export declare class RakServer {
  constructor(hostname: string, port: number, options: ServerOptions)
  listen(packetCallback: (buffer: ArrayBuffer, address: string, guid: string) => void): void;
  send(message: Buffer, priority: PacketPriority, reliability: PacketReliability, orderingChannel: number, broadcast?: boolean): number
  close()
}