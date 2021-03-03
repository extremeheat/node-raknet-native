var SegfaultHandler = require('segfault-handler')
SegfaultHandler.registerHandler("crash.log")

var bindings = require('bindings')('node-raknet.node')

var client = new bindings.RakClient('127.0.0.1', 19132, 'minecraft')

function connect() {
    client.connect((...args) => {
        console.log('Called', ...args)

        const buf = Buffer.from([0xfe, 20])
        console.log('send',client.send(buf.buffer, 1, 0, 0, false))
    
    })

    //sendEncapsulated(message: Buffer, priority : PacketPriority, reliability : PacketReliability, orderingChannel : int, broadcast = false) {
}

console.log(bindings, client)
connect()