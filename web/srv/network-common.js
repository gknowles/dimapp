/*
Copyright Glen Knowles 2022.
Distributed under the Boost Software License, Version 1.0.

network-common.js - dim webapp
*/

(function() {
    addOpts({
        methods: {
            miniNav() {
                return [
                    { name: 'Connections', href: 'network-conns.html', disabled: true },
                    { name: 'Routes', href: 'network-routes.html' },
                    { name: 'Messages', href: 'network-messages.html', disabled: true },
                ]
            },
        },
    })
})()

function networkIntro(selected) {
    addOpts({
        computed: {
            miniNavSelected() {
                return selected
            },
        },
    })
    includeHtmlFragment('navbar-mini.html', true)
    document.currentScript.remove()
}
