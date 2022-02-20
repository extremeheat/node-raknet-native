const bedrockServer = require('minecraft-bedrock-server')
const bp = require('bedrock-protocol')
const { join } = require('path')
const { once } = require('events')
const net = require('net')

const getPort = () => new Promise(resolve => {
  const server = net.createServer()
  server.listen(0, '127.0.0.1')
  server.on('listening', () => {
    const { port } = server.address()
    server.close(() => resolve(port))
  })
})

const versions = ['1.16.220', '1.17.10', '1.18.0']

async function main() {
  this.timeout(1000 * 60 * 5)
  for (const version of versions) {
    const [port4, port6] = [await getPort(), await getPort()]
    console.log('Server ran on port', port4, port6)
    const handle = await bedrockServer.startServerAndWait(version, 90000, { path: join(__dirname, './bds-' + version), 'server-port': port4, 'server-portv6': port6 })

    async function connect(cachingEnabled) {
      const client = bp.createClient({
        host: 'localhost',
        port: port4,
        version,
        // @ts-ignore
        username: 'Notch',
        offline: true
      })

      client.on('join', () => {
        client.queue('client_cache_status', { enabled: cachingEnabled })
      })

      await once(client, 'join')
      client.close()
    }

    for (let i = 0; i < 50; i++) {
      await connect(false)
      console.log('✅ Once', i)
      await connect(true)
      console.log('✅ Twice', i)
    }

    await handle.kill()
    await once(handle, 'exit')
  }
}

describe('bedrock-protocol works', function () {
  // it('works', main)
})