var { Server, Client } = require('../ts/RakNet')
var { MessageID, PacketPriority, PacketReliability } = require('../ts/Constants')
const ServerName = require('../ts/mcPingMessage')

async function pingTest() {
  return new Promise((res, rej) => {
    const message = new ServerName().toBuffer()
    var server = new Server('0.0.0.0', 19130, {
      maxConnections: 3,
      minecraft: {},
      message
    })
    var client = new Client('127.0.0.1', 19130, 'minecraft')

    client.on('pong', (data) => {
      const msg = data.extra?.toString()
      console.log('PONG data', data)
      if (!msg || msg != message) throw Error(`PONG mismatch ${msg} != ${message}`)
      console.log('OK')
      client.close()
      server.close()
      setTimeout(() => {
        res() // allow for server+client to close
      }, 500)
    })

    server.listen()
    client.ping()
  })
}

async function connectTest() {
  return new Promise((res, rej) => {
    const message = new ServerName().toBuffer()
    var server = new Server('0.0.0.0', 19130, {
      maxConnections: 3,
      minecraft: {},
      message
    })
    var client = new Client('127.0.0.1', 19130, 'minecraft')

    server.listen()
    var lastC = 0;
    client.on('encapsulated', (encap) => {
      console.assert(encap.buffer[0] == 0xf0)
      const ix = encap.buffer[1]
      if (lastC++ !== ix) {
        throw Error(`Packet mismatch: ${lastC - 1} != ${ix}`)
      }
      client.send(encap.buffer, PacketPriority.HIGH_PRIORITY, PacketReliability.UNRELIABLE, 0)
    })
    var lastS = 0;
    server.on('encapsulated', (encap) => {
      // console.log('Server encap', encap)
      console.assert(encap.buffer[0] == 0xf0)
      const ix = encap.buffer[1]
      if (lastS++ !== ix) {
        throw Error(`Packet mismatch: ${lastS - 1} != ${ix}`)
      }
      if (lastS == lastC) {
        client.close()
        server.close()
        res(true)
      }
    })
    server.on('openConnection', (client) => {
      console.debug('Client opened connection')
      for (let i = 0; i < 50; i++) {
        const buf = Buffer.alloc(1000)
        for (var j = 0; j < 64; j += 4) buf[j] = j + i
        buf[0] = 0xf0
        buf[1] = i
        // console.log('BUF', buf, buf.buffer)
        // console.log('i', i)
        client.send(buf, PacketPriority.HIGH_PRIORITY, PacketReliability.UNRELIABLE, 0)
      }
    })
    client.connect()
  })
}

var done = false
async function runTests() {
  console.info('🔵 Running ping test')
  await pingTest()
  console.info('✔ Passed, OK')
  console.info('🔵 Running connection test')
  await connectTest()
  console.info('✔ Passed, OK')
  done = true
}

runTests()
setTimeout(() => {
  if (!done) throw Error('Timeout!')
}, 3000)