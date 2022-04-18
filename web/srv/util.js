/*
Copyright Glen Knowles 2022.
Distributed under the Boost Software License, Version 1.0.
*/

// Duration from this.now to val, measured in seconds, where val is an ISO-8601
// date string.
function elapsedTime(val) {
    return this.nowSecs - Date.parse(val) / 1000
}

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
