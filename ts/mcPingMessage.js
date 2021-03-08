module.exports = class ServerName {
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

    toBuffer() {
        const str = Buffer.from(this.toString())
        const buf = Buffer.alloc(str.length + 2)
        str.copy(buf, 2)
        buf.writeUInt16BE(str.length, 0)
        return buf
    }
}