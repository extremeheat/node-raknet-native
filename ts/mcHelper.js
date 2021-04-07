const { MinecraftHelper } = require('../binding')

class CipherCFB8 {
  constructor(secret, iv) {
    if (secret instanceof Buffer && secret.buffer.byteLength != secret.byteLength) secret = new Uint8Array(secret)
    if (iv instanceof Buffer && iv.buffer.byteLength != iv.byteLength) iv = new Uint8Array(iv)
    this.ctx = new MinecraftHelper()
    this.ctx.createCipher(secret.buffer ?? secret, iv.buffer ?? iv.buffer)
  }

  cipher(message) {
    if (message instanceof Buffer && message.buffer.byteLength != message.byteLength) message = new Uint8Array(message)
    return Buffer.from(this.ctx.cipher(message.buffer || message))
  }

  decipher(message) {
    if (message instanceof ArrayBuffer) message = Buffer.from(message)
    const buf = this.ctx.decipher(message)
    return Buffer.from(buf)
  }
}

module.exports = CipherCFB8