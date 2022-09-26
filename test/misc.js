/* eslint-env mocha */
const { Server, Client } = require('raknet-native')
const { PacketPriority, PacketReliability } = require('../lib/Constants')

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

  toString () {
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

describe('misc test', () => {
  it('works with custom ServerNames', async function () {
    const randomPort = 19132 + ((Math.random() * 100) | 0)
    let server = new Server('0.0.0.0', randomPort, {
      maxConnections: 3,
      message: Buffer.from('FMCPE;JSRakNet - JS powered RakNet;408;1.16.20;0;5;0;JSRakNet;Creative;')
    })
    server.listen().then((ok) => {
      console.log('closed!')
      server = null
    })
    const client = new Client('127.0.0.1', randomPort, 'minecraft')

    client.on('encapsulated', (packet) => {
      client.send(packet.buffer, PacketPriority.HIGH_PRIORITY, PacketReliability.RELIABLE, 0)
    })

    setTimeout(() => {
      client.connect()
      client.ping()
    }, 100)

    server.on('openConnection', (client) => {
      for (let i = 0; i < 5; i++) {
        const buf = Buffer.alloc(1000)
        for (let j = 0; j < 64; j += 4) buf[j] = j + i
        buf[0] = 0xf0
        client.send(buf, 1, 0, 0)
      }
    })
  })
})
