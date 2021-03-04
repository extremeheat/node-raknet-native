const bindings = require('./binding')
const { Client, Server } = require('./ts/RakNet')
const { MessageID, PacketReliability, PacketPriority } = require('./ts/Constants')
module.exports = {
  RakClient: bindings.RakClient,
  RakServer: bindings.RakServer,
  Client, Server, MessageID, PacketPriority, PacketReliability,
  mcPingMessage: require('./ts/mcPingMessage')
}