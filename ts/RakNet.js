var SegfaultHandler = require('segfault-handler')
SegfaultHandler.registerHandler("crash.log")
const { RakClient, RakServer } = require('bindings')('node-raknet.node')
const { EventEmitter } = require('events')
const { PacketPriority, PacketReliability, MessageID } = require('./Constants')

class Client extends EventEmitter {
  constructor(hostname, port, game = 'minecraft') {
    super()
    this.client = new RakClient(hostname, port, game)
    this.ping = () => this.client.ping()
    this.connect = () => this.client.connect()
    this.close = () => this.client.close()
    this.startListening()
  }

  // Handle inbound packets and emit events
  startListening() {
    var recvC = 0
    this.client.listen((buffer, address, guid) => {
      const buf = Buffer.from(buffer.slice(0)) // copy native buffer to js
      const id = buf[0]
      console.log('C -> ', buf, address, guid, recvC++)
      try {
        if (id < MessageID.ID_USER_PACKET_ENUM) { // Internal RakNet messages: we handle & emit
          if (id == MessageID.ID_UNCONNECTED_PONG) {
            if (buffer.byteLength > 5) {
              const extra = Buffer.from(buffer.slice(5))
              console.log('Extra', extra.toString())
              // this.emit('pong', { extra })
            } else {
              this.emit('pong', {})
            }
          }
        } else { // User messages
          this.emit('encapsulated', { buffer: buf })
        }
      } catch (e) {
        console.error('Server failed to read packet:', e)
      }
    })
  }

  send(message, priority, reliability, orderingChannel = 0, broadcast = false) {
    this.client.send(message instanceof ArrayBuffer ? message : message.buffer, priority, reliability, orderingChannel, broadcast)
  }
}

function ServerClient(server, address) {
  const [ hostname, port ] = address.split('/')
  this.address = address
  this.send = (...args) => server.send(hostname, port, ...args)
  this.addEncapsulatedToQueue = this.send // For back-compat with JSRakNet

  this.neuter = () => { // Client is disconnected, no-op to block sending with valid ref
    this.send = () => {}
  }
}

class Server extends EventEmitter {
  constructor(hostname, port, options) {
    super()
    this.server = new RakServer(hostname, port, options)
    this.close = () => this.server.close()
    this.connections = new Map()
  }

  // TODO: setOfflineMessage instead of the mc-specific things

  listen() {
      // Constants.MessageID(args[0]),
    console.log('Listening!')
    return this.server.listen((buffer, address, guid) => {
      try {
        console.log('S -> ', buffer, address, guid)
        const buf = Buffer.from(buffer.slice(0))
        const id = buf[0]
        if (id < /*ID_USER_PACKET_ENUM*/ 134) { // Internal RakNet messages: we handle & emit
          if (id == MessageID.ID_NEW_INCOMING_CONNECTION) {
            const client = new ServerClient(this, address)
            this.connections.set(guid, client)
            this.emit('openConnection', client)
          } else if (id == MessageID.ID_DISCONNECTION_NOTIFICATION) {
            if (this.connections.has(guid)) this.connections.get(guid).neuter()
            this.connections.delete(guid)
          }
        } else { // User messages
          this.emit('encapsulated', { buffer: buf })
        }
      } catch (e) { // If we don't handle this the program will segfault
        console.error('Server failed to read packet:', e)
      }
    })
  }

  send(sendAddr, sendPort, message, priority, reliability, orderingChannel = 0, broadcast = false) {
    // console.warn('SENDING', arguments)
    const ret = this.server.send(sendAddr, parseInt(sendPort), message instanceof ArrayBuffer ? message : message.buffer, priority, reliability, orderingChannel, broadcast)
    if (ret <= 0) {
      throw Error(`Failed to send: ${ret}`)
    }
  }
}

async function work() {
  var server = new Server('0.0.0.0', 19132, {
    maxConnections: 3,
    minecraft: {
      message: new ServerName().toString()
    }
  })
  server.listen().then((ok) => {
    console.log('closed!!!')
    server=null
  })
  var client = new Client('127.0.0.1', 19132, 'minecraft')

  client.on('encapsulated', (packet) => {
    console.warn("Client Packet",packet)

    client.send(packet.buffer, PacketPriority.HIGH_PRIORITY, PacketReliability.RELIABLE, 0)
  })

  setTimeout(() => {
    // client.on('pong', (packet) => {
    //   console.log("PONG", packet, packet.extra?.toString())

    //   client.connect()
    // })

    client.connect()

    // console.log(client, client.ping, RakClient)
    client.ping()

    // client.close()

    // server.close()
  },100)

  server.on('openConnection', (client) => {
    console.log('OPEN CONNEC',client)
    for (let i = 0; i < 5; i++) {
      // const buf = new Uint8Array([0xf0, i])
      const buf = Buffer.alloc(1000)
      for (var j =0; j < 64; j+=4) buf[j] = j+i
      buf[0] = 0xf0
      console.log('BUF', buf, buf.buffer)
      console.log('i', i)
      client.send(buf, 1, 0, 0)
    }
  })
}

class ServerName {
  motd = 'JSRakNet - JS powered RakNet'
  name = 'JSRakNet'
  protocol = 408
  version = '1.16.20'
  players = {
    online: 0,
    max: 5
  }
  gamemode = 'Creative'
  serverId = '0'

  toString() {
    return [
      'MCPE',
      this.motd,
      this.protocol,
      this.version,
      this.players.online,
      this.players.max,
      this.serverId,
      this.name,
      this.gamemode
    ].join(';') + ';'
  }
}

work()