/*
Copyright Glen Knowles 2022.
Distributed under the Boost Software License, Version 1.0.

util.js - dim webapp
*/

//===========================================================================
function readableDuration(val) {
    if (!val)
        return isNaN(val) ? 'NaNs' : '0s'
    let out = ''
    if (val < 0) {
        out += '-'
        val = -val
    }
    let units = [
        { name: 'y', secs: 365 * 24 * 60 * 60 },
        { name: 'w', secs: 7 * 24 * 60 * 60 },
        { name: 'd', secs: 24 * 60 * 60 },
        { name: 'h', secs: 60 * 60 },
        { name: 'm', secs: 60 },
        { name: 's', secs: 1 },
        { name: 'ms', secs: 0.001 },
        { name: undefined, secs: 0 },
    ]
    let matched = false
    for (u of units) {
        if (u.secs > val) continue
        out += Math.trunc(val / u.secs).toFixed(0)
        out += u.name
        val %= u.secs
        if (!val || matched) break
        out += ' '
        matched = true
    }
    return out
}

//===========================================================================
// Splits arr onto an array of pages where each page is an array of up to
// pageSize elements of arr.
function makePages(arr, pageSize) {
      let out = []
      let pageOut = []
      for (let [i, cnt] of arr.entries()) {
        if (i % pageSize == 0)
          out.push([])
        out[out.length - 1].push(cnt)
      }
      return out
}
