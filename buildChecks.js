// Some helpful pre-build enviornment checks
const fs = require('fs')
const cp = require('child_process')

function checkIfPrebuildExists () {
  try {
    const bindings = require('./binding')
    if (!bindings) throw new Error('Bindings are undefined')
    console.log('[raknet] not building as already have prebuild')
    return true
  } catch (e) {
    console.log(e)
    console.log('[raknet] need to build')
  }
}

let runCmake = true

if (!process.env.FORCE_BUILD) {
  if (checkIfPrebuildExists()) {
    runCmake = false
  }
}
if (process.env.SKIP_BUILD) {
  runCmake = false
}

async function runChecks () {
  if (!fs.existsSync('./raknet/Source')) {
    console.info('Cloning submodules...')
    cp.execSync('git submodule init', { stdio: 'inherit' })
    cp.execSync('git submodule update', { stdio: 'inherit' })

    if (!fs.existsSync('./raknet/Source')) { // npm install does not clone submodules...
      cp.execSync('git clone https://github.com/extremeheat/fb-raknet raknet') // so do it manually

      if (!fs.existsSync('./raknet/Source')) { // give up
        console.error('******************* READ ME ****************\n')
        console.error(' Failed to install git submodules. Please create an issue at https://github.com/extremeheat/node-raknet-native\n')
        console.error('******************* READ ME ****************\n')
        process.exit(1)
      }
    }
  }
}

if (runCmake) {
  runChecks().then(
    () => {
      console.log('Build checks are passing! Building...')
      // cp.execSync(`ls -R`, {stdio: 'inherit'})
      cp.execSync('cmake-js compile', { stdio: 'inherit' })
    }
  )
}

module.exports = () => { }
