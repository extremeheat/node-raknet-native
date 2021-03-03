var SegfaultHandler = require('segfault-handler')
SegfaultHandler.registerHandler("crash.log")

var bindings = require('bindings')('node-raknet.node')

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

const serverOptions = {
    maxConnections: 3,
    minecraft: {
        message: new ServerName().toString()
    }
}

var server = new bindings.RakServer('', 19132, serverOptions)
var client = new bindings.RakClient('127.0.0.1', 19132, 'minecraft')

function host() {
    server.listen((...args) => {
        console.log('Server ->', ...args)
    })
}

function connect() {

    client.ping()

    setTimeout(() => {
        client.connect((...args) => {
            console.log('Called', ...args)
    
            const buf = new Uint8Array([0xfe, 20])
            console.log('Sending',buf,buf.buffer)
            console.log('send',client.send(buf.buffer, 1, 0, 0, false))
        
        })
    }, 1000)

    //sendEncapsulated(message: Buffer, priority : PacketPriority, reliability : PacketReliability, orderingChannel : int, broadcast = false) {
}

console.log(bindings, client)
host()
connect()

// console.log(Buffer.from([0]).byteOffset)