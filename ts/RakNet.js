const { RakClient, RakServer } = require('../binding')
const { EventEmitter } = require('events')
const { MessageID } = require('./Constants')

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
    // var recvC = 0
    this.client.listen((buffer, address, guid) => {
      const buf = Buffer.from(buffer) // copy native buffer to js
      const id = buf[0]
      // console.log('C -> ', buf, address, guid, recvC++)
      try {
        if (id < MessageID.ID_USER_PACKET_ENUM) { // Internal RakNet messages: we handle & emit
          if (id == MessageID.ID_UNCONNECTED_PONG) {
            if (buf.byteLength > 5) {
              const extra = Buffer.from(buf.slice(5))
              console.log('Extra', extra.toString())
              this.emit('pong', { extra })
            } else {
              this.emit('pong', {})
            }
          }
        } else { // User messages
          this.emit('encapsulated', { buffer: buf, address, guid })
        }
      } catch (e) {
        console.error('Server failed to read packet:', e)
      }
    })
  }

  send(message, priority, reliability, orderingChannel = 0, broadcast = false) {
    const ret = this.client.send(message instanceof ArrayBuffer ? message : message.buffer, priority, reliability, orderingChannel, broadcast)
    if (ret <= 0) {
      throw Error(`Failed to send: ${ret}`)
    }
  }
}

function ServerClient(server, address) {
  const [hostname, port] = address.split('/')
  this.address = address
  this.send = (...args) => server.send(hostname, port, ...args)

  this.neuter = () => { // Client is disconnected, no-op to block sending
    this.send = () => { }
  }
}

class Server extends EventEmitter {
  constructor(hostname, port, options) {
    super()
    this.server = new RakServer(hostname, port, options)
    this.close = () => this.server.close()
    this.connections = new Map()
    this.waiting = 0
  }

  // TODO: setOfflineMessage instead of the mc-specific things

  listen() {
    return this.server.listen((buffer, address, guid) => {
      const buf = Buffer.from(buffer)
      // console.log('S -> ', buffer, address, guid)
      try {
        const id = buf[0]
        if (id < MessageID.ID_USER_PACKET_ENUM) { // Internal RakNet messages: we handle & emit
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

module.exports = { Server, Client }