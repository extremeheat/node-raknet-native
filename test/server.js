/* eslint-env mocha */
const { Server, Client } = require('raknet-native')
const { MessageID, PacketPriority, PacketReliability } = require('../lib/Constants')

async function pingTest () {
  return new Promise((resolve, reject) => {
    const message = 'FMCPE;JSRakNet - JS powered RakNet;408;1.16.20;0;5;0;JSRakNet;Creative;'
    const server = new Server('0.0.0.0', 19130, {
      maxConnections: 3,
      minecraft: {},
      message: Buffer.from(message)
    })
    const client = new Client('127.0.0.1', 19130, 'minecraft')

    client.on('pong', (data) => {
      const msg = data.extra?.toString()
      console.log('PONG data', data)
      if (!msg || msg !== message) throw Error(`PONG mismatch ${msg} != ${message}`)
      console.log('OK')
      client.close()
      server.close()
      setTimeout(() => {
        res() // allow for server + client to close
      }, 500)
    })

    server.listen()
    client.ping()
  })
}

async function connectTest () {
  return new Promise((res, rej) => {
    const message = 'FMCPE;JSRakNet - JS powered RakNet;408;1.16.20;0;5;0;JSRakNet;Creative;'
    const server = new Server('0.0.0.0', 19130, {
      maxConnections: 3,
      minecraft: {},
      message: Buffer.from(message)
    })
    const client = new Client('127.0.0.1', 19130, 'minecraft')

    server.listen()
    let lastC = 0
    client.on('connect', () => {
      console.log('connected!')
      client.on('encapsulated', (encap) => {
        console.assert(encap.buffer[0] === 0xf0)
        const ix = encap.buffer[1]
        if (lastC++ !== ix) {
          throw Error(`Packet mismatch: ${lastC - 1} != ${ix}`)
        }
        client.send(encap.buffer, PacketPriority.HIGH_PRIORITY, PacketReliability.UNRELIABLE, 0)
      })
    })
    let lastS = 0
    server.on('encapsulated', (encap) => {
      console.assert(encap.buffer[0] == 0xf0)
      const ix = encap.buffer[1]
      if (lastS++ !== ix) {
        throw Error(`Packet mismatch: ${lastS - 1} != ${ix}`)
      }
      if (lastS === 50) {
        client.close()
        server.close()
        res(true)
      }
    })
    server.on('openConnection', (client) => {
      console.debug('Client opened connection')
      for (let i = 0; i < 50; i++) {
        const buf = Buffer.alloc(1000)
        for (let j = 0; j < 64; j += 4) buf[j] = j + i
        buf[0] = 0xf0
        buf[1] = i
        client.send(buf, PacketPriority.HIGH_PRIORITY, PacketReliability.UNRELIABLE, 0)
      }
    })
    client.connect()
  })
}

async function kickTest () {
  return new Promise((resolve, reject) => {
    const server = new Server('0.0.0.0', 19131, {
      maxConnections: 3
    })
    const client = new Client('127.0.0.1', 19131, 'minecraft')

    server.on('openConnection', (client) => {
      console.log('new connection', client)
      client.close()
    })
    server.listen()
    client.on('disconnect', packet => {
      console.log('clien got disconnect', packet)
      try {
        client.send(Buffer.from('\xf0 yello'), PacketPriority.HIGH_PRIORITY, PacketReliability.UNRELIABLE, 0)
      } catch (e) {
        console.log('** Expected error 😀 **', e)
        server.close()
        client.close()
        res()
      }
    })

    client.connect()
  })
}

describe('server tests', function () {
  this.timeout(5000)
  it('ping test', pingTest)
  it('connection test', connectTest)
  it('kick test', kickTest)
})
