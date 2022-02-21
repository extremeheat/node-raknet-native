const helper = require('./helpers/buildPath.js')
const fs = require('fs')
const debug = require('debug')('raknet')

if (!process.versions.electron) {
  // Electron has its own crash handler, and segfault-handler
  // uses NAN which is a hassle, so only load outside electron
  // Note: Need to be using debug release to make use of this! Run `npm run clean` then `npm test` to get a debug build
  try {
    const SegfaultHandler = require('segfault-handler')
    SegfaultHandler.registerHandler('crash.log')
  } catch (e) {
    debug('[raknet] segfault handler is not installed. If you run into crashing issues, install it with `npm i -D segfault-handler` to get debug info on native crashes')
  }
}

let bindings
const pathsToSearch = [helper.getPath()]
for (const importPath of pathsToSearch) {
  try {
    bindings = require(importPath)
  } catch (e) {
    debug("Didn't find in", importPath)
    debug(e)
  }
}

// We want to use the built release if available
if (!bindings || fs.existsSync('./build/Release')) {
  bindings = require('bindings')('node-raknet.node')
}

module.exports = bindings
