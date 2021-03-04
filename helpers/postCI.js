const { getPlatformString } = require('./buildPath')

console.log('::set-output name=PLATFORM_STRING::' + getPlatformString())
