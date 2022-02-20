const bindings = require('./binding')
const { Client, Server } = require('./lib/RakNet')
const { MessageID, PacketReliability, PacketPriority } = require('./lib/Constants')
module.exports = {
  RakClient: bindings.RakClient,
  RakServer: bindings.RakServer,
  Client,
  Server,
  MessageID,
  PacketPriority,
  PacketReliability
}
