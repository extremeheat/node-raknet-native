const { RakClient, RakServer } = require('../binding')
const { EventEmitter } = require('events')
const { MessageID } = require('./Constants')

class Client extends EventEmitter {
  constructor (hostname, port, options = {}) {
    super()
    this.client = new RakClient(hostname, port, options)
    this.ping = () => this.client.ping()
    this.connect = () => this.client.connect()
    this.close = () => this.client.close()
    this.startListening()
  }

  // Handle inbound packets and emit events
  startListening () {
    // var recvC = 0
    this.client.listen((buffers, address, guid) => {
      for (const buffer of buffers) {
        const buf = Buffer.from(buffer) // copy native buffer to js
        const id = buf[0]
        // console.log('C -> ', buf, address, guid)
        try {
          if (id < MessageID.ID_USER_PACKET_ENUM) { // Internal RakNet messages: we handle & emit
            if (id == MessageID.ID_UNCONNECTED_PONG) {
              if (buf.byteLength > 5) {
                const extra = Buffer.from(buf.slice(5))
                // console.log('Extra', extra.toString())
                this.emit('pong', { extra })
              } else {
                this.emit('pong', {})
              }
            }
            if (id == MessageID.ID_CONNECTION_REQUEST_ACCEPTED) {
              this.emit('connect', { address, guid })
            }
            if (id == MessageID.ID_CONNECTION_LOST || id == MessageID.ID_DISCONNECTION_NOTIFICATION || id == MessageID.ID_CONNECTION_BANNED || id == MessageID.ID_INCOMPATIBLE_PROTOCOL_VERSION) {
              this.emit('disconnect', { address, guid, reason: id })
            }
          } else { // User messages
            this.emit('encapsulated', { buffer: buf, address, guid })
          }
        } catch (e) {
          console.warn('While decoding', buf.toString('hex'))
          console.error('Client failed to read packet:', e)
        }
      }
    })
  }

  send (message, priority, reliability, orderingChannel = 0, broadcast = false) {
    // When you Buffer.from/allocUnsafe, it may put your data into a global buffer, we need it in its own buffer
    if (message instanceof Buffer && message.buffer.byteLength != message.byteLength) message = new Uint8Array(message)
    const ret = this.client.send(message instanceof ArrayBuffer ? message : message.buffer, priority, reliability, orderingChannel, broadcast)
    if (ret <= 0) {
      throw Error(`Failed to send: ${ret}`)
    }
  }
}

function ServerClient (server, address, guid) {
  const [hostname, port] = address.split('/')
  this.address = address
  this.guid = guid
  this.send = (...args) => server.send(hostname, port, ...args)
  this.close = (silent) => server.kick(guid, silent)

  this.neuter = () => { // Client is disconnected, no-op to block sending
    this.send = () => { }
  }
}

class Server extends EventEmitter {
  constructor (hostname, port, options) {
    super()
    this.server = new RakServer(hostname, port, options)
    this.close = () => this.server.close()
    this.connections = new Map()
    if (options.message) this.setOfflineMessage(options.message)
  }

  setOfflineMessage (message) {
    if (!message instanceof Buffer) Buffer.from(message)
    this.server.setPongResponse(message)
  }

  listen () {
    return this.server.listen(packets => {
      for (const [buffer, address, guid] of packets) {
        const buf = Buffer.from(buffer)
        // console.log('S -> ', buffer, address, guid)
        try {
          const id = buf[0]
          if (id < MessageID.ID_USER_PACKET_ENUM) { // Internal RakNet messages: we handle & emit
            if (id == MessageID.ID_NEW_INCOMING_CONNECTION) {
              const client = new ServerClient(this, address, guid)
              this.connections.set(guid, client)
              this.emit('openConnection', client)
            } else if (id == MessageID.ID_DISCONNECTION_NOTIFICATION || id == MessageID.ID_CONNECTION_LOST || id == MessageID.ID_INCOMPATIBLE_PROTOCOL_VERSION) {
              if (this.connections.has(guid)) {
                const con = this.connections.get(guid)
                this.emit('closeConnection', con, id)
                con.neuter()
              }
              this.connections.delete(guid)
            }
          } else { // User messages
            this.emit('encapsulated', { buffer: buf, address, guid })
          }
        } catch (e) { // If we don't handle this the program will segfault
          console.warn('While decoding', buf.toString('hex'))
          console.error('Server failed to read packet:', e)
        }
      }
    })
  }

  send (sendAddr, sendPort, message, priority, reliability, orderingChannel = 0, broadcast = false) {
    if (message instanceof Buffer && message.buffer.byteLength != message.byteLength) message = new Uint8Array(message)
    // console.warn('SENDING', arguments)
    const ret = this.server.send(sendAddr, parseInt(sendPort), message instanceof ArrayBuffer ? message : message.buffer, priority, reliability, orderingChannel, broadcast)
    if (ret <= 0) {
      throw Error(`Failed to send: ${ret}`)
    }
  }

  kick (clientGuid, silent) {
    this.server.kick(clientGuid, silent)
  }
}

module.exports = { Server, Client }
