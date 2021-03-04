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
}