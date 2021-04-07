const { CipherCFB8 } = require('../index')
const { MinecraftHelper } = require('../binding')

function testDecrypt () {
  const mc = new MinecraftHelper()
  const client = {
    secretKeyBytes: Buffer.from('ZOBpyzki/M8UZv5tiBih048eYOBVPkQE3r5Fl0gmUP4=', 'base64'),
    onDecryptedPacket: (...data) => console.log('Decrypted', data)
  }

  const iv = Buffer.from('ZOBpyzki/M8UZv5tiBih0w==', 'base64')

  mc.createCipher('CFB8', new Uint8Array(client.secretKeyBytes).buffer, new Uint8Array(iv).buffer)

  const txt = Buffer.from('Hello world!')
  const enc = mc.cipher(txt)
  console.log(Buffer.from(mc.decipher(Buffer.from(enc))).toString())
}

function testWrapper () {
  const secret = Buffer.from('ZOBpyzki/M8UZv5tiBih048eYOBVPkQE3r5Fl0gmUP4=', 'base64')
  const iv = Buffer.from('ZOBpyzki/M8UZv5tiBih0w==', 'base64')
  const cipher = new CipherCFB8(secret, iv)
  console.log('c', cipher)
  console.log(cipher.decipher(cipher.cipher(Buffer.from('Hello world!'))).toString())
}

testDecrypt()
testWrapper()
