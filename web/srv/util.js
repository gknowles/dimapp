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

//===========================================================================
function makeUrl(path, params) {
    let out = ""
    for (let n in params) {
        out = out.concat(
            '&',
            encodeURIComponent(n),
            '=',
            encodeURIComponent(params[n])
        )
    }
    if (out.length > 0) {
        return path + '?' + out.substring(1)
    } else {
        return path
    }
}

//===========================================================================
// Return query string containing the parameters from the current location
// and overridden with values from params.
function updateUrl(params) {
    let loc = window.location
    let query = new URLSearchParams(loc.search)
    for (let n in params) {
        query.set(n, params[n])
    }
    return '?' + query.toString()
}


/****************************************************************************
*
*   Sorting tables
*
*   Based on answer to "Sorting HTML table with JavaScript" at
*   https://stackoverflow.com/questions/14267781
*
***/

//===========================================================================
function tableCellValue(tr, idx) {
    const node = tr.children[idx]
    return node.getAttribute('sort-key')
        || node.innerText
        || node.textContent
}

//===========================================================================
function tableRowCompare(idx, asc) {
    return (a, b) => {
        const v1 = tableCellValue(a, idx)
        const v2 = tableCellValue(b, idx)
        const cmp = v1 !== '' && v2 !== '' && !isNaN(v1) && !isNaN(v2)
            ? v1 - v2
            : v1.toString().localeCompare(v2);
        return asc ? cmp : -cmp;
    }
}

//===========================================================================
// Options:
//  skipRows    Header rows to leave in place (default: 1)
//  reverse     Reverse comparator making up down and down up (default: false)
//  asc         Initial sort order is ascending (default: false)
//  idx         Row column to use as key, defaults to the position of this cell
//                in its containing row. Used to adjust for colspan/rowspan.
function tableSort(th, options) {
    const opts = options || {}
    let asc = opts.asc == undefined ? false : opts.asc
    const skips = opts.skipRows || 1
    const rev = opts.reverse || false
    const table = th.closest('table')
    const rows = Array.from(table.tBodies[0].rows).slice(skips)
    const idx = opts.idx == undefined
        ? Array.from(th.parentNode.children).indexOf(th)
        : opts.idx
    asc = typeof th['sort-asc'] !== 'undefined'
        ? !th['sort-asc']
        : !asc
    th['sort-asc'] = asc
    rows.sort(tableRowCompare(idx, rev ? !asc : asc))
    table.tBodies[0].append(...rows)
    if (asc) {
        th.classList.add('table-sort-asc')
        th.classList.remove('table-sort-desc')
    } else {
        th.classList.add('table-sort-desc')
        th.classList.remove('table-sort-asc')
    }
}
