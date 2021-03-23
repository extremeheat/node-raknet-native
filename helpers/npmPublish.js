// This script compiles the CI outputs
// and publishes them to npm as pre-built modules
// so users do not need to build themselves
// This only works for x64

const fs = require('fs')
const proc = require('child_process')

// console.log('Current dir:')

// if (process.platform == 'win32') {
//     proc.execSync('dir /s /b')
// } else {
//     proc.execSync('ls -r')
// }

function saveBuilds () {
  if (!fs.existsSync('../prebuilds')) {
    fs.mkdirSync('../prebuilds/')
  }

  fs.readdirSync('.').forEach(file => {
    if (file.startsWith('dist')) {
      const name = file.split('dist-')[1]
      const newName = '../prebuilds/' + name.replace('dist-', '')
      console.log('Moving ', name, 'to', newName)
      fs.renameSync(file, newName)
    }
  })
  console.log('Ready for publish!')
}
saveBuilds()

module.exports = {}
