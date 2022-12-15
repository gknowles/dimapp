/*
Copyright Glen Knowles 2022.
Distributed under the Boost Software License, Version 1.0.

initialize.js - dim webapp
*/

var appOpts = {}

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
        computed: {
            nowSecs() {
                return Date.parse(this.now) / 1000
            },
            appName() {
                let out = this.server.baseName
                if (this.server.appIndex > 1)
                    out += this.server.appIndex.toString()
                return out
            },
        },
        methods: {
            readableDuration,
            elapsedTime(val) {
                return this.nowSecs - Date.parse(val) / 1000
            },
            readableAge(val) {
                if (typeof val === 'undefined') return '-'
                return this.readableDuration(this.elapsedTime(val))
            },
            ageClass(val) {
                if (typeof val === 'undefined') return 'disabled'
                let age = this.elapsedTime(val)
                if (age < 3600) return 'null'
                if (age < 24 * 3600) return 'recent'
                return 'old'
            },
            miniNav() {
                return []
            },
        },
    }
    addOpts(newOpts)

    const app = Vue.createApp(appOpts)
    for (var name in appOpts.components) {
        app.component(name, appOpts.components[name])
    }
    app.mount('#app')
    let el = document.getElementById('app')
    el.classList.add('groupType-' + appOpts.data().server.groupType)
    el.style.visibility = 'visible'
}
createApp.waitingHtmlFragments = 1

//===========================================================================
function finalize() {
    let tags = [
        { tag: 'script', props: {
            src: '/web/srv/vendor/popperjs@2.10.2/popper.min.js',
            integrity:
'sha384-7+zCNj/IqJ95wo16oMtfsKbZ9ccEh31eOz1HGyDuCQ6wgnyJNSYdrPa03rtR1zdB',
            crossOrigin: 'anonymous',
            onerror: (event) => { this.href =
'https://cdn.jsdelivr.net/npm/@popperjs/core@2.10.2/dist/umd/popper.min.js'
            },
        }},
        { tag: 'script', props: {
            src: '/web/srv/vendor/bootstrap@5.1.3/bootstrap.min.js',
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
    frame.src = src
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
            href: '/web/srv/vendor/bootstrap@5.1.3/bootstrap.min.css',
            onerror: (event) => { this.href =
'https://cdn.jsdelivr.net/npm/bootstrap@5.1.3/dist/css/bootstrap.min.css'
            },
        }},
        { tag: 'link', props: {
            rel: 'stylesheet',
            href: '/web/srv/vendor/bootstrap-icons@1.8.1/bootstrap-icons.css',
            onerror: (event) => { this.href =
'https://cdn.jsdelivr.net/npm/bootstrap-icons@1.8.1/font/bootstrap-icons.css'
            },
        }},
        { tag: 'link', props: {
            rel: 'stylesheet',
            href: 'init.css'
        }},
        { tag: 'link', props: {
            rel: 'stylesheet',
            href: 'groupType.css'
        }},
        { tag: 'script', props: {
            src: '/web/srv/vendor/vue@3.2.45/vue.global.prod.js',
            onerror: (event) => { this.href =
'https://unpkg.com/vue@3.2.45/dist/vue.global.prod.js'
            },
        }},
        { tag: 'script', props: {
            src: 'util.js',
        }},
    ]

    addTags(tags)
    window.addEventListener('load', createApp)
}())
