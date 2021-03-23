const fs = require('fs')
const cp = require('child_process')

// var packageJson = fs.readFileSync('../package.json', 'utf-8')

cp.execSync('npm version patch', { stdio: 'inherit' })
cp.execSync('git push --follow-tags', { stdio: 'inherit' })

// packageJson.replace(/"version": "[0-9\.]+\.[0-9\.].[0-9\.]",/, )
