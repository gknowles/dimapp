/*
Copyright Glen Knowles 2022 - 2024.
Distributed under the Boost Software License, Version 1.0.

network-common.js - dim webapp
*/

//===========================================================================
function networkIntro(selected) {
    navTopIntro('Debug')
    addOpts({
        computed: {
            navSubSelected() {
                return selected
            },
        },
        methods: {
            navSub() {
                return [
                    { name: 'Connections', href: 'srv/network-conns.html' },
                    { name: 'Routes', href: 'srv/network-routes.html' },
                    { name: 'Messages', href: 'srv/network-messages.html',
                        disabled: true },
                ]
            },
        },
    })
    includeHtmlFragment('srv/navsub.html')
    document.currentScript.remove()
}
