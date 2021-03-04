const helper = require('./helpers/buildPath.js')
const path = require('path')
const debug = require('debug')('raknet')

if (!process.versions.electron) { 
  // Electron has its own crash handler, and segfault-handler
  // uses NAN which is a hassle, so only load outside electron
  try {
    var SegfaultHandler = require('segfault-handler')
    SegfaultHandler.registerHandler("crash.log")
  } catch (e) {
    debug('[raknet] segfault handler is not installed. If you run into crashing issues, install it with `npm i -D segfault-handler` to get debug info on native crashes')
  }
}

var bindings
var pathToSearch = helper.getPath()
if (pathToSearch) {
  var rpath = path.join(__dirname, pathToSearch, '/node-raknet.node')
  try {
    bindings = require(rpath)
  } catch (e) {
    console.log(e)
    console.warn('[raknet] did not find lib in ', rpath)
  }
}
if (!bindings) {
  bindings = require('bindings')('node-raknet.node')
}

module.exports = bindings