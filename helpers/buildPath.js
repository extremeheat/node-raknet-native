const fs = require('fs')
const os = require('os')
const path = require('path')

module.exports = {
  getPath() {
    let _osVersion = os.release()

    let plat = process.platform
    let arch = process.arch
    let ver = _osVersion.split('.', 1)

    let bpath = `./prebuilds/${plat}-${ver}-${arch}/`
    return bpath
  },

  getFallbackPath() { // try to ignore OS version & load just on plat+arch
    const dirs = fs.readdirSync('./prebuilds/')
    for (const dir of dirs) {
      const [plat,ver,arch] = dir.split('-')

      if (plat === process.platform && arch === process.arch) {
        return dir
      }
    }
  },
  
  getPlatformString() {
    let _osVersion = os.release()

    let plat = process.platform
    let arch = process.arch
    let ver = _osVersion.split('.', 1)
    return `${plat}-${ver}-${arch}`
  }
}