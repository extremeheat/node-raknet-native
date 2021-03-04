async function work() {
  var server = new Server('0.0.0.0', 19132, {
    maxConnections: 3,
    minecraft: {
      message: new ServerName().toString()
    }
  })
  server.listen().then((ok) => {
    console.log('closed!!!')
    server = null
  })
  var client = new Client('127.0.0.1', 19132, 'minecraft')

  client.on('encapsulated', (packet) => {
    console.warn("Client Packet", packet)

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
  }, 100)

  server.on('openConnection', (client) => {
    console.log('OPEN CONNEC', client)
    for (let i = 0; i < 5; i++) {
      // const buf = new Uint8Array([0xf0, i])
      const buf = Buffer.alloc(1000)
      for (var j = 0; j < 64; j += 4) buf[j] = j + i
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