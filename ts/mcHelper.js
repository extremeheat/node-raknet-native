const { MinecraftHelper } = require('../binding')

class CipherCFB8 {
  constructor (secret, iv) {
    if (secret instanceof Buffer && secret.buffer.byteLength != secret.byteLength) secret = new Uint8Array(secret)
    if (iv instanceof Buffer && iv.buffer.byteLength != iv.byteLength) iv = new Uint8Array(iv)
    this.ctx = new MinecraftHelper()
    this.ctx.createCipher('CFB8', secret.buffer ?? secret, iv.buffer ?? iv.buffer)
  }

  cipher (message) {
    if (message instanceof ArrayBuffer) message = Buffer.from(message)
    return Buffer.from(this.ctx.cipher(message))
  }

  decipher (message) {
    if (message instanceof ArrayBuffer) message = Buffer.from(message)
    const buf = this.ctx.decipher(message)
    return Buffer.from(buf)
  }
}

class CipherGCM {
  constructor (secret, iv) {
    if (secret instanceof Buffer && secret.buffer.byteLength != secret.byteLength) secret = new Uint8Array(secret)
    if (iv instanceof Buffer && iv.buffer.byteLength != iv.byteLength) iv = new Uint8Array(iv)
    this.ctx = new MinecraftHelper()
    this.ctx.createCipher('GCM', secret.buffer ?? secret, iv.buffer ?? iv)
  }

  cipher (message) {
    if (message instanceof ArrayBuffer) message = Buffer.from(message)
    return Buffer.from(this.ctx.cipher(message))
  }

  decipher (message) {
    if (message instanceof ArrayBuffer) message = Buffer.from(message)
    const buf = this.ctx.decipher(message)
    return Buffer.from(buf)
  }
}

module.exports = { CipherGCM, CipherCFB8 }
