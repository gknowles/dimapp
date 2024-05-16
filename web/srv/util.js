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
    for (let u of units) {
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
function parseDuration(val) {
    let units = {
        y: 365 * 24 * 60 * 60,
        w: 7 * 24 * 60 * 60,
        d: 24 * 60 * 60,
        h: 60 * 60,
        m: 60,
        s: 1,
        ms: 0.001,
    }
    if (val == '-')
        return Infinity
    let out = 0
    let vals = val.split(' ')
    if (vals.length == 0)
        return NaN
    for (let v of vals) {
        let match = v.match(/([^a-z]+)([a-z]+)/)
        if (!match || match[0] != v)
            return NaN
        let vn = parseInt(match[1]) * units[match[2]];
        if (isNaN(vn))
            return vn
        out += vn
    }
    return out
}

//===========================================================================
function parseLocaleFloat(val) {
    return Number(val.toString().replace(',', ''))
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
// Updates key with value.
//  - null or undefined value removes the key from the query string.
//  - non-array value replaces the key's value.
//  - array value has its members appended to the key, except that null members
//    remove the key. So you can replace an array by using an array value whose
//    first member is a null.
function updateUrlParam(query, key, val) {
    if (val == null) {
        query.delete(key)
    } else if (Array.isArray(val)) {
        for (let v of val) {
            if (v == null) {
                query.delete(key)
            } else {
                query.append(key, v)
            }
        }
    } else {
        query.set(key, val)
    }
}

//===========================================================================
// Return query string containing the parameters from the current location
// and overridden with values from params.
function updateUrl(params) {
    let loc = window.location
    let query = new URLSearchParams(loc.search)
    for (let n in params) {
        updateUrlParam(query, n, params[n])
    }
    return '?' + query.toString()
}

//===========================================================================
function getParam(name, defVal) {
    let params = new URLSearchParams(window.location.search)
    return params.get(name) || defVal
}

//===========================================================================
function getRefreshUrl() {
    let rawRef = getParam('refresh')
    let ref = parseInt(rawRef) || Infinity;
    let refs = [Infinity, 2, 10];
    let pos = (refs.indexOf(ref) + 1);
    return updateUrl({refresh: refs[pos]});
}


/****************************************************************************
*
*   Sorting tables
*
*   Thanks to answer for "Sorting HTML table with JavaScript" at
*   https://stackoverflow.com/questions/14267781
*
***/

//===========================================================================
function tableCellIndex(th) {
    const table = th.closest('table')
    let indexes = []
    for (let row of table.rows) {
        let x = 0
        for (let cell of row.cells) {
            while (indexes[x] > 0) {
                indexes[x] -= 1
                x += 1
            }
            if (cell === th)
                return x
            for (let espan = x + cell.colSpan; x < espan; ++x) {
                indexes[x] = cell.rowSpan - 1
            }
        }
        while (x < indexes.length && indexes[x] > 0) {
            indexes[x] -= 1
            x += 1
        }
    }
}

//===========================================================================
function tableCellValue(tr, idx) {
    const node = tr.cells[idx]
    if (node == null)
        return ""
    return node.getAttribute('sort-key')
        || node.innerText
        || node.textContent
}

//===========================================================================
function tableRowCompare(idx, asc) {
    return (a, b) => {
        const v1 = tableCellValue(a, idx)
        const v2 = tableCellValue(b, idx)

        // Durations?
        const d1 = parseDuration(v1)
        if (!isNaN(d1)) {
            const d2 = parseDuration(v2)
            if (!isNaN(d2)) {
                // Can't subtract because d1 and/or d2 could be infinity.
                const cmp = d1 == d2 ? 0 : d1 < d2 ? -1 : 1
                return asc ? cmp : -cmp
            }
        }

        // Locale numbers?
        const n1 = parseLocaleFloat(v1)
        if (!isNaN(n1)) {
            const n2 = parseLocaleFloat(v2)
            if (!isNaN(n2))
                return asc ? n1 - n2 : n2 - n1
        }

        // Must be strings
        const cmp = v1.toString().localeCompare(v2)
        return asc ? cmp : -cmp;
    }
}

//===========================================================================
function tableHeaderRows(table, th) {
    const same = (table == th.closest('table'))
    for (const x in table.rows) {
        if (table.rows[x].classList.contains('unsortable'))
            continue
        for (let cell of table.rows[x].cells) {
            if (cell.nodeName != 'TH')
                return parseInt(x)
        }
    }
    return table.rows.length
}

//===========================================================================
function updateHeaderClass(table, th, asc) {
    const same = (table == th.closest('table'))
    for (const x in table.rows) {
        for (let cell of table.rows[x].cells) {
            if (cell.nodeName != 'TH')
                return
            if (same && cell === th
                || !same && cell.textContent == th.textContent
            ) {
                cell.classList.add('table-sort-active')
                if (asc) {
                    cell.classList.add('table-sort-asc')
                    cell.classList.remove('table-sort-desc')
                } else {
                    cell.classList.add('table-sort-desc')
                    cell.classList.remove('table-sort-asc')
                }
            } else {
                cell.classList.remove('table-sort-active')
            }
        }
    }
}

//===========================================================================
function tableSortDirection(th, rows) {
    // Get column index and sort direction of key
    const idx = tableCellIndex(th)
    const rattr = th.getAttribute('sort-reverse')
    const rev = rattr != undefined && rattr != '0' && rattr != 'false'

    let asc
    if (th.classList.contains('table-sort-asc')) {
        // Was ascending, target descending (i.e. not ascending)
        asc = false
    } else if (th.classList.contains('table-sort-desc')) {
        // Was descending, target ascending
        asc = true
    } else {
        asc = false
        let cmp = tableRowCompare(idx, rev ? !asc : asc)
        for (let i = 1; i < rows.length; ++i) {
            if (cmp(rows[i - 1], rows[i]) < 0) {
                // Found row not in ascending order, change to ascending sort.
                asc = true
                break
            }
        }
    }
    let cmp = tableRowCompare(idx, rev ? !asc : asc)
    return [asc, cmp]
}

//===========================================================================
// Configurable classes:
//      div.sort-group - Contains one or more tables, all with the same
//          columns, whose rows are sorted as a group.
//      tr.unsortable - Not repositioned by sort. Must immediately follow the
//          header (or other unsortable) rows for this to be honored.
// Configurable attributes:
//      th.sort-reverse - Comparisons are reversed.
//      td.sort-key - Used instead of content for comparisons, if present.
function tableSort(th) {
    const table = th.closest('table')
    const grproot = table.closest('div.sort-group')
    const tables = grproot
        ? Array.from(grproot.getElementsByTagName('table'))
        : [table]
    const tabs = tables.map((x) => ({
        table: x,
        skipped: tableHeaderRows(x, th),
        count: x.rows.length,
    }))
    let rows = []
    for (let tab of tabs) {
        let trows = Array.from(tab.table.rows).slice(tab.skipped)
        rows = rows.concat(trows)
    }

    let [asc, cmp] = tableSortDirection(th, rows)
    rows.sort(cmp)
    if (rows.length > 0 && cmp(rows[0], rows.at(-1)) == 0) {
        // All values are equal, report sort as ascending.
        asc = true
    }

    // Move the now sorted rows back to tables in group and update the arrow
    // symbols in their headers.
    for (let tab of tabs) {
        let trows = rows.splice(0, tab.count - tab.skipped)
        tab.table.tBodies[0].append(...trows)
        updateHeaderClass(tab.table, th, asc)
    }
}
