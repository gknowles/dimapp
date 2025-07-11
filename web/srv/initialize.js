/*
Copyright Glen Knowles 2022 - 2025.
Distributed under the Boost Software License, Version 1.0.

initialize.js - dim webapp
*/

var appOpts = {}

// This script is at <base>/srv/initialize.js, resolve src relative to <base>.
var appRoot = new URL(document.currentScript.src, document.URL)
appRoot = new URL('..', appRoot)

//===========================================================================
function createApp() {
    createApp.waitingHtmlFragments -= 1
    if (createApp.waitingHtmlFragments > 0)
        return

    let newOpts = appOpts
    appOpts = {
        data() {
            return srvdata
        },
        mounted() {
            // Listener for sortable table headers
            const hdrs = document.querySelectorAll("th.sortable");
            hdrs.forEach(
                hdr => hdr.addEventListener(
                    'click',
                    () => { this.tableSort(hdr) }
                )
            )

            // Make #app element visible and reflect the server's group type.
            let el = document.getElementById('app')
            el.classList.add('groupType-' + appOpts.data().server.groupType)
            el.style.visibility = 'visible'

            // Set page refresh time if there's a "refresh" query parameter.
            let ref = parseInt(getParam('refresh'))
            if (ref) {
                setTimeout(() => { window.location.reload() }, ref * 1000)
            }
        },
        computed: {
            nowSecs() {
                return Date.parse(this.now) / 1000
            },
            appName() {
                let out = this.server.productName
                if (this.server.appIndex > 1)
                    out += this.server.appIndex.toString()
                return out
            },
        },
        methods: {
            ageClass(val) {
                if (typeof val === 'undefined') return 'bg-disabled'
                let age = this.elapsedTime(val)
                if (age < 3600) return 'null'
                if (age < 24 * 3600) return 'bg-recent'
                return 'bg-old'
            },
            elapsedTime(val) {
                return this.nowSecs - Date.parse(val) / 1000
            },
            getParam,
            getRefreshUrl,
            makeUrl,
            navSub() { return [] },
            navTop() { return [] },
            readableAge(val) {
                if (typeof val === 'undefined') return '-'
                return this.readableDuration(this.elapsedTime(val))
            },
            readableDuration,
            tableSort,
            updateUrl,
        },
    }
    addOpts(newOpts)

    const app = Vue.createApp(appOpts)
    for (var name in appOpts.components) {
        app.component(name, appOpts.components[name])
    }
    app.mount('#app')
}
createApp.waitingHtmlFragments = 1

//===========================================================================
function finalize() {
    let tags = [
        { tag: 'script', props: {
            src: appRoot + 'srv/vendor/popperjs@2.10.2/popper.min.js',
            integrity:
'sha384-7+zCNj/IqJ95wo16oMtfsKbZ9ccEh31eOz1HGyDuCQ6wgnyJNSYdrPa03rtR1zdB',
            crossOrigin: 'anonymous',
            onerror: (event) => { this.href =
'https://cdn.jsdelivr.net/npm/@popperjs/core@2.10.2/dist/umd/popper.min.js'
            },
        }},
        { tag: 'script', props: {
            src: appRoot + 'srv/vendor/bootstrap@5.1.3/bootstrap.min.js',
            integrity:
'sha384-QJHtvGhmr9XOIpI6YVutG+2QOK9T+ZnN4kzFN1RtK3zEFEIsxhlmWl5/YESvpZ13',
            crossOrigin: 'anonymous',
            onerror: (event) => { this.href =
'https://cdn.jsdelivr.net/npm/bootstrap@5.1.3/dist/js/bootstrap.min.js'
            },
        }},
    ]

    addTags(tags)
}

//===========================================================================
function includeHtmlFragment(src) {
    let node = document.currentScript
    createApp.waitingHtmlFragments += 1
    const frame = document.createElement("iframe")
    node.insertAdjacentElement('beforebegin', frame)
    frame.style.display = 'none'
    frame.onload = function() {
        this.insertAdjacentHTML(
            'afterend',
            this.contentDocument.body.innerHTML
        )
        this.remove()
        createApp()
    }
    let url = new URL(src, appRoot)
    frame.src = url.href
}

//===========================================================================
function replaceWithHtmlFragment(src, keep) {
    let node = document.currentScript
    includeHtmlFragment(src)
    node.remove()
}

//===========================================================================
// Merge in additional application options.
function addOpts(newOpts) {
    if (newOpts !== null) {
        for (let opt in newOpts) {
            let val = newOpts[opt]
            if (val === null)
                continue
            if (typeof val === 'object' && appOpts[opt]) {
                Object.assign(appOpts[opt], val)
            } else if (typeof val == 'array') {
                alert('appOpt[' + opt + ']: array options not supported')
            } else {
                appOpts[opt] = val
            }
        }
    }
}

//===========================================================================
function addTags(tags) {
    let self = document.currentScript
    for (let i = tags.length - 1; i >= 0; --i) {
        let el = document.createElement(tags[i].tag)
        for (let prop in tags[i].props) {
            el[prop] = tags[i].props[prop]
        }
        self.insertAdjacentElement('afterend', el)
    }
}


/****************************************************************************
*
*   Main
*
***/

//===========================================================================
(function() {
    let tags = [
        { tag: 'link', props: {
            rel: 'stylesheet',
            integrity:
'sha384-1BmE4kWBq78iYhFldvKuhfTAU6auU8tT94WrHftjDbrCEXSU1oBoqyl2QvZ6jIW3',
            crossOrigin: 'anonymous',
            href: appRoot + 'srv/vendor/bootstrap@5.1.3/bootstrap.min.css',
            onerror: (event) => { this.href =
'https://cdn.jsdelivr.net/npm/bootstrap@5.1.3/dist/css/bootstrap.min.css'
            },
        }},
        { tag: 'link', props: {
            rel: 'stylesheet',
            integrity:
'sha384-He3RckdFB2wffiHOcESa3sf4Ida+ni/fw9SSzAcfY2EPnU1zkK/sLUzw2C5Tyuhj',
            crossOrigin: 'anonymous',
            href: appRoot +
                'srv/vendor/bootstrap-icons@1.8.1/bootstrap-icons.css',
            onerror: (event) => { this.href =
'https://cdn.jsdelivr.net/npm/bootstrap-icons@1.8.1/font/bootstrap-icons.css'
            },
        }},
        { tag: 'link', props: {
            rel: 'stylesheet',
            href: appRoot + 'srv/init.css'
        }},
        { tag: 'link', props: {
            rel: 'stylesheet',
            href: appRoot + 'srv/groupType.css'
        }},
        { tag: 'script', props: {
            src: appRoot + 'srv/vendor/vue@3.2.45/vue.global.prod.js',
            integrity:
'sha384-w4L+s8BW1EtTnMQtvUXyHkzjvhpPyCGR7IBebBAHquUXQ20ZCLuAVg2ZWRWriVPL',
            crossOrigin: 'anonymous',
            onerror: (event) => { this.href =
'https://unpkg.com/vue@3.2.45/dist/vue.global.prod.js'
            },
        }},
        { tag: 'script', props: {
            src: appRoot + 'srv/util.js',
        }},
    ]

    addTags(tags)
    window.addEventListener('load', createApp)
}())
